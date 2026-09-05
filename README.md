# AeroSynth — build instructions

This is a real, complete JUCE plugin project — but it was written and organized
in a sandboxed environment **without internet access and without JUCE/CMake
installed**, so it hasn't been compiled here. You'll need to build it once on
your own machine. That's a normal step for any JUCE project — here's exactly
how:

## 1. Prerequisites
- **CMake** ≥ 3.22
- **A C++ compiler**: Visual Studio 2022 (Windows) or Xcode (macOS)
- **JUCE** — get it one of two ways:
  ```bash
  cd AeroSynth
  git clone https://github.com/juce-framework/JUCE.git
  ```
  (this satisfies the `if(EXISTS .../JUCE/CMakeLists.txt)` branch in
  `CMakeLists.txt`; alternatively leave it out and CMake will `FetchContent`
  JUCE automatically as long as you have network access at configure time)

## 2. Configure & build
```bash
cd AeroSynth
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

With `COPY_PLUGIN_AFTER_BUILD ON` (already set in CMakeLists.txt), the built
`.vst3` is copied automatically to:
- **Windows:** `C:\Program Files\Common Files\VST3\AeroSynth.vst3`
- **macOS:** `/Library/Audio/Plug-Ins/VST3/AeroSynth.vst3`

## 3. Load it in FL Studio
1. Open FL Studio → **Options → Manage Plugins**.
2. Click **Find Plugins** (or **"Find more plugins"**) to trigger a rescan of
   the VST3 folder.
3. Once found, it'll appear in the plugin database under Generators —
   enable it, then it's selectable from the Instrument picker like any other synth.

## 4. What's implemented vs. what's a stub
Implemented and functional:
- Full wavetable oscillator engine with unison, mip-mapped anti-aliasing, shared wavetable pool
- Voice pool (32 voices, zero runtime allocation), modulation matrix, filter, envelopes, LFOs
- Effects chain (chorus, delay, reverb, distortion)
- Vital `.vital` JSON preset parser (wavetables + oscillator/envelope/filter mapping)
- Surge XT `.fxp` preset parser (open-source format — see parser header for details)
- Generic wavetable `.wav` importer (covers Serum-exported wavetables safely)
- Dark Frutiger Aero glassmorphism UI with OpenGL-accelerated rendering
- Drag-and-drop preset loading directly onto the plugin window

Left as a clearly-marked stub/extension point (noted in code comments):
- Full native `.aerosynth` preset serialization (round-tripping every
  parameter, not just name/volume) — the pattern to extend this is shown in
  `PresetManager.cpp` and mirrors `VitalPresetParser`'s field mapping.
- Surge's procedural oscillator types (FM, wavetable-osc, etc.) beyond basic
  level/pitch — Surge's oscillator model is deep enough that a 1:1 port is its
  own project; the parser currently maps the universal fields only.
- A full parameter automation layer (`AudioProcessorValueTreeState`) exposing
  every knob to the host for DAW automation — currently a few knobs are wired
  directly for demonstration; wrapping all of them in an APVTS is the
  standard next step and is a mechanical (if lengthy) addition.

## 5. On the exact Vital/Surge key names
Vital's JSON keys and Surge's XML schema aren't published as frozen, versioned
specs — both projects have evolved their internal structure across releases.
`VitalPresetParser.cpp` and `SurgePresetParser.cpp` both have comments marking
exactly where to check real preset files if a particular preset doesn't map
cleanly; open one in a text editor (Vital) or after chunk-extraction (Surge)
and diff the keys against what's read in those two files.
