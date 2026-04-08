#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ModulateTab.h"
#include "PerformTab.h"

//==============================================================================
HowlingWolvesAudioProcessorEditor::SavePresetOverlay::SavePresetOverlay() {
  title.setText("Enter a name for your preset:", juce::dontSendNotification);
  title.setJustificationType(juce::Justification::centredLeft);
  title.setColour(juce::Label::textColourId, WolfColors::TEXT_PRIMARY);
  addAndMakeVisible(title);

  nameEditor.setMultiLine(false);
  nameEditor.setReturnKeyStartsNewLine(false);
  nameEditor.setText("My Preset");
  nameEditor.setColour(juce::TextEditor::backgroundColourId, WolfColors::PANEL_DARK);
  nameEditor.setColour(juce::TextEditor::textColourId, WolfColors::TEXT_PRIMARY);
  nameEditor.setColour(juce::TextEditor::outlineColourId, WolfColors::BORDER_SUBTLE);
  addAndMakeVisible(nameEditor);

  commitButton.setButtonText("Save");
  cancelButton.setButtonText("Cancel");
  addAndMakeVisible(commitButton);
  addAndMakeVisible(cancelButton);
}

void HowlingWolvesAudioProcessorEditor::SavePresetOverlay::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black.withAlpha(0.55f));
  auto box = getLocalBounds().withSizeKeepingCentre(360, 150).toFloat();
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(box, 8.0f);
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(box, 8.0f, 1.0f);
}

void HowlingWolvesAudioProcessorEditor::SavePresetOverlay::resized() {
  auto box = getLocalBounds().withSizeKeepingCentre(360, 150).reduced(16);
  title.setBounds(box.removeFromTop(22));
  box.removeFromTop(8);
  nameEditor.setBounds(box.removeFromTop(28));
  box.removeFromTop(12);
  auto btnRow = box.removeFromTop(30);
  commitButton.setBounds(btnRow.removeFromLeft(100).reduced(0, 2));
  btnRow.removeFromLeft(10);
  cancelButton.setBounds(btnRow.removeFromLeft(100).reduced(0, 2));
}

