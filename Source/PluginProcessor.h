#pragma once
#include <JuceHeader.h>
#include "DSP/SynthEngine.h"
#include "Presets/PresetTypes.h"

class AeroSynthAudioProcessor : public juce::AudioProcessor
{
public:
    AeroSynthAudioProcessor();
    ~AeroSynthAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AeroSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; } // matches EffectsChain's capped delay/reverb tail

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    /** Called from the editor when the user picks a preset file (any supported format). */
    bool loadPresetFromFile(const juce::File& file);

    SynthEngine& getEngine() { return engine; }
    const AeroPreset& getCurrentPreset() const { return currentPreset; }

private:
    SynthEngine engine;
    AeroPreset currentPreset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AeroSynthAudioProcessor)
};
