#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>

/**
    A shared, reference-counted wavetable resource.
    Multiple voices/oscillators point at the SAME WavetableData instance,
    so loading one preset never duplicates wavetable memory per-voice.

    Storage is 16-bit (int16) rather than 32-bit float to halve memory
    footprint; samples are de-quantized to float only during interpolation.
*/
struct WavetableData : public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<WavetableData>;

    int numFrames = 1;          // wavetable "position" frames (e.g. 1-256)
    int frameSize = 2048;       // samples per single-cycle frame
    int numMipLevels = 1;       // bandlimited mip levels per frame

    // data[mipLevel][frame][sampleIndex], stored as int16 fixed point (-32768..32767 == -1..1)
    std::vector<std::vector<std::vector<int16_t>>> data;

    // Hash of the source content, used for de-duplication in the shared pool.
    juce::uint64 contentHash = 0;

    inline float sampleAt(int mip, int frame, int index) const noexcept
    {
        return data[mip][frame][index] / 32768.0f;
    }
};

/** Global pool so identical wavetables loaded from different presets are stored once. */
class WavetablePool
{
public:
    static WavetablePool& instance()
    {
        static WavetablePool pool;
        return pool;
    }

    WavetableData::Ptr getOrCreate(juce::uint64 hash, std::function<WavetableData::Ptr()> factory)
    {
        const juce::ScopedLock sl(lock);
        if (auto existing = pool[hash])
            return existing;

        auto created = factory();
        created->contentHash = hash;
        pool[hash] = created;
        return created;
    }

    void purgeUnused()
    {
        const juce::ScopedLock sl(lock);
        for (auto it = pool.begin(); it != pool.end(); )
        {
            if (it->second->getReferenceCount() <= 1)
                it = pool.erase(it);
            else
                ++it;
        }
    }

private:
    juce::CriticalSection lock;
    std::unordered_map<juce::uint64, WavetableData::Ptr> pool;
};

/**
    A single bandlimited wavetable oscillator with unison support.
    Unison voices share ONE WavetableData buffer (phase-randomized copies only,
    no per-voice wavetable duplication).
*/
class WavetableOscillator
{
public:
    void setWavetable(WavetableData::Ptr wt) { table = wt; }

    void prepare(double sr) { sampleRate = sr; }

    void setUnison(int voices, float detuneCents, float stereoSpread)
    {
        unisonVoices = juce::jlimit(1, 16, voices);
        detune = detuneCents;
        spread = stereoSpread;
        phases.assign((size_t) unisonVoices, 0.0);
        phaseOffsetsInitialised = false;
    }

    void noteOn(float baseFreqHz)
    {
        freq = baseFreqHz;
        if (!phaseOffsetsInitialised)
        {
            juce::Random rng;
            for (auto& p : phases)
                p = rng.nextDouble();
            phaseOffsetsInitialised = true;
        }
    }

    void setWavetablePosition(float pos01) { wtPosition = juce::jlimit(0.0f, 1.0f, pos01); }

    /** Renders one stereo sample frame (L,R) additively across unison voices. */
    void renderNextFrame(float& outL, float& outR)
    {
        if (table == nullptr || table->numFrames == 0)
        {
            outL = outR = 0.0f;
            return;
        }

        float sumL = 0.0f, sumR = 0.0f;
        const int frameCount = table->numFrames;
        const float framePosF = wtPosition * (frameCount - 1);
        const int frameLo = (int) framePosF;
        const int frameHi = juce::jmin(frameLo + 1, frameCount - 1);
        const float frameFrac = framePosF - frameLo;

        const int mip = chooseMipLevel(freq, sampleRate, table->frameSize, table->numMipLevels);

        for (int v = 0; v < unisonVoices; ++v)
        {
            const float voiceSpread = unisonVoices > 1
                ? juce::jmap((float) v, 0.0f, (float) (unisonVoices - 1), -1.0f, 1.0f)
                : 0.0f;
            const float detuneRatio = std::pow(2.0f, (voiceSpread * detune) / 1200.0f);
            const double phaseInc = (freq * detuneRatio) / sampleRate;

            phases[(size_t) v] += phaseInc;
            if (phases[(size_t) v] >= 1.0) phases[(size_t) v] -= 1.0;

            const float sample = cubicSample(mip, frameLo, frameHi, frameFrac, (float) phases[(size_t) v]);

            const float pan = unisonVoices > 1 ? juce::jmap((float) v, 0.0f, (float) (unisonVoices - 1), -spread, spread) : 0.0f;
            const float gL = std::sqrt(0.5f * (1.0f - pan));
            const float gR = std::sqrt(0.5f * (1.0f + pan));

            sumL += sample * gL;
            sumR += sample * gR;
        }

        const float norm = 1.0f / std::sqrt((float) unisonVoices);
        outL = sumL * norm;
        outR = sumR * norm;
    }

private:
    static int chooseMipLevel(float f, double sr, int frameSize, int mipLevels)
    {
        // Pick the mip level whose harmonic content stays below Nyquist.
        // Higher fundamental frequency -> fewer harmonics allowed -> higher mip index.
        const float nyquist = (float) sr * 0.5f;
        const float maxHarmonics = nyquist / juce::jmax(f, 1.0f);
        int level = (int) juce::jlimit(0.0, (double) (mipLevels - 1), (double) std::log2(juce::jmax(1.0f, (float) frameSize / (2.0f * maxHarmonics))));
        return level;
    }

    float cubicSample(int mip, int frameLo, int frameHi, float frameFrac, float phase) const
    {
        const int size = table->frameSize;
        const float posF = phase * size;
        const int i0 = ((int) posF - 1 + size) % size;
        const int i1 = ((int) posF) % size;
        const int i2 = (i1 + 1) % size;
        const int i3 = (i1 + 2) % size;
        const float frac = posF - (int) posF;

        auto s = [&](int frame, int idx) { return table->sampleAt(mip, frame, idx); };

        auto cubicAt = [&](int frame)
        {
            const float y0 = s(frame, i0), y1 = s(frame, i1), y2 = s(frame, i2), y3 = s(frame, i3);
            const float a0 = y3 - y2 - y0 + y1;
            const float a1 = y0 - y1 - a0;
            const float a2 = y2 - y0;
            const float a3 = y1;
            return a0 * frac * frac * frac + a1 * frac * frac + a2 * frac + a3;
        };

        const float lo = cubicAt(frameLo);
        const float hi = cubicAt(frameHi);
        return lo + (hi - lo) * frameFrac;
    }

    WavetableData::Ptr table;
    double sampleRate = 44100.0;
    float freq = 440.0f;
    float wtPosition = 0.0f;

    int unisonVoices = 1;
    float detune = 0.0f;
    float spread = 0.0f;
    std::vector<double> phases;
    bool phaseOffsetsInitialised = false;
};
