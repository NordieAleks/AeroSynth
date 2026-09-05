#include "SurgePresetParser.h"

bool SurgePresetParser::canParse(const juce::File& file)
{
    if (!file.hasFileExtension(".fxp"))
        return false;

    // Cheap sniff test: Surge's XML chunk contains a recognizable root tag.
    auto text = file.loadFileAsString();
    return text.contains("<patch ") || text.contains("<patch>");
}

juce::MemoryBlock SurgePresetParser::extractXmlChunkFromFxp(const juce::File& file)
{
    // Standard VST2 .fxp "chunk" presets store an opaque data blob after a
    // fixed-size header. Surge puts its XML directly in that chunk, so for
    // Surge patches we can usually locate the "<patch" tag directly in the
    // raw bytes without fully parsing the FXP header structure.
    juce::MemoryBlock raw;
    file.loadFileAsData(raw);

    const char* bytes = static_cast<const char*>(raw.getData());
    const size_t size = raw.getSize();
    const char* needle = "<patch";
    const size_t needleLen = 6;

    for (size_t i = 0; i + needleLen < size; ++i)
    {
        if (std::memcmp(bytes + i, needle, needleLen) == 0)
        {
            juce::MemoryBlock xmlChunk;
            xmlChunk.append(bytes + i, size - i);
            return xmlChunk;
        }
    }
    return {};
}

bool SurgePresetParser::parse(const juce::File& file, AeroPreset& outPreset)
{
    auto xmlChunk = extractXmlChunkFromFxp(file);
    if (xmlChunk.getSize() == 0)
        return false;

    auto xmlText = juce::String::fromUTF8(static_cast<const char*>(xmlChunk.getData()), (int) xmlChunk.getSize());
    std::unique_ptr<juce::XmlElement> root(juce::XmlDocument::parse(xmlText));
    if (root == nullptr)
        return false;

    outPreset.sourceFormat = "surge";
    outPreset.name = root->getStringAttribute("name", file.getFileNameWithoutExtension());

    if (auto* scene = root->getChildByName("scene"))
    {
        int oscIndex = 0;
        int envIndex = 0;
        for (auto* child = scene->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            if (child->hasTagName("osc") && oscIndex < 3)
            {
                mapOscillator(child, outPreset.oscillators[(size_t) oscIndex]);
                ++oscIndex;
            }
            else if (child->hasTagName("adsr") && envIndex < 2)
            {
                mapEnvelope(child, outPreset.envelopes[(size_t) envIndex]);
                ++envIndex;
            }
        }

        if (auto* filterXml = scene->getChildByName("filter"))
            mapFilter(filterXml, outPreset.filter);
    }

    return true;
}

void SurgePresetParser::mapOscillator(const juce::XmlElement* oscXml, OscillatorSettings& dest)
{
    if (oscXml == nullptr) return;
    dest.enabled = true;
    dest.level = (float) oscXml->getDoubleAttribute("level", 0.8);
    dest.coarseTuneSemitones = (float) oscXml->getDoubleAttribute("pitch", 0.0);
    // Surge oscillators are procedural (wavetable/wavetable-osc/FM/etc.) rather
    // than always sample-based; a full port would map Surge's oscillator
    // "type" attribute to an equivalent internal generator. For wavetable-type
    // Surge oscillators, the referenced .wt file should be loaded separately
    // via WavetableImporter and attached here.
}

void SurgePresetParser::mapFilter(const juce::XmlElement* filterXml, FilterSettings& dest)
{
    if (filterXml == nullptr) return;
    dest.cutoffHz = (float) juce::jmap(filterXml->getDoubleAttribute("cutoff", 60.0), -60.0, 70.0, 20.0, 20000.0);
    dest.resonance = (float) filterXml->getDoubleAttribute("resonance", 0.2);
}

void SurgePresetParser::mapEnvelope(const juce::XmlElement* envXml, EnvelopeSettings& dest)
{
    if (envXml == nullptr) return;
    dest.attack = (float) envXml->getDoubleAttribute("a", 0.01);
    dest.decay = (float) envXml->getDoubleAttribute("d", 0.3);
    dest.sustain = (float) envXml->getDoubleAttribute("s", 0.7);
    dest.release = (float) envXml->getDoubleAttribute("r", 0.3);
}
