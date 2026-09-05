#pragma once
#include <JuceHeader.h>

enum class LfoShape { Sine, Triangle, Saw, Square, SampleHold };

class LFO
{
public:
    void prepare(double sr) { sampleRate = sr; }
    void setShape(LfoShape s) { shape = s; }
    void setFrequency(float hz) { freq = hz; }
    void setRetrigger(bool shouldRetrigger) { retrigger = shouldRetrigger; }

    void noteOn() { if (retrigger) phase = 0.0; }

    float renderNextSample()
    {
        phase += freq / sampleRate;
        if (phase >= 1.0) phase -= 1.0;

        float value = 0.0f;
        switch (shape)
        {
            case LfoShape::Sine:     value = std::sin(juce::MathConstants<float>::twoPi * (float) phase); break;
            case LfoShape::Triangle: value = 1.0f - 4.0f * std::abs(juce::jmap((float) phase, 0.0f, 1.0f, -0.5f, 0.5f)); break;
            case LfoShape::Saw:      value = juce::jmap((float) phase, 0.0f, 1.0f, -1.0f, 1.0f); break;
            case LfoShape::Square:   value = phase < 0.5 ? 1.0f : -1.0f; break;
            case LfoShape::SampleHold:
                if (phase < lastPhase) heldValue = rng.nextFloat() * 2.0f - 1.0f;
                value = heldValue;
                break;
        }
        lastPhase = (float) phase;
        return value; // -1..1
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    float lastPhase = 0.0f;
    float freq = 1.0f;
    float heldValue = 0.0f;
    bool retrigger = true;
    LfoShape shape = LfoShape::Sine;
    juce::Random rng;
};
