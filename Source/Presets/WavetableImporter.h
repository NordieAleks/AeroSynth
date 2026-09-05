#pragma once
#include <JuceHeader.h>
#include "../DSP/WavetableOscillator.h"

/**
    Imports plain wavetable audio files — the format-agnostic, legally clean
    path for "Serum sound compatibility":

      * Serum can export any oscillator's wavetable as a standard multi-cycle
        .wav file (File > Export Wavetable). This is just PCM audio, no
        proprietary structure involved.
      * WaveEdit / most wavetable packs ship as plain .wav too, usually with
        frames of 2048 or 4096 samples concatenated back-to-back, sometimes
        with a `clm ` / `srge` RIFF chunk declaring frame size explicitly
        (Surge and Serum both understand this chunk; if present we read it,
        otherwise we auto-detect via total-length / common frame sizes).
*/
class WavetableImporter
{
public:
    static bool canImport(const juce::File& file)
    {
        return file.hasFileExtension(".wav");
    }

    /** Reads a multi-frame wavetable .wav into a shared WavetableData. */
    static WavetableData::Ptr importWavFile(const juce::File& file, int frameSizeHint = 0);

private:
    static int detectFrameSize(int totalSamples, int explicitFrameSize);
    static int readClmChunkFrameSize(const juce::File& file);
};
