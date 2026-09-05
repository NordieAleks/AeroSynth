#include "PluginEditor.h"

AeroSynthAudioProcessorEditor::AeroSynthAudioProcessorEditor(AeroSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&aeroLookAndFeel);

    // GPU-accelerated rendering: keeps the blurred glass panels and gradients
    // cheap even with many decorative elements on screen at once.
    openGLContext.attachTo(*this);

    addAndMakeVisible(oscPanel);
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(envPanel);
    addAndMakeVisible(fxPanel);

    for (auto* knob : { &wtPositionKnob, &cutoffKnob, &resonanceKnob, &attackKnob, &releaseKnob })
    {
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob->setRange(0.0, 1.0, 0.001);
        addAndMakeVisible(*knob);
    }

    cutoffKnob.setRange(20.0, 20000.0, 1.0);
    cutoffKnob.setSkewFactorFromMidPoint(2000.0);
    cutoffKnob.setValue(processorRef.getCurrentPreset().filter.cutoffHz);

    presetNameLabel.setText(processorRef.getCurrentPreset().name, juce::dontSendNotification);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setColour(juce::Label::textColourId, AeroColours::accentTeal);
    addAndMakeVisible(presetNameLabel);

    loadPresetButton.onClick = [this] { loadPresetViaDialog(); };
    addAndMakeVisible(loadPresetButton);

    setSize(720, 480);
}

AeroSynthAudioProcessorEditor::~AeroSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    openGLContext.detach();
}

void AeroSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark navy-to-charcoal backdrop rather than bright Aero sky-blue.
    juce::ColourGradient bg(juce::Colour(0xff0c1014), 0.0f, 0.0f,
                             juce::Colour(0xff161d24), 0.0f, (float) getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // A couple of soft, low-opacity teal/blue "bokeh" glows for the Aero nature-tech feel,
    // dimmed way down from the classic bright version so it stays easy on the eyes at night.
    g.setColour(AeroColours::accentTeal.withAlpha(0.05f));
    g.fillEllipse(-100.0f, -100.0f, 300.0f, 300.0f);
    g.setColour(AeroColours::accentBlue.withAlpha(0.04f));
    g.fillEllipse((float) getWidth() - 250.0f, (float) getHeight() - 200.0f, 350.0f, 350.0f);
}

void AeroSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);

    auto header = area.removeFromTop(36);
    loadPresetButton.setBounds(header.removeFromLeft(140));
    presetNameLabel.setBounds(header);

    area.removeFromTop(12);

    auto oscArea = area.removeFromTop(140);
    oscPanel.setBounds(oscArea);
    wtPositionKnob.setBounds(oscArea.reduced(20).removeFromLeft(90).removeFromTop(90));

    area.removeFromTop(12);
    auto filterArea = area.removeFromTop(140);
    filterPanel.setBounds(filterArea);
    auto filterKnobs = filterArea.reduced(20).removeFromTop(90);
    cutoffKnob.setBounds(filterKnobs.removeFromLeft(90));
    filterKnobs.removeFromLeft(20);
    resonanceKnob.setBounds(filterKnobs.removeFromLeft(90));

    area.removeFromTop(12);
    auto envArea = area.removeFromTop(120);
    envPanel.setBounds(envArea);
    auto envKnobs = envArea.reduced(20).removeFromTop(80);
    attackKnob.setBounds(envKnobs.removeFromLeft(80));
    envKnobs.removeFromLeft(20);
    releaseKnob.setBounds(envKnobs.removeFromLeft(80));

    fxPanel.setBounds(area);
}

bool AeroSynthAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        juce::File file(f);
        if (file.hasFileExtension(".vital;.fxp;.wav;.aerosynth"))
            return true;
    }
    return false;
}

void AeroSynthAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
    {
        if (processorRef.loadPresetFromFile(juce::File(f)))
        {
            presetNameLabel.setText(processorRef.getCurrentPreset().name, juce::dontSendNotification);
            cutoffKnob.setValue(processorRef.getCurrentPreset().filter.cutoffHz, juce::dontSendNotification);
            break;
        }
    }
}

void AeroSynthAudioProcessorEditor::loadPresetViaDialog()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load a preset (.vital, .fxp, .wav, .aerosynth)",
        juce::File(),
        "*.vital;*.fxp;*.wav;*.aerosynth");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile() && processorRef.loadPresetFromFile(file))
            {
                presetNameLabel.setText(processorRef.getCurrentPreset().name, juce::dontSendNotification);
                cutoffKnob.setValue(processorRef.getCurrentPreset().filter.cutoffHz, juce::dontSendNotification);
            }
        });
}
