#pragma once
#include <JuceHeader.h>
#include <array>
#include <unordered_map>

enum class ModSource { Env1, Env2, Lfo1, Lfo2, Lfo3, Velocity, ModWheel, Aftertouch, NumSources };
enum class ModDest
{
    Osc1WtPos, Osc2WtPos, Osc3WtPos,
    Osc1Pitch, Osc2Pitch, Osc3Pitch,
    FilterCutoff, FilterResonance,
    OscMixBalance, MasterVolume,
    NumDests
};

struct ModRoute
{
    ModSource source;
    ModDest dest;
    float amount = 0.0f; // -1..1
    bool bipolar = true;
};

/**
    Lightweight mod matrix: sources are rendered once per sample by the voice,
    then summed into destinations according to the active routes. No dynamic
    allocation in the audio-processing path — routes are a fixed-size vector
    sized at preset-load time only.
*/
class ModulationMatrix
{
public:
    void setRoutes(std::vector<ModRoute> newRoutes) { routes = std::move(newRoutes); }

    void setSourceValue(ModSource src, float value01Bipolar)
    {
        sourceValues[(size_t) src] = value01Bipolar;
    }

    /** Returns summed modulation amount (-N..N range, caller scales/clamps per destination). */
    float getModulationFor(ModDest dest) const
    {
        float sum = 0.0f;
        for (const auto& r : routes)
            if (r.dest == dest)
                sum += sourceValues[(size_t) r.source] * r.amount;
        return sum;
    }

private:
    std::array<float, (size_t) ModSource::NumSources> sourceValues {};
    std::vector<ModRoute> routes;
};
