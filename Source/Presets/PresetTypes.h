#pragma once
#include <JuceHeader.h>
#include "../DSP/WavetableOscillator.h"
#include "../DSP/ModulationMatrix.h"
#include "../DSP/LFO.h"
#include "../DSP/Filter.h"

/**
    Unified internal preset representation.
    Every format-specific parser (Vital JSON, Surge, raw wavetable .wav import,
    native .aerosynth) converts into THIS struct. The DSP engine only ever
    reads AeroPreset — it never needs to know where a patch came from.
*/
struct OscillatorSettings
{
    WavetableData::Ptr wavetable;
    float level = 0.8f;
    float pan = 0.0f;
    float coarseTuneSemitones = 0.0f;
    float fineTuneCents = 0.0f;
    float wavetablePosition = 0.0f;
    int unisonVoices = 1;
    float unisonDetuneCents = 0.0f;
    float unisonSpread = 0.5f;
    bool enabled = true;
};

struct FilterSettings
{
    FilterType type = FilterType::LowPass;
    float cutoffHz = 2000.0f;
    float resonance = 0.2f;
    bool enabled = true;
};

struct EnvelopeSettings
{
    float attack = 0.01f, decay = 0.3f, sustain = 0.7f, release = 0.3f;
};

struct LfoSettings
{
    LfoShape shape = LfoShape::Sine;
    float frequencyHz = 2.0f;
    bool retrigger = true;
};

struct EffectSettings
{
    float chorusMix = 0.0f;
    float delayMix = 0.0f;
    float delayTimeSec = 0.3f;
    float delayFeedback = 0.3f;
    float reverbMix = 0.0f;
    float distortionDrive = 1.0f;
};

struct AeroPreset
{
    juce::String name = "Init";
    juce::String sourceFormat = "native"; // "native" | "vital" | "surge" | "wav-import"

    std::array<OscillatorSettings, 3> oscillators;
    std::array<EnvelopeSettings, 2> envelopes;
    std::array<LfoSettings, 3> lfos;
    FilterSettings filter;
    EffectSettings effects;
    std::vector<ModRoute> modRoutes;

    float masterVolume = 0.8f;
    int polyphony = 16;
};
