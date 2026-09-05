#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AeroLookAndFeel.h"
#include "UI/GlassPanel.h"

class AeroSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::FileDragAndDropTarget
{
public:
    explicit AeroSynthAudioProcessorEditor(AeroSynthAudioProcessor&);
    ~AeroSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void loadPresetViaDialog();

    AeroSynthAudioProcessor& processorRef;
    AeroLookAndFeel aeroLookAndFeel;
    juce::OpenGLContext openGLContext; // hardware-accelerated compositing for the glass blur/gradients

    GlassPanel oscPanel   { "Oscillators" };
    GlassPanel filterPanel{ "Filter" };
    GlassPanel envPanel   { "Envelope" };
    GlassPanel fxPanel    { "Effects" };

    juce::Slider wtPositionKnob, cutoffKnob, resonanceKnob, attackKnob, releaseKnob;
    juce::Label presetNameLabel;
    juce::TextButton loadPresetButton { "Load Preset..." };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AeroSynthAudioProcessorEditor)
};
