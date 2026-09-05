#pragma once
#include <JuceHeader.h>

enum class FilterType { LowPass, HighPass, BandPass, Notch };

/** Zero-delay-feedback state-variable filter (cheap, stable, good for modulated cutoff). */
class MultiModeFilter
{
public:
    void prepare(double sr) { sampleRate = sr; reset(); }
    void reset() { s1 = s2 = 0.0f; }

    void setParameters(FilterType t, float cutoffHz, float resonance01)
    {
        type = t;
        cutoff = juce::jlimit(20.0f, (float) sampleRate * 0.49f, cutoffHz);
        resonance = juce::jlimit(0.0f, 0.99f, resonance01);
    }

    float processSample(float in)
    {
        const float g = std::tan(juce::MathConstants<float>::pi * cutoff / (float) sampleRate);
        const float k = 2.0f - 2.0f * resonance; // damping
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = in - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;

        switch (type)
        {
            case FilterType::LowPass:  return v2;
            case FilterType::HighPass: return in - k * v1 - v2;
            case FilterType::BandPass: return v1;
            case FilterType::Notch:    return in - k * v1;
        }
        return v2;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 1000.0f, resonance = 0.2f;
    float s1 = 0.0f, s2 = 0.0f;
    FilterType type = FilterType::LowPass;
};
