#pragma once
#include <JuceHeader.h>

/**
    Dark Frutiger Aero theme: keeps the glossy glass/blue-green sheen of the
    classic 2007-era aesthetic but on a deep navy/charcoal base instead of
    bright white/sky-blue, so it's comfortable in a dim studio at 2am.
*/
namespace AeroColours
{
    static const juce::Colour background      { 0xff11161c }; // near-black navy
    static const juce::Colour panelGlass       { 0x332a4a5c }; // translucent steel-blue glass
    static const juce::Colour panelGlassBorder { 0x554fd1c7 }; // teal glass edge highlight
    static const juce::Colour accentTeal       { 0xff35d7c2 };
    static const juce::Colour accentBlue       { 0xff4fa3e3 };
    static const juce::Colour accentLime       { 0xff8ce04a };
    static const juce::Colour textPrimary      { 0xffe8f4f6 };
    static const juce::Colour textDim          { 0xff8fa3ad };
    static const juce::Colour knobShadow       { 0xff05080b };
}

class AeroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AeroLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    juce::Font getLabelFont(juce::Label&) override;
};
