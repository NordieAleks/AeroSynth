#pragma once
#include <JuceHeader.h>
#include "PresetTypes.h"

/**
    Parses Vital's .vital preset files. Vital's format is an open, plain-text
    JSON document (this is one of the reasons Vital is the easiest and most
    legally straightforward third-party format to support) — wavetables are
    embedded as base64-encoded frame arrays inside the "groups"/"oscillators"
    structure.

    NOTE: Vital's exact JSON key names have evolved across versions and are
    not formally published as a versioned spec. The mapping below targets the
    commonly-seen structure; if Vital ships a schema change, only this parser
    needs updating — the rest of the engine is unaffected because it only
    consumes the unified AeroPreset struct.
*/
class VitalPresetParser
{
public:
    static bool canParse(const juce::File& file);
    static bool parse(const juce::File& file, AeroPreset& outPreset);

private:
    static WavetableData::Ptr decodeWavetableFromJson(const juce::var& oscJson);
    static void mapOscillator(const juce::var& oscJson, OscillatorSettings& dest);
    static void mapEnvelope(const juce::var& envJson, EnvelopeSettings& dest);
    static void mapFilter(const juce::var& filterJson, FilterSettings& dest);
};
