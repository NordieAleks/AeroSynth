#pragma once
#include <JuceHeader.h>
#include "PresetTypes.h"

/** Detects a preset file's format and routes it to the correct parser, or loads a native .aerosynth preset. */
class PresetManager
{
public:
    /** Returns true and fills outPreset on success. Supported: .vital, .fxp (Surge), .wav (wavetable-only import), .aerosynth (native). */
    static bool loadPreset(const juce::File& file, AeroPreset& outPreset);

    static bool saveNativePreset(const juce::File& file, const AeroPreset& preset);

    static juce::StringArray getSupportedExtensions() { return { "*.aerosynth", "*.vital", "*.fxp", "*.wav" }; }
};
