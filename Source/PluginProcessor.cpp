#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets/PresetManager.h"

AeroSynthAudioProcessor::AeroSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Start from a simple init patch: single sine-ish table on osc 1, no unison,
    // so the plugin makes sound immediately even with no preset loaded yet.
    currentPreset = AeroPreset();
}

void AeroSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    engine.loadPreset(currentPreset);
}

bool AeroSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AeroSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            engine.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff(msg.getNoteNumber());
    }

    engine.renderBlock(buffer);
}

bool AeroSynthAudioProcessor::loadPresetFromFile(const juce::File& file)
{
    AeroPreset loaded;
    if (!PresetManager::loadPreset(file, loaded))
        return false;

    currentPreset = loaded;
    engine.loadPreset(currentPreset);
    return true;
}

void AeroSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Persist just enough for the host (FL Studio) to restore the sound:
    // preset name + core parameters as JSON, matching the native .aerosynth shape.
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty("name", currentPreset.name);
    obj->setProperty("sourceFormat", currentPreset.sourceFormat);
    obj->setProperty("masterVolume", currentPreset.masterVolume);
    juce::var v(obj.release());
    auto json = juce::JSON::toString(v);
    destData.replaceAll(json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void AeroSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto json = juce::JSON::parse(juce::String::createStringFromData(data, sizeInBytes));
    if (json.isObject())
    {
        currentPreset.name = json.getProperty("name", "Init").toString();
        currentPreset.masterVolume = (float) json.getProperty("masterVolume", 0.8);
        engine.loadPreset(currentPreset);
    }
}

juce::AudioProcessorEditor* AeroSynthAudioProcessor::createEditor()
{
    return new AeroSynthAudioProcessorEditor(*this);
}

// This creates the connection FL Studio (or any VST3 host) uses to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AeroSynthAudioProcessor();
}
