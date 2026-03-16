cmake_minimum_required(VERSION 3.22)

project(HowlingWolves VERSION 1.0.19)

# Initialize JUCE 
# We'll use a recent stable version of JUCE 7
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.3
)
FetchContent_MakeAvailable(JUCE)

# Define the JUCE plugin
juce_add_plugin(HowlingWolves
    COMPANY_NAME "WolfInstruments"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE WolF
    PLUGIN_CODE Hwl2
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Howling Wolves"
    ICON_BIG "Resources/logo_icon.png"
)

juce_generate_juce_header(HowlingWolves)


# Add source files
target_sources(HowlingWolves
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SynthEngine.cpp
        Source/SynthEngine.h
        Source/TransientShaper.cpp
        Source/TransientShaper.h
        Source/SampleManager.cpp
        Source/SampleManager.h
        Source/PresetManager.cpp
        Source/PresetManager.h
        Source/PresetBrowser.cpp
        Source/PresetBrowser.h
        Source/PlayTab.cpp
        Source/PlayTab.h
        Source/ModulateTab.cpp
        Source/ModulateTab.h
        Source/EffectsTab.cpp
        Source/EffectsTab.h
        Source/EffectsProcessor.cpp
        Source/EffectsProcessor.h
        Source/MidiProcessor.cpp
        Source/MidiProcessor.h
        Source/PerformTab.cpp
        Source/PerformTab.h
        Source/PianoRollComponent.cpp
        Source/PianoRollComponent.h
        Source/MidiCapturer.cpp
        Source/MidiCapturer.h
        Source/MidiDragComponent.cpp
        Source/MidiDragComponent.h
        Source/DrumTab.cpp
        Source/DrumTab.h
        Source/SettingsTab.cpp
        Source/SettingsTab.h
        Source/VisualizerComponent.cpp
        Source/VisualizerComponent.h
        Source/CustomKnobLookAndFeel.cpp
        Source/CustomKnobLookAndFeel.h
        Source/FilterProcessor.cpp
        Source/FilterProcessor.h
        Source/LFOProcessor.cpp
        Source/LFOProcessor.h
        Source/PremiumKnobLookAndFeel.cpp
        Source/PremiumKnobLookAndFeel.h
        Source/VerticalFaderLookAndFeel.cpp
        Source/VerticalFaderLookAndFeel.h
        Source/LEDToggleButton.cpp
        Source/LEDToggleButton.h
        Source/ModernCyberLookAndFeel.cpp
        Source/ModernCyberLookAndFeel.h
        Source/HuntEngine.cpp
        Source/HuntEngine.h
        Source/LicenseManager.cpp
        Source/LicenseManager.h
        Source/LicenseActivationOverlay.cpp
        Source/LicenseActivationOverlay.h
)

# Link against JUCE libraries
target_link_libraries(HowlingWolves
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Binary Data
juce_add_binary_data(HowlingWolvesAssets
    SOURCES
        Resources/background.png
        Resources/howling_wolves_cave_bg_1768783846310.png
        Resources/logo_full.png
        Resources/logo_icon.png
)
target_link_libraries(HowlingWolves PRIVATE HowlingWolvesAssets)
