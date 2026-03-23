#pragma once

// #include "DrumTab.h" // Added - DISABLED
#include "EffectsTab.h"
#include "LicenseActivationOverlay.h"
#include "ModulateTab.h"
#include "ObsidianLookAndFeel.h"
#include "PerformTab.h"
#include "PlayTab.h"
#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include "SettingsTab.h"
#include <JuceHeader.h>

//==============================================================================
// Placeholder tab components (will be implemented in later phases)
//==============================================================================
// PlayTab class definition removed (now in PlayTab.h)
// ModulateTab class definition removed (now in ModulateTab.h)
// EffectsTab class definition removed (now in EffectsTab.h)
// SettingsTab class definition removed (now in SettingsTab.h)

//==============================================================================
class HowlingWolvesAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Timer,
                                          public juce::DragAndDropContainer {
public:
  HowlingWolvesAudioProcessorEditor(HowlingWolvesAudioProcessor &);
  ~HowlingWolvesAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // Modern UI components
  ObsidianLookAndFeel obsidianLookAndFeel;
  juce::TabbedComponent tabs;
  juce::ComponentBoundsConstrainer constrainer;

  // Keyboard (always visible at bottom)
  juce::MidiKeyboardComponent keyboardComponent;

  // Tooltips
  std::unique_ptr<juce::TooltipWindow> tooltipWindow;

  // Cave background
  juce::Image backgroundImage;
  juce::Image logoImage;

  // Top bar buttons
  juce::TextButton browseButton{"BROWSE"};
  juce::TextButton saveButton{"SAVE"};
  juce::TextButton settingsButton{"SETTINGS"}; // Now says SETTINGS
  juce::TextButton tipsButton{"TIPS"};         // New Tips Toggle

  // Settings Overlay
  SettingsTab settingsTab; // Reusing the SettingsTab as a component
  bool showSettings = false;

  // Preset browser overlay
  PresetBrowser presetBrowser;

  // Drum Tab (New)
  // Drum Tab (New)
  // Drum Tab (New)
  // DrumTab drumTab;

  std::unique_ptr<LicenseActivationOverlay> licenseOverlay;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      HowlingWolvesAudioProcessorEditor)
};
