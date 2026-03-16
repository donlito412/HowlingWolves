#include "PresetBrowser.h"
#include "ModernCyberLookAndFeel.h"
#include <set>

PresetBrowser::PresetBrowser(PresetManager &pm) : presetManager(pm) {
  addAndMakeVisible(presetList);
  presetList.setModel(this);
  presetList.setColour(juce::ListBox::backgroundColourId,
                       juce::Colours::transparentBlack);
  presetList.setRowHeight(22);

  // No title label in sidebar mode
  // addAndMakeVisible(titleLabel); // Removed

  // Search Box
  addAndMakeVisible(searchBox);
  searchBox.setTextToShowWhenEmpty("Search presets...",
                                   juce::Colours::white.withAlpha(0.5f));
  searchBox.setColour(juce::TextEditor::backgroundColourId,
                      juce::Colours::transparentBlack);
  searchBox.setColour(juce::TextEditor::textColourId,
                      juce::Colours::white); // Ensure text is visible
  searchBox.setColour(juce::TextEditor::outlineColourId,
                      juce::Colours::transparentBlack);
  searchBox.onTextChange = [this] { filterPresets(); };

  // Category Filter
  addAndMakeVisible(categoryFilter);
  categories.add("All");
  categoryFilter.addItemList(categories, 1);
  categoryFilter.setSelectedId(1, juce::dontSendNotification); // Default to All
  categoryFilter.setTextWhenNothingSelected("Select Category...");
  // Match the panel style (Transparent to let panel color show through)
  categoryFilter.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colours::transparentBlack);
  categoryFilter.setColour(juce::ComboBox::textColourId, juce::Colours::white);
  categoryFilter.setColour(juce::ComboBox::arrowColourId,
                           juce::Colour::fromString("FF88CCFF"));
  categoryFilter.setColour(juce::ComboBox::outlineColourId,
                           juce::Colours::transparentBlack);
  // Match dropdown list (Popup) to the panel background
  categoryFilter.setColour(juce::PopupMenu::backgroundColourId,
                           WolfColors::PANEL_DARKER);
  categoryFilter.setColour(juce::PopupMenu::textColourId, juce::Colours::white);
  categoryFilter.setColour(juce::PopupMenu::highlightedBackgroundColourId,
                           WolfColors::ACCENT_CYAN.withAlpha(0.2f));
  categoryFilter.onChange = [this] { filterPresets(); };

  // refresh(); // Deferred to first open (visibilityChanged) or explicit
  // refresh
}

PresetBrowser::~PresetBrowser() {}

void PresetBrowser::paint(juce::Graphics &g) {
  // Fill background with semi-transparent dark color (Glass Look)
  g.setColour(WolfColors::PANEL_DARKER);
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

  // Draw border
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

void PresetBrowser::resized() {
  auto area = getLocalBounds();

  // No header title, just search and sort
  // No header title, just search and sort
  auto topArea = area.removeFromTop(90);

  searchBox.setBounds(
      topArea.removeFromTop(40).reduced(5, 5)); // 30px effective height

  categoryFilter.setBounds(topArea.removeFromTop(30).reduced(5, 0));

  area.reduce(5, 0);
  presetList.setBounds(area);
}

int PresetBrowser::getNumRows() { return displayedPresets.size(); }

void PresetBrowser::paintListBoxItem(int rowNumber, juce::Graphics &g,
                                     int width, int height,
                                     bool rowIsSelected) {
  if (rowNumber >= displayedPresets.size())
    return;

  if (rowIsSelected) {
    g.setColour(WolfColors::ACCENT_CYAN.withAlpha(0.2f));
    g.fillRect(0, 0, width, height);
  }

  g.setColour(rowIsSelected ? WolfColors::ACCENT_CYAN
                            : WolfColors::TEXT_PRIMARY);
  g.setFont(12.0f); // Compact font
  g.drawText(displayedPresets[rowNumber], 8, 0, width - 10, height,
             juce::Justification::centredLeft, true);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.fillRect(0, height - 1, width, 1); // Separator
}

void PresetBrowser::listBoxItemClicked(int rowNumber,
                                       const juce::MouseEvent &) {
  if (rowNumber < 0 || rowNumber >= displayedPresets.size())
    return;

  const auto presetName = displayedPresets[rowNumber];
  presetManager.loadPreset(presetName);

  // Notify listeners
  if (onPresetSelected)
    onPresetSelected(presetName);
  // logic in filterPresets) We use sendNotification to ensure the callback
  // triggers and updates the list
  searchBox.setText("", juce::dontSendNotification);

  // Update the placeholder text to show the selected preset name
  categoryFilter.setTextWhenNothingSelected(presetName);

  // Update selection without callback first, to ensure text updates
  categoryFilter.setSelectedId(0, juce::dontSendNotification);
  categoryFilter.repaint();

  // Manually hide the list
  filterPresets();
}

void PresetBrowser::selectedRowsChanged(int) {}

void PresetBrowser::refresh() {
  allPresetsInfo.clear();
  auto files = presetManager.getAllPresets();
  std::set<juce::String> uniqueCategories;

  // Scan metadata
  for (const auto &file : files) {
    juce::String name = file.getFileNameWithoutExtension();

    // For WAVs, we don't have metadata yet.
    // In future, we could look for a matching .xml file or check parent folder
    // name as Category. For now, default category to file's parent folder name
    // if inside a subfolder?

    juce::String category = "All";

    // Check if valid file
    if (file.existsAsFile()) {
      // Use parent directory name as category if it's not the root Preset
      // folder
      auto parent = file.getParentDirectory();
      if (parent != presetManager.getPresetFolder() &&
          parent != PresetManager::factoryDirectory) {
        category = parent.getFileName();
        uniqueCategories.insert(category);
      }
    }

    allPresetsInfo.push_back({name, category});
    DBG("Preset: " + name + " | Category: " + category);
  }

  // Rebuild categories list
  categories.clear();
  categories.add("All");
  for (const auto &cat : uniqueCategories) {
    categories.add(cat);
  }

  // Update ComboBox
  auto currentId = categoryFilter.getSelectedId();
  auto currentText = categoryFilter.getText();

  categoryFilter.clear();
  categoryFilter.addItemList(categories, 1);

  // Restore selection by Text (IDs might change if list changes)
  int idToSelect = 1; // Default to "All"

  if (currentId > 0 && categories.contains(currentText)) {
    // Find the new ID for this text
    for (int i = 0; i < categories.size(); ++i) {
      if (categories[i] == currentText) {
        idToSelect = i + 1;
        break;
      }
    }
  }

  categoryFilter.setSelectedId(idToSelect, juce::dontSendNotification);

  filterPresets();
}

void PresetBrowser::visibilityChanged() {
  if (isVisible()) {
    searchBox.setText("", juce::dontSendNotification);
    refresh();
  }
}

void PresetBrowser::filterPresets() {
  displayedPresets.clear();

  // If no category selected, don't show anything (unless searching? No, user
  // requested explicit select)
  if (categoryFilter.getSelectedId() == 0) {
    presetList.updateContent();
    repaint();
    return;
  }

  juce::String searchText = searchBox.getText().toLowerCase();
  juce::String selectedCat = categoryFilter.getText();

  for (const auto &info : allPresetsInfo) {
    bool matchSearch =
        searchText.isEmpty() || info.name.toLowerCase().contains(searchText);
    bool matchCat = (selectedCat == "All") || (info.category == selectedCat);

    if (matchSearch && matchCat) {
      displayedPresets.add(info.name);
    }
  }
  presetList.updateContent();
  repaint();
}
