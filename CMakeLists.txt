cmake_minimum_required(VERSION 3.22)

project(HowlingWolves VERSION 1.0.20)

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

# Local maintainer bypass: create an empty ".wolf_author_build" in the repo root
# (gitignored) so your own builds skip the Gumroad screen. CI and clones without
# that file still require a license. Do not commit the marker file.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.wolf_author_build")
  target_compile_definitions(HowlingWolves PRIVATE WOLF_AUTHOR_BUILD=1)
endif()

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

# Ship DistributionContent as factory WAV "presets" inside each plugin bundle.
# POST_BUILD runs after JUCE's copy-to-Plugins step, so we also copy Factory into
# the installed VST3/AU under ~/Library (Windows zip still ships DistributionContent).
if (APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DistributionContent")
  set(WOLF_FACTORY_CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/DistributionContent")
  get_target_property(WOLF_PRODUCT_NAME HowlingWolves JUCE_PRODUCT_NAME)
  set(WOLF_USER_VST3_FACTORY
      "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/${WOLF_PRODUCT_NAME}.vst3/Contents/Resources/Factory"
  )
  set(WOLF_USER_AU_FACTORY
      "$ENV{HOME}/Library/Audio/Plug-Ins/Components/${WOLF_PRODUCT_NAME}.component/Contents/Resources/Factory"
  )

  if (TARGET HowlingWolves_VST3)
    add_custom_command(
      TARGET HowlingWolves_VST3
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E rm -rf
              "$<TARGET_BUNDLE_DIR:HowlingWolves_VST3>/Contents/Resources/Factory"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${WOLF_FACTORY_CONTENT}"
              "$<TARGET_BUNDLE_DIR:HowlingWolves_VST3>/Contents/Resources/Factory"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${WOLF_USER_VST3_FACTORY}"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${WOLF_FACTORY_CONTENT}"
              "${WOLF_USER_VST3_FACTORY}"
      VERBATIM)
  endif()

  if (TARGET HowlingWolves_AU)
    add_custom_command(
      TARGET HowlingWolves_AU
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E rm -rf
              "$<TARGET_BUNDLE_DIR:HowlingWolves_AU>/Contents/Resources/Factory"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${WOLF_FACTORY_CONTENT}"
              "$<TARGET_BUNDLE_DIR:HowlingWolves_AU>/Contents/Resources/Factory"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${WOLF_USER_AU_FACTORY}"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${WOLF_FACTORY_CONTENT}"
              "${WOLF_USER_AU_FACTORY}"
      VERBATIM)
  endif()

  if (TARGET HowlingWolves_Standalone)
    add_custom_command(
      TARGET HowlingWolves_Standalone
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E rm -rf
              "$<TARGET_BUNDLE_DIR:HowlingWolves_Standalone>/Contents/Resources/Factory"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${WOLF_FACTORY_CONTENT}"
              "$<TARGET_BUNDLE_DIR:HowlingWolves_Standalone>/Contents/Resources/Factory"
      VERBATIM)
  endif()
endif()
