#include "WavetableImporter.h"

int WavetableImporter::readClmChunkFrameSize(const juce::File& file)
{
    // Many wavetable .wav files carry a "clm " RIFF chunk with a single
    // uint32 declaring the frame size in samples. We scan for it manually
    // since JUCE's WavAudioFormat doesn't surface arbitrary chunks by default.
    juce::MemoryBlock raw;
    file.loadFileAsData(raw);
    const char* bytes = static_cast<const char*>(raw.getData());
    const size_t size = raw.getSize();

    for (size_t i = 0; i + 8 < size; ++i)
    {
        if (bytes[i] == 'c' && bytes[i + 1] == 'l' && bytes[i + 2] == 'm' && bytes[i + 3] == ' ')
        {
            juce::uint32 frameSize = 0;
            std::memcpy(&frameSize, bytes + i + 8, sizeof(juce::uint32));
            if (frameSize > 0 && frameSize <= 16384)
                return (int) frameSize;
        }
    }
    return 0;
}

int WavetableImporter::detectFrameSize(int totalSamples, int explicitFrameSize)
{
    if (explicitFrameSize > 0)
        return explicitFrameSize;

    // Common single-cycle frame sizes used by Serum/Vital/WaveEdit exports.
    const int candidates[] = { 2048, 4096, 1024, 512, 2000, 3000 };
    for (int c : candidates)
        if (totalSamples % c == 0 && totalSamples / c >= 1 && totalSamples / c <= 512)
            return c;

    // Fall back: treat the whole file as a single frame.
    return totalSamples;
}

WavetableData::Ptr WavetableImporter::importWavFile(const juce::File& file, int frameSizeHint)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return nullptr;

    const int totalSamples = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> buffer(1, totalSamples);
    reader->read(&buffer, 0, totalSamples, 0, true, false);

    int frameSize = frameSizeHint > 0 ? frameSizeHint : readClmChunkFrameSize(file);
    frameSize = detectFrameSize(totalSamples, frameSize);
    const int numFrames = juce::jmax(1, totalSamples / frameSize);

    const juce::uint64 hash = (juce::uint64) file.hashCode64() ^ (juce::uint64) totalSamples;

    return WavetablePool::instance().getOrCreate(hash, [&]() -> WavetableData::Ptr
    {
        auto wt = new WavetableData();
        wt->frameSize = frameSize;
        wt->numFrames = numFrames;
        wt->numMipLevels = 4;
        wt->data.resize((size_t) wt->numMipLevels);
        for (auto& mipData : wt->data)
            mipData.resize((size_t) numFrames);

        const float* src = buffer.getReadPointer(0);
        for (int f = 0; f < numFrames; ++f)
        {
            std::vector<int16_t> baseFrame((size_t) frameSize, 0);
            for (int s = 0; s < frameSize; ++s)
            {
                const int idx = f * frameSize + s;
                const float v = idx < totalSamples ? src[idx] : 0.0f;
                baseFrame[(size_t) s] = (int16_t) juce::jlimit(-32768.0f, 32767.0f, v * 32767.0f);
            }
            wt->data[0][(size_t) f] = baseFrame;

            for (int m = 1; m < wt->numMipLevels; ++m)
            {
                std::vector<int16_t> filtered(baseFrame.size());
                const int radius = 1 << m;
                for (size_t i = 0; i < baseFrame.size(); ++i)
                {
                    int32_t sum = 0;
                    for (int k = -radius; k <= radius; ++k)
                        sum += baseFrame[(i + (size_t) k + baseFrame.size()) % baseFrame.size()];
                    filtered[i] = (int16_t) (sum / (2 * radius + 1));
                }
                wt->data[(size_t) m][(size_t) f] = filtered;
            }
        }
        return wt;
    });
}
