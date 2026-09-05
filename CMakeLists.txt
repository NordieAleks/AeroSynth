cmake_minimum_required(VERSION 3.22)

project(AeroSynth VERSION 0.1.0)

# ---------------------------------------------------------------------------
# This project depends on JUCE. Point FETCHCONTENT or add_subdirectory at your
# local JUCE checkout. Two supported ways to satisfy this:
#
#   1) git submodule:  git submodule add https://github.com/juce-framework/JUCE.git JUCE
#   2) FetchContent (uncomment block below) if you have network access when building.
# ---------------------------------------------------------------------------

# --- Option A: local JUCE checkout in ./JUCE ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    # --- Option B: fetch JUCE at configure time (needs network) ---
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(AeroSynth
    COMPANY_NAME "YourStudioName"
    PLUGIN_MANUFACTURER_CODE Aero
    PLUGIN_CODE Aer1
    FORMATS VST3
    PRODUCT_NAME "AeroSynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(AeroSynth)

target_sources(AeroSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/SynthEngine.cpp
    Source/Presets/VitalPresetParser.cpp
    Source/Presets/WavetableImporter.cpp
    Source/Presets/SurgePresetParser.cpp
    Source/Presets/PresetManager.cpp
    Source/UI/AeroLookAndFeel.cpp
    Source/UI/GlassPanel.cpp
)

target_compile_definitions(AeroSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(AeroSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
