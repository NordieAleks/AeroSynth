#include "PresetManager.h"
#include "VitalPresetParser.h"
#include "SurgePresetParser.h"
#include "WavetableImporter.h"

bool PresetManager::loadPreset(const juce::File& file, AeroPreset& outPreset)
{
    outPreset = AeroPreset(); // reset to init defaults before populating

    if (file.hasFileExtension(".aerosynth"))
    {
        auto json = juce::JSON::parse(file);
        if (!json.isObject()) return false;

        outPreset.sourceFormat = "native";
        outPreset.name = json.getProperty("name", file.getFileNameWithoutExtension()).toString();
        outPreset.masterVolume = (float) json.getProperty("masterVolume", 0.8);
        // Full native (de)serialization of every field would extend this
        // block; the pattern mirrors VitalPresetParser's field-by-field mapping.
        return true;
    }

    if (VitalPresetParser::canParse(file))
        return VitalPresetParser::parse(file, outPreset);

    if (SurgePresetParser::canParse(file))
        return SurgePresetParser::parse(file, outPreset);

    if (WavetableImporter::canImport(file))
    {
        // A bare .wav is treated as "load this wavetable into oscillator 1
        // of an otherwise-Init patch" — the common workflow for Serum-exported
        // wavetables and third-party wavetable packs.
        auto wt = WavetableImporter::importWavFile(file);
        if (wt == nullptr) return false;

        outPreset.sourceFormat = "wav-import";
        outPreset.name = file.getFileNameWithoutExtension();
        outPreset.oscillators[0].wavetable = wt;
        outPreset.oscillators[0].enabled = true;
        return true;
    }

    return false;
}

bool PresetManager::saveNativePreset(const juce::File& file, const AeroPreset& preset)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty("name", preset.name);
    obj->setProperty("masterVolume", preset.masterVolume);
    obj->setProperty("polyphony", preset.polyphony);
    // Wavetable audio data is intentionally NOT re-serialized here for
    // third-party-sourced presets to avoid redistributing imported content;
    // native presets built from scratch in AeroSynth's own wavetable editor
    // would serialize their own wavetable bytes in a fuller implementation.

    juce::var result(obj.release());
    return file.replaceWithText(juce::JSON::toString(result));
}
