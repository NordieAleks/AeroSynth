#include "VitalPresetParser.h"

bool VitalPresetParser::canParse(const juce::File& file)
{
    return file.hasFileExtension(".vital");
}

bool VitalPresetParser::parse(const juce::File& file, AeroPreset& outPreset)
{
    auto text = file.loadFileAsString();
    auto json = juce::JSON::parse(text);

    if (!json.isObject())
        return false;

    outPreset.sourceFormat = "vital";
    outPreset.name = json.getProperty("preset_name", file.getFileNameWithoutExtension()).toString();

    // Vital nests synth parameters under "settings" in most exported presets.
    auto settings = json.getProperty("settings", juce::var());
    if (!settings.isObject())
        settings = json; // some exports are flatter; fall back to top-level

    auto oscillatorsArray = settings.getProperty("oscillators", juce::var());
    if (oscillatorsArray.isArray())
    {
        auto* arr = oscillatorsArray.getArray();
        for (int i = 0; i < juce::jmin(3, arr->size()); ++i)
            mapOscillator(arr->getUnchecked(i), outPreset.oscillators[(size_t) i]);
    }

    auto envelopesArray = settings.getProperty("envelopes", juce::var());
    if (envelopesArray.isArray())
    {
        auto* arr = envelopesArray.getArray();
        for (int i = 0; i < juce::jmin(2, arr->size()); ++i)
            mapEnvelope(arr->getUnchecked(i), outPreset.envelopes[(size_t) i]);
    }

    auto filterJson = settings.getProperty("filter_1", juce::var());
    if (filterJson.isObject())
        mapFilter(filterJson, outPreset.filter);

    outPreset.masterVolume = (float) settings.getProperty("volume", 0.8);

    return true;
}

void VitalPresetParser::mapOscillator(const juce::var& oscJson, OscillatorSettings& dest)
{
    dest.enabled = (bool) oscJson.getProperty("on", true);
    dest.level = (float) oscJson.getProperty("level", 0.8);
    dest.pan = (float) oscJson.getProperty("pan", 0.0);
    dest.coarseTuneSemitones = (float) oscJson.getProperty("transpose", 0.0);
    dest.fineTuneCents = (float) oscJson.getProperty("tune", 0.0) * 100.0f;
    dest.wavetablePosition = (float) oscJson.getProperty("wave_frame", 0.0) / 255.0f;
    dest.unisonVoices = (int) oscJson.getProperty("unison_voices", 1);
    dest.unisonDetuneCents = (float) oscJson.getProperty("unison_detune", 0.0);
    dest.unisonSpread = (float) oscJson.getProperty("unison_blend", 0.5);

    dest.wavetable = decodeWavetableFromJson(oscJson);
}

WavetableData::Ptr VitalPresetParser::decodeWavetableFromJson(const juce::var& oscJson)
{
    auto wtJson = oscJson.getProperty("wavetable", juce::var());
    if (!wtJson.isObject())
        return nullptr;

    // Frames are typically base64-encoded raw float32 arrays, one per wavetable position.
    auto framesArray = wtJson.getProperty("frames", juce::var());
    if (!framesArray.isArray())
        return nullptr;

    auto* arr = framesArray.getArray();
    const int numFrames = arr->size();
    if (numFrames == 0)
        return nullptr;

    // Hash the raw base64 blob for de-dup in the shared wavetable pool.
    juce::String concatForHash;
    for (auto& f : *arr)
        concatForHash += f.toString();
    const juce::uint64 hash = (juce::uint64) concatForHash.hashCode64();

    return WavetablePool::instance().getOrCreate(hash, [&]() -> WavetableData::Ptr
    {
        auto wt = new WavetableData();
        wt->numFrames = numFrames;
        wt->frameSize = 2048; // Vital's default single-cycle frame length
        wt->numMipLevels = 4; // build a small mip chain for anti-aliasing

        wt->data.resize((size_t) wt->numMipLevels);
        for (auto& mipData : wt->data)
            mipData.resize((size_t) numFrames);

        for (int f = 0; f < numFrames; ++f)
        {
            juce::MemoryBlock raw;
            raw.fromBase64Encoding(arr->getUnchecked(f).toString());

            const int numSamples = juce::jmin(wt->frameSize, (int) (raw.getSize() / sizeof(float)));
            std::vector<int16_t> baseFrame((size_t) wt->frameSize, 0);
            auto* floatData = static_cast<const float*>(raw.getData());
            for (int s = 0; s < numSamples; ++s)
                baseFrame[(size_t) s] = (int16_t) juce::jlimit(-32768.0f, 32767.0f, floatData[s] * 32767.0f);

            // Mip 0 = full bandwidth. Higher mips = progressively low-pass filtered
            // copies (simple box-filter downsampling here; a production build
            // would use a proper windowed-sinc filter for cleaner harmonics).
            wt->data[0][(size_t) f] = baseFrame;
            for (int m = 1; m < wt->numMipLevels; ++m)
            {
                std::vector<int16_t> filtered(baseFrame.size());
                const int radius = 1 << m;
                for (size_t i = 0; i < baseFrame.size(); ++i)
                {
                    int32_t sum = 0;
                    for (int k = -radius; k <= radius; ++k)
                        sum += baseFrame[(i + (size_t) k + baseFrame.size()) % baseFrame.size()];
                    filtered[i] = (int16_t) (sum / (2 * radius + 1));
                }
                wt->data[(size_t) m][(size_t) f] = filtered;
            }
        }
        return wt;
    });
}

void VitalPresetParser::mapEnvelope(const juce::var& envJson, EnvelopeSettings& dest)
{
    dest.attack = (float) envJson.getProperty("attack", 0.01);
    dest.decay = (float) envJson.getProperty("decay", 0.3);
    dest.sustain = (float) envJson.getProperty("sustain", 0.7);
    dest.release = (float) envJson.getProperty("release", 0.3);
}

void VitalPresetParser::mapFilter(const juce::var& filterJson, FilterSettings& dest)
{
    dest.enabled = (bool) filterJson.getProperty("on", true);
    dest.cutoffHz = (float) filterJson.getProperty("cutoff", 2000.0);
    dest.resonance = (float) filterJson.getProperty("resonance", 0.2);

    const juce::String modeStr = filterJson.getProperty("style", "lowpass").toString();
    if (modeStr.containsIgnoreCase("high")) dest.type = FilterType::HighPass;
    else if (modeStr.containsIgnoreCase("band")) dest.type = FilterType::BandPass;
    else if (modeStr.containsIgnoreCase("notch")) dest.type = FilterType::Notch;
    else dest.type = FilterType::LowPass;
}
