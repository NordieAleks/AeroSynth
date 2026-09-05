#pragma once
#include <JuceHeader.h>
#include "PresetTypes.h"

/**
    Parses Surge XT patches (.fxp). Surge is fully open-source (GPL-3.0,
    https://github.com/surge-synthesizer/surge), so unlike Serum this is not
    a reverse-engineering situation — the patch structure is published in
    Surge's own source (`src/common/patch-serialization/`). Surge wraps an
    XML patch document inside a standard VST2 .fxp chunk container.

    NOTE: Surge's XML schema has changed across major versions (patch format
    revision number is stored in the XML root). Treat the tag names below as
    a starting point and diff them against the Surge version you're targeting
    before shipping — this parser only needs the oscillator/filter/envelope
    section, not Surge's full modulation scene complexity.
*/
class SurgePresetParser
{
public:
    static bool canParse(const juce::File& file);
    static bool parse(const juce::File& file, AeroPreset& outPreset);

private:
    static juce::MemoryBlock extractXmlChunkFromFxp(const juce::File& file);
    static void mapOscillator(const juce::XmlElement* oscXml, OscillatorSettings& dest);
    static void mapFilter(const juce::XmlElement* filterXml, FilterSettings& dest);
    static void mapEnvelope(const juce::XmlElement* envXml, EnvelopeSettings& dest);
};
