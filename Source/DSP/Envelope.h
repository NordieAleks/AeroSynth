#pragma once
#include <JuceHeader.h>

class Envelope
{
public:
    void prepare(double sr) { sampleRate = sr; }

    void setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec)
    {
        attack = juce::jmax(0.0005f, attackSec);
        decay = juce::jmax(0.0005f, decaySec);
        sustain = juce::jlimit(0.0f, 1.0f, sustainLevel);
        release = juce::jmax(0.0005f, releaseSec);
    }

    void noteOn()
    {
        stage = Stage::Attack;
        // start from current level for smooth re-trigger, not from zero
    }

    void noteOff() { stage = Stage::Release; releaseStartLevel = currentLevel; }

    bool isActive() const { return stage != Stage::Idle; }

    float renderNextSample()
    {
        const float attackRate = 1.0f / (attack * (float) sampleRate);
        const float decayRate = 1.0f / (decay * (float) sampleRate);
        const float releaseRate = 1.0f / (release * (float) sampleRate);

        switch (stage)
        {
            case Stage::Attack:
                currentLevel += attackRate;
                if (currentLevel >= 1.0f) { currentLevel = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                currentLevel -= decayRate * (1.0f - sustain);
                if (currentLevel <= sustain) { currentLevel = sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain:
                currentLevel = sustain;
                break;
            case Stage::Release:
                currentLevel -= releaseRate * releaseStartLevel;
                if (currentLevel <= 0.0f) { currentLevel = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Idle:
                currentLevel = 0.0f;
                break;
        }
        return currentLevel;
    }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Stage::Idle;

    double sampleRate = 44100.0;
    float attack = 0.01f, decay = 0.1f, sustain = 0.8f, release = 0.2f;
    float currentLevel = 0.0f;
    float releaseStartLevel = 0.0f;
};
