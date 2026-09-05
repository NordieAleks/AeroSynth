#pragma once
#include <JuceHeader.h>

/** Wraps juce::dsp modules with a small, fixed memory budget (no dynamic delay-line growth at runtime). */
class EffectsChain
{
public:
    void prepare(double sr, int blockSize)
    {
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };

        chorus.prepare(spec);
        chorus.setRate(0.8f);
        chorus.setDepth(0.25f);
        chorus.setCentreDelay(7.0f);
        chorus.setFeedback(0.1f);
        chorus.setMix(0.0f); // off by default, preset enables it

        delay.prepare(spec);
        delay.setMaximumDelayInSamples((int) (sr * 2.0)); // cap at 2s to bound memory

        reverb.prepare(spec);
        reverbParams.roomSize = 0.5f;
        reverbParams.wetLevel = 0.0f; // off by default
        reverb.setParameters(reverbParams);

        distortionDrive = 1.0f;
    }

    void setChorusMix(float mix01) { chorus.setMix(mix01); }
    void setDelay(float mixLevel, float timeSeconds, float feedback01, double sr)
    {
        delayMix = mixLevel;
        delayFeedback = juce::jlimit(0.0f, 0.95f, feedback01);
        delay.setDelay((float) (timeSeconds * sr));
    }
    void setReverbMix(float wet01) { reverbParams.wetLevel = wet01; reverbParams.dryLevel = 1.0f - wet01; reverb.setParameters(reverbParams); }
    void setDistortionDrive(float drive) { distortionDrive = juce::jmax(1.0f, drive); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        // Distortion (soft clip) - cheap waveshaper, no oversampling unless drive is high
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
                data[i] = std::tanh(data[i] * distortionDrive) / std::tanh(distortionDrive);
        }

        juce::dsp::ProcessContextReplacing<float> ctx(block);
        chorus.process(ctx);

        // simple feedback delay mixed in-place
        if (delayMix > 0.0001f)
        {
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            {
                auto* data = block.getChannelPointer(ch);
                for (size_t i = 0; i < block.getNumSamples(); ++i)
                {
                    const float dry = data[i];
                    const float wet = delay.popSample((int) ch);
                    delay.pushSample((int) ch, dry + wet * delayFeedback);
                    data[i] = dry + wet * delayMix;
                }
            }
        }

        reverb.process(ctx);
    }

private:
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float> delay { 96000 };
    float delayMix = 0.0f, delayFeedback = 0.3f;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    float distortionDrive = 1.0f;
};
