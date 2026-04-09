#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ModulateTab.h"
#include "PerformTab.h"

//==============================================================================
HowlingWolvesAudioProcessorEditor::SavePresetOverlay::SavePresetOverlay()
{
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

void HowlingWolvesAudioProcessorEditor::SavePresetOverlay::paint(juce::Graphics &g)
{
  g.fillAll(juce::Colours::black.withAlpha(0.55f));
  auto box = getLocalBounds().withSizeKeepingCentre(360, 150).toFloat();
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(box, 8.0f);
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(box, 8.0f, 1.0f);
}

void HowlingWolvesAudioProcessorEditor::SavePresetOverlay::resized()
{
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
      settingsTab(p),
      presetBrowser(audioProcessor.getPresetManager())
{
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
  addAndMakeVisible(keyboardComponent);
  keyboardComponent.setAvailableRange(0, 127);

  // Overlays: added as children, immediately hidden.
  // Both setVisible AND off-screen bounds used together for host compatibility.
  addAndMakeVisible(settingsTab);
  addAndMakeVisible(savePresetOverlay);
  settingsTab.setVisible(false);
  savePresetOverlay.setVisible(false);

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
    if (showSettings) {
      settingsTab.setVisible(true);
      settingsTab.toFront(false);
      presetBrowser.setVisible(false);
      showSaveOverlay = false;
      savePresetOverlay.setVisible(false);
    } else {
      settingsTab.setVisible(false);
    }
    resized();
    repaint();
  };

  savePresetOverlay.commitButton.onClick = [this] {
    auto name = savePresetOverlay.nameEditor.getText().trim();
    if (name.isNotEmpty()) {
      audioProcessor.getPresetManager().savePreset(name);
      browseButton.setButtonText(name);
      audioProcessor.setLastPresetName(name);
    }
    showSaveOverlay = false;
    savePresetOverlay.setVisible(false);
    resized();
    repaint();
  };

  savePresetOverlay.cancelButton.onClick = [this] {
    showSaveOverlay = false;
    savePresetOverlay.setVisible(false);
    resized();
    repaint();
  };

  saveButton.onClick = [this] {
    savePresetOverlay.nameEditor.setText("My Preset");
    showSaveOverlay = true;
    savePresetOverlay.setVisible(true);
    savePresetOverlay.toFront(false);
    resized();
    repaint();
  };

  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false);
  presetBrowser.onPresetSelected = [this](const juce::String &presetName) {
    browseButton.setButtonText(presetName);
    audioProcessor.setLastPresetName(presetName);
    presetBrowser.setVisible(false);
  };

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

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor()
{
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
void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g)
{
  g.drawImage(backgroundImage, getLocalBounds().toFloat(),
              juce::RectanglePlacement::fillDestination);
}

void HowlingWolvesAudioProcessorEditor::timerCallback() { repaint(); }

void HowlingWolvesAudioProcessorEditor::resized()
{
  auto area = getLocalBounds();
  const int w = getWidth();
  const int h = getHeight();

  // Top bar
  auto topBar = area.removeFromTop(35);
  auto buttonArea = topBar.removeFromRight(360).reduced(5);
  browseButton.setBounds(buttonArea.removeFromLeft(150).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(buttonArea.removeFromLeft(90).reduced(2));
  tipsButton.setBounds(buttonArea.removeFromLeft(50).reduced(2));

  // Settings overlay: on-screen when showSettings, off-screen otherwise.
  if (showSettings)
    settingsTab.setBounds(getLocalBounds().reduced(40));
  else
    settingsTab.setBounds(-w, 0, w, h);

  // Save overlay: same approach.
  if (showSaveOverlay)
    savePresetOverlay.setBounds(getLocalBounds());
  else
    savePresetOverlay.setBounds(-w, 0, w, h);

  // Browser overlay
  if (presetBrowser.isVisible())
    presetBrowser.setBounds(browseButton.getX(), browseButton.getBottom() + 5,
                            220, 350);

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
