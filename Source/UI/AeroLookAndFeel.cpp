#include "AeroLookAndFeel.h"

AeroLookAndFeel::AeroLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, AeroColours::background);
    setColour(juce::Label::textColourId, AeroColours::textPrimary);
    setColour(juce::Slider::thumbColourId, AeroColours::accentTeal);
    setColour(juce::ComboBox::backgroundColourId, AeroColours::panelGlass);
    setColour(juce::ComboBox::textColourId, AeroColours::textPrimary);
}

void AeroLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Drop shadow (glass sits slightly "above" the panel)
    g.setColour(AeroColours::knobShadow.withAlpha(0.6f));
    g.fillEllipse(bounds.translated(0, 2.0f));

    // Glass body: dark radial gradient, subtle teal rim
    juce::ColourGradient body(AeroColours::panelGlass.brighter(0.15f), centre.x, centre.y - radius * 0.4f,
                               AeroColours::panelGlass.darker(0.6f), centre.x, centre.y + radius, false);
    g.setGradientFill(body);
    g.fillEllipse(bounds);

    g.setColour(AeroColours::panelGlassBorder);
    g.drawEllipse(bounds, 1.2f);

    // Glossy highlight (the classic Aero "light from upper-left" hotspot)
    auto highlightArea = bounds.reduced(radius * 0.35f).translated(-radius * 0.15f, -radius * 0.3f);
    juce::ColourGradient gloss(juce::Colours::white.withAlpha(0.28f), highlightArea.getCentreX(), highlightArea.getY(),
                                juce::Colours::white.withAlpha(0.0f), highlightArea.getCentreX(), highlightArea.getBottom(), false);
    g.setGradientFill(gloss);
    g.fillEllipse(highlightArea);

    // Value arc in accent teal, track in dim glass-blue
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius * 0.92f, radius * 0.92f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(AeroColours::panelGlassBorder.withAlpha(0.4f));
    g.strokePath(track, juce::PathStrokeType(2.5f));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius * 0.92f, radius * 0.92f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(AeroColours::accentTeal);
    g.strokePath(valueArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointer;
    pointer.addRectangle(-1.5f, -radius * 0.85f, 3.0f, radius * 0.4f);
    g.setColour(AeroColours::textPrimary);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre));
}

void AeroLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const float corner = 8.0f;

    juce::Colour base = AeroColours::panelGlass;
    if (isDown) base = base.darker(0.3f);
    else if (isHighlighted) base = base.brighter(0.2f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, corner);

    // top sheen
    auto sheen = bounds.removeFromTop(bounds.getHeight() * 0.45f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(sheen, corner);

    g.setColour(AeroColours::panelGlassBorder);
    g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(1.0f), corner, 1.0f);
}

void AeroLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(1.0f);
    g.setColour(AeroColours::panelGlass);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(AeroColours::panelGlassBorder);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

juce::Font AeroLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(14.0f)).withExtraKerningFactor(0.02f);
}
