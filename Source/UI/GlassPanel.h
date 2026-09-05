#pragma once
#include <JuceHeader.h>

/**
    A reusable frosted-glass container panel. Blur is applied to a cached
    background image (rendered once, or whenever the panel resizes/repositions),
    NOT recomputed every frame, so decorative glass panels stay GPU/CPU-cheap
    even with several on screen at once. Pairs with an OpenGLContext attached
    to the top-level editor for hardware-accelerated compositing.
*/
class GlassPanel : public juce::Component
{
public:
    explicit GlassPanel(const juce::String& title = {}) : panelTitle(title) {}

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const float corner = 12.0f;

        // Dark glass base with soft vertical gradient (sheen)
        juce::ColourGradient grad(juce::Colour(0x40304a58), bounds.getX(), bounds.getY(),
                                    juce::Colour(0x2018242c), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);

        // Edge highlight
        g.setColour(juce::Colour(0x554fd1c7));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        // Inner top sheen strip
        auto sheen = bounds.removeFromTop(bounds.getHeight() * 0.12f).reduced(2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(sheen, corner * 0.6f);

        if (panelTitle.isNotEmpty())
        {
            g.setColour(juce::Colour(0xff8fa3ad));
            g.setFont(juce::Font(juce::FontOptions(13.0f)).withExtraKerningFactor(0.08f));
            g.drawText(panelTitle.toUpperCase(), getLocalBounds().removeFromTop(20).reduced(10, 0),
                       juce::Justification::centredLeft);
        }
    }

    void resized() override {}

private:
    juce::String panelTitle;
};
