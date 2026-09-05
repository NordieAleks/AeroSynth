#pragma once
#include <JuceHeader.h>
#include "WavetableOscillator.h"
#include "Envelope.h"
#include "LFO.h"
#include "Filter.h"
#include "ModulationMatrix.h"
#include "EffectsChain.h"
#include "../Presets/PresetTypes.h"

/** One polyphonic voice. Fully pre-allocated; no heap activity during noteOn/noteOff/render. */
struct SynthVoice
{
    std::array<WavetableOscillator, 3> oscillators;
    Envelope ampEnv;
    Envelope modEnv;
    std::array<LFO, 3> lfos;
    MultiModeFilter filter;
    ModulationMatrix modMatrix;

    int midiNote = -1;
    float velocity01 = 1.0f;
    bool active = false;

    void prepare(double sr)
    {
        for (auto& o : oscillators) o.prepare(sr);
        ampEnv.prepare(sr);
        modEnv.prepare(sr);
        for (auto& l : lfos) l.prepare(sr);
        filter.prepare(sr);
    }

    void noteOn(int note, float velocity)
    {
        midiNote = note;
        velocity01 = velocity;
        active = true;
        const float freq = (float) juce::MidiMessage::getMidiNoteInHertz(note);
        for (auto& o : oscillators) o.noteOn(freq);
        ampEnv.noteOn();
        modEnv.noteOn();
        for (auto& l : lfos) l.noteOn();
    }

    void noteOff()
    {
        ampEnv.noteOff();
        modEnv.noteOff();
    }

    bool isFinished() const { return active && !ampEnv.isActive(); }

    void renderNextFrame(float& outL, float& outR, const AeroPreset& preset)
    {
        modMatrix.setSourceValue(ModSource::Velocity, velocity01);
        modMatrix.setSourceValue(ModSource::Env1, ampEnv.renderNextSample());
        modMatrix.setSourceValue(ModSource::Env2, modEnv.renderNextSample());
        modMatrix.setSourceValue(ModSource::Lfo1, lfos[0].renderNextSample());
        modMatrix.setSourceValue(ModSource::Lfo2, lfos[1].renderNextSample());
        modMatrix.setSourceValue(ModSource::Lfo3, lfos[2].renderNextSample());

        float mixL = 0.0f, mixR = 0.0f;
        static const ModDest wtPosDests[3] = { ModDest::Osc1WtPos, ModDest::Osc2WtPos, ModDest::Osc3WtPos };

        for (size_t i = 0; i < oscillators.size(); ++i)
        {
            const auto& oscSettings = preset.oscillators[i];
            if (!oscSettings.enabled) continue;

            const float modAmount = modMatrix.getModulationFor(wtPosDests[i]);
            oscillators[i].setWavetablePosition(oscSettings.wavetablePosition + modAmount);

            float l, r;
            oscillators[i].renderNextFrame(l, r);
            mixL += l * oscSettings.level;
            mixR += r * oscSettings.level;
        }

        const float cutoffMod = modMatrix.getModulationFor(ModDest::FilterCutoff) * 4000.0f;
        filter.setParameters(preset.filter.type,
                              juce::jlimit(20.0f, 20000.0f, preset.filter.cutoffHz + cutoffMod),
                              preset.filter.resonance);

        mixL = filter.processSample(mixL);
        mixR = filter.processSample(mixR);

        const float amp = ampEnv.isActive() ? velocity01 : 0.0f;
        outL = mixL * amp;
        outR = mixR * amp;

        if (isFinished())
            active = false;
    }
};

/** Owns the voice pool and applies AeroPreset state; the AudioProcessor drives this per-block. */
class SynthEngine
{
public:
    static constexpr int kMaxVoices = 32;

    void prepare(double sampleRate, int blockSize)
    {
        for (auto& v : voices)
            v.prepare(sampleRate);
        effects.prepare(sampleRate, blockSize);
    }

    void loadPreset(const AeroPreset& p)
    {
        currentPreset = p;
        for (auto& v : voices)
        {
            for (size_t i = 0; i < v.oscillators.size(); ++i)
            {
                v.oscillators[i].setWavetable(p.oscillators[i].wavetable);
                v.oscillators[i].setUnison(p.oscillators[i].unisonVoices,
                                            p.oscillators[i].unisonDetuneCents,
                                            p.oscillators[i].unisonSpread);
            }
            v.ampEnv.setParameters(p.envelopes[0].attack, p.envelopes[0].decay, p.envelopes[0].sustain, p.envelopes[0].release);
            v.modEnv.setParameters(p.envelopes[1].attack, p.envelopes[1].decay, p.envelopes[1].sustain, p.envelopes[1].release);
            for (size_t i = 0; i < v.lfos.size(); ++i)
            {
                v.lfos[i].setShape(p.lfos[i].shape);
                v.lfos[i].setFrequency(p.lfos[i].frequencyHz);
                v.lfos[i].setRetrigger(p.lfos[i].retrigger);
            }
            v.modMatrix.setRoutes(p.modRoutes);
        }
        effects.setChorusMix(p.effects.chorusMix);
        effects.setReverbMix(p.effects.reverbMix);
        effects.setDistortionDrive(p.effects.distortionDrive);
    }

    void noteOn(int note, float velocity)
    {
        // Voice-stealing: prefer an idle voice, else steal the oldest active one.
        for (auto& v : voices)
        {
            if (!v.active) { v.noteOn(note, velocity); return; }
        }
        voices[0].noteOn(note, velocity); // simple steal strategy; refine with age-tracking as needed
    }

    void noteOff(int note)
    {
        for (auto& v : voices)
            if (v.active && v.midiNote == note)
                v.noteOff();
    }

    void renderBlock(juce::AudioBuffer<float>& buffer)
    {
        buffer.clear();
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            float sumL = 0.0f, sumR = 0.0f;
            for (auto& v : voices)
            {
                if (!v.active) continue;
                float vl, vr;
                v.renderNextFrame(vl, vr, currentPreset);
                sumL += vl;
                sumR += vr;
            }
            L[s] += sumL * currentPreset.masterVolume;
            R[s] += sumR * currentPreset.masterVolume;
        }

        juce::dsp::AudioBlock<float> block(buffer);
        effects.process(block);
    }

private:
    std::array<SynthVoice, kMaxVoices> voices;
    EffectsChain effects;
    AeroPreset currentPreset;
};
