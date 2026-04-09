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
  juce::TextButton settingsButton{"SETTINGS"};
  juce::TextButton tipsButton{"TIPS"};

  // Settings Overlay
  SettingsTab settingsTab;
  bool showSettings = false;
  bool showSaveOverlay = false;

  // In-editor save preset UI (avoids modal AlertWindow)
  struct SavePresetOverlay : public juce::Component {
    SavePresetOverlay();
    void resized() override;
    void paint(juce::Graphics &g) override;
    juce::Label title;
    juce::TextEditor nameEditor;
    juce::TextButton commitButton;
    juce::TextButton cancelButton;
  };
  SavePresetOverlay savePresetOverlay;

  // Preset browser overlay
  PresetBrowser presetBrowser;

  // DrumTab drumTab;
  std::unique_ptr<LicenseActivationOverlay> licenseOverlay;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      HowlingWolvesAudioProcessorEditor)
};