//==============================================================================
HowlingWolvesAudioProcessorEditor::HowlingWolvesAudioProcessorEditor(
    HowlingWolvesAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      tabs(juce::TabbedButtonBar::Orientation::TabsAtTop),
      keyboardComponent(audioProcessor.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard),
      settingsTab(p), presetBrowser(audioProcessor.getPresetManager()) {
  setLookAndFeel(&obsidianLookAndFeel);
  tabs.setLookAndFeel(&obsidianLookAndFeel);

  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::howling_wolves_cave_bg_1768783846310_png,
      BinaryData::howling_wolves_cave_bg_1768783846310_pngSize);
  logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_full_png,
                                              BinaryData::logo_full_pngSize);

  setResizable(false, false);
  setSize(800, 545);

  tabs.addTab("PLAY", WolfColors::PANEL_DARK, new PlayTab(audioProcessor), true);
  tabs.addTab("MODULATE", WolfColors::PANEL_DARK, new ModulateTab(audioProcessor), true);
  tabs.addTab("PERFORM", WolfColors::PANEL_DARK, new PerformTab(audioProcessor), true);
  tabs.addTab("EFFECTS", WolfColors::PANEL_DARK, new EffectsTab(audioProcessor), true);
  tabs.setCurrentTabIndex(0);
  addAndMakeVisible(tabs);

  const auto savedName = audioProcessor.getLastPresetName();
  browseButton.setButtonText(savedName.isNotEmpty() ? savedName : "Select a Preset");
  browseButton.onClick = [this] {
    presetBrowser.setVisible(!presetBrowser.isVisible());
    presetBrowser.toFront(true);
    resized();
  };
  addAndMakeVisible(browseButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(settingsButton);
  addAndMakeVisible(tipsButton);

  // Keyboard — added before overlays so overlays are naturally on top
  addAndMakeVisible(keyboardComponent);
  keyboardComponent.setAvailableRange(0, 127);

  // Settings overlay — use alpha=0 to hide (more reliable than setVisible in plugin hosts)
  addAndMakeVisible(settingsTab);
  settingsTab.setAlpha(0.0f);
  settingsTab.setInterceptsMouseClicks(false, false);

  tipsButton.setClickingTogglesState(true);
  tipsButton.setToggleState(true, juce::dontSendNotification);
  tipsButton.onClick = [this] {
    if (tipsButton.getToggleState())
      tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
    else
      tooltipWindow = nullptr;
  };
  tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);

  settingsButton.onClick = [this] {
    showSettings = !showSettings;
    settingsTab.setAlpha(showSettings ? 1.0f : 0.0f);
    settingsTab.setInterceptsMouseClicks(showSettings, showSettings);
    if (showSettings) {
      settingsTab.toFront(true);
      presetBrowser.setVisible(false);
      // Also hide save overlay if open
      savePresetOverlay.setAlpha(0.0f);
      savePresetOverlay.setInterceptsMouseClicks(false, false);
    }
    resized();
  };

  // Save overlay — use alpha=0 to hide
  addAndMakeVisible(savePresetOverlay);
  savePresetOverlay.setAlpha(0.0f);
  savePresetOverlay.setInterceptsMouseClicks(false, false);

  savePresetOverlay.commitButton.onClick = [this] {
    auto name = savePresetOverlay.nameEditor.getText().trim();
    if (name.isNotEmpty()) {
      audioProcessor.getPresetManager().savePreset(name);
      browseButton.setButtonText(name);
      audioProcessor.setLastPresetName(name);
    }
    savePresetOverlay.setAlpha(0.0f);
    savePresetOverlay.setInterceptsMouseClicks(false, false);
  };

  savePresetOverlay.cancelButton.onClick = [this] {
    savePresetOverlay.setAlpha(0.0f);
    savePresetOverlay.setInterceptsMouseClicks(false, false);
  };

  saveButton.onClick = [this] {
    savePresetOverlay.nameEditor.setText("My Preset");
    savePresetOverlay.setAlpha(1.0f);
    savePresetOverlay.setInterceptsMouseClicks(true, true);
    savePresetOverlay.toFront(true);
    resized();
  };

  // Preset browser overlay
  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false);
  presetBrowser.onPresetSelected = [this](const juce::String &presetName) {
    browseButton.setButtonText(presetName);
    audioProcessor.setLastPresetName(presetName);
    presetBrowser.setVisible(false);
  };

  // License overlay
  if (!audioProcessor.checkLicenseValid()) {
    licenseOverlay = std::make_unique<LicenseActivationOverlay>(
        audioProcessor.getLicenseManager(), [this]() {
          audioProcessor.setLicenseValid(true);
          if (licenseOverlay != nullptr)
            licenseOverlay->setVisible(false);
        });
    addChildComponent(licenseOverlay.get());
    licenseOverlay->setVisible(true);
    licenseOverlay->toFront(true);
  }

  resized();
  repaint();
  startTimerHz(30);
}

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor() {
  browseButton.onClick = nullptr;
  saveButton.onClick = nullptr;
  settingsButton.onClick = nullptr;
  tipsButton.onClick = nullptr;
  savePresetOverlay.commitButton.onClick = nullptr;
  savePresetOverlay.cancelButton.onClick = nullptr;
  stopTimer();
  setLookAndFeel(nullptr);
  tabs.setLookAndFeel(nullptr);
}

//==============================================================================
void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g) {
  g.drawImage(backgroundImage, getLocalBounds().toFloat(),
              juce::RectanglePlacement::fillDestination);
}

void HowlingWolvesAudioProcessorEditor::timerCallback() {
  repaint();
}

void HowlingWolvesAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  auto topBar = area.removeFromTop(35);
  auto buttonArea = topBar.removeFromRight(360).reduced(5);
  browseButton.setBounds(buttonArea.removeFromLeft(150).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(buttonArea.removeFromLeft(90).reduced(2));
  tipsButton.setBounds(buttonArea.removeFromLeft(50).reduced(2));

  // Settings overlay
  settingsTab.setBounds(getLocalBounds().reduced(40));

  // Save preset overlay — always full bounds
  savePresetOverlay.setBounds(getLocalBounds());

  // Browser overlay
  if (presetBrowser.isVisible()) {
    presetBrowser.setBounds(browseButton.getX(), browseButton.getBottom() + 5,
                            220, 350);
  }

  // Keyboard
  auto keyboardArea = area.removeFromBottom(80);
  keyboardComponent.setBounds(keyboardArea);
  int totalKeys = 72;
  float keyWidth = keyboardArea.getWidth() / (float)(totalKeys * 0.7f);
  keyboardComponent.setKeyWidth(keyWidth);

  // Tabs
  tabs.setBounds(area);

  // License overlay
  if (licenseOverlay != nullptr)
    licenseOverlay->setBounds(getLocalBounds());
}
