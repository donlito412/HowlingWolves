#pragma once

#include <JuceHeader.h>

class SampleManager; // Forward declaration

class PresetManager {
public:
  static const juce::File defaultDirectory;
  static const juce::File factoryDirectory;
  static const juce::File sharedDirectory; // /Users/Shared/Wolf Instruments/
  static const juce::File developmentDirectory;
  static const juce::File projectDirectory;
  static const juce::String presetExtension;

  PresetManager(juce::AudioProcessorValueTreeState &, SampleManager &);

  void savePreset(const juce::String &presetName);
  void deletePreset(const juce::String &presetName);
  void loadPreset(const juce::String &presetName);
  int loadNextPreset();
  int loadPreviousPreset();

  juce::Array<juce::File> getAllPresets() const;
  juce::String getCurrentPreset() const;

  juce::File getPresetFolder() const;
  juce::File getPresetFile(const juce::String &presetName) const;

private:
  void valueTreeRedirected(juce::ValueTree &treeThatChanged);

  juce::AudioProcessorValueTreeState &valueTreeState;
  SampleManager &sampleManager;
  juce::String currentPresetName;
};
