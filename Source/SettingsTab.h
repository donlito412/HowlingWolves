#pragma once

#include "ModernCyberLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
class SettingsTab : public juce::Component {
public:
  SettingsTab(HowlingWolvesAudioProcessor &p);
  ~SettingsTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // MIDI Settings
  juce::GroupComponent midiGroup;
  juce::Label midiLabel;
  juce::ComboBox midiChannelBox;
  juce::Label midiChannelLabel;

  // UI Settings
  juce::GroupComponent uiGroup;
  juce::Label uiLabel;
  juce::ComboBox scaleBox;
  juce::Label scaleLabel;

  // About / Info
  juce::Label aboutLabel;
  juce::Label versionLabel;
  juce::TextButton panicButton;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsTab)
};
