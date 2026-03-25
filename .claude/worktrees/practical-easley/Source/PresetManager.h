#pragma once

#include <JuceHeader.h>

class SampleManager; // Forward declaration

class PresetManager {
public:
  static const juce::File defaultDirectory;
  static const juce::File factoryDirectory;
  // macOS: /Users/Shared/Wolf Instruments — Windows: ProgramData\Wolf Instruments
  static const juce::File sharedDirectory;
  /** Drop-in packs from your site: each subfolder is one expansion (WAV/XML). */
  static const juce::File expansionsDirectory;
  static const juce::String presetExtension;

  /** WAV/XML shipped inside the plugin bundle (Resources/Factory). */
  static juce::File bundledFactoryDirectory() noexcept;

  /** List label for the browser (pack-prefixed under Expansions/). */
  static juce::String getDisplayNameForFile(const juce::File &file);
  /** Category combobox: pack name for expansions, or parent folder otherwise. */
  static juce::String getCategoryForPresetBrowser(const juce::File &file);

  PresetManager(juce::AudioProcessorValueTreeState &, SampleManager &);

  void savePreset(const juce::String &presetName);
  void deletePreset(const juce::String &presetName);
  void loadPreset(const juce::String &presetName);
  void loadPresetFile(const juce::File &file);
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
  juce::File currentPresetFile;
};
