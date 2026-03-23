#pragma once

#include "PresetManager.h"
#include <JuceHeader.h>

class PresetBrowser : public juce::Component, public juce::ListBoxModel {
public:
  PresetBrowser(PresetManager &pm);
  ~PresetBrowser() override;

  std::function<void(const juce::String &)> onPresetSelected;

  void paint(juce::Graphics &) override;
  void resized() override;
  void visibilityChanged() override;

  // ListBoxModel overrides
  int getNumRows() override;
  void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                        bool rowIsSelected) override;
  void listBoxItemClicked(int rowNumber, const juce::MouseEvent &) override;
  void selectedRowsChanged(int lastRowSelected) override;

  void refresh();
  void filterPresets();

private:
  PresetManager &presetManager;
  juce::ListBox presetList;

  // Search/Filter
  juce::TextEditor searchBox;
  juce::ComboBox categoryFilter;
  juce::Label titleLabel{"PRESET BROWSER", "PRESET BROWSER"};

  // Data
  juce::StringArray categories;

  struct PresetInfo {
    juce::String name;
    juce::String category;
    juce::File file;
  };
  std::vector<PresetInfo> allPresetsInfo;
  juce::StringArray displayedPresets;
  juce::Array<juce::File> displayedPresetFiles;
};
