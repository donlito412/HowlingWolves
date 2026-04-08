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
  nameEditor.setColour(juce::TextEditor::backgroundColourId,
                       WolfColors::PANEL_DARK);
  nameEditor.setColour(juce::TextEditor::textColourId, WolfColors::TEXT_PRIMARY);
  nameEditor.setColour(juce::TextEditor::outlineColourId,
                       WolfColors::BORDER_SUBTLE);
  addAndMakeVisible(nameEditor);

  commitButton.setButtonText("Save");
  cancelButton.setButtonText("Cancel");
  addAndMakeVisible(commitButton);
  addAndMakeVisible(cancelButton);
}

void HowlingWolvesAudioProcessorEditor::SavePresetOverlay::paint(
    juce::Graphics &g) {
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
  // Set LookAndFeel (Obsidian)
  setLookAndFeel(&obsidianLookAndFeel);
  tabs.setLookAndFeel(&obsidianLookAndFeel);

  // Load cave background
  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::howling_wolves_cave_bg_1768783846310_png,
      BinaryData::howling_wolves_cave_bg_1768783846310_pngSize);

  // Load Logo
  logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_full_png,
                                              BinaryData::logo_full_pngSize);

  // Set up size constraints
  // NUCLEAR OPTION: Fixed Size, No Limits (Simplicity)
  setResizable(false, false);

  // Set initial size
  setSize(800, 545);

  // ... inside constructor ...
  // Create and add tabs
  tabs.addTab("PLAY", WolfColors::PANEL_DARK, new PlayTab(audioProcessor),
              true);
  tabs.addTab("MODULATE", WolfColors::PANEL_DARK,
              new ModulateTab(audioProcessor), true);
  tabs.addTab("PERFORM", WolfColors::PANEL_DARK, new PerformTab(audioProcessor),
              true);
  tabs.addTab("EFFECTS", WolfColors::PANEL_DARK, new EffectsTab(audioProcessor),
              true);

  tabs.setCurrentTabIndex(0);
  addAndMakeVisible(tabs);

  // Top bar buttons — restore saved name, or show default
  const auto savedName = audioProcessor.getLastPresetName();
  browseButton.setButtonText(savedName.isNotEmpty() ? savedName : "Select a Preset");
  browseButton.onClick = [this] {
    presetBrowser.setVisible(!presetBrowser.isVisible());
    presetBrowser.toFront(true);
    resized(); // Force layout update to position the browser
  };
  addAndMakeVisible(browseButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(settingsButton);
  addAndMakeVisible(tipsButton);

  // Settings Overlay
  addChildComponent(settingsTab);
  settingsTab.setVisible(false);

  // Tips Button Logic
  tipsButton.setClickingTogglesState(true);
  tipsButton.setToggleState(true, juce::dontSendNotification); // On by default
  tipsButton.onClick = [this] {
    if (tipsButton.getToggleState()) {
      tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
    } else {
      tooltipWindow = nullptr;
    }
  };
  // Initialize Tooltips (On by default)
  tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);

  // Settings Button Logic
  settingsButton.onClick = [this] {
    showSettings = !showSettings;
    settingsTab.setVisible(showSettings);
    if (showSettings) {
      settingsTab.toFront(true);
      presetBrowser.setVisible(false);
      savePresetOverlay.setVisible(false);
    }
    resized();
  };

  // Save: in-editor overlay (modal AlertWindow stays open in plugin/standalone)
  addChildComponent(savePresetOverlay);
  savePresetOverlay.setVisible(false);
  savePresetOverlay.commitButton.onClick = [this] {
    auto name = savePresetOverlay.nameEditor.getText().trim();
    if (name.isNotEmpty()) {
      audioProcessor.getPresetManager().savePreset(name);
      browseButton.setButtonText(name);
      audioProcessor.setLastPresetName(name);
    }
    savePresetOverlay.setVisible(false);
    resized();
  };
  savePresetOverlay.cancelButton.onClick = [this] {
    savePresetOverlay.setVisible(false);
    resized();
  };

  saveButton.onClick = [this] {
    savePresetOverlay.nameEditor.setText("My Preset");
    savePresetOverlay.setVisible(true);
    savePresetOverlay.toFront(true);
    resized();
  };

  // Keyboard
  addAndMakeVisible(keyboardComponent);

  // Keyboard Setup
  keyboardComponent.setAvailableRange(0, 127);

  // Preset browser overlay
  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false);

  // Handle preset selection
  presetBrowser.onPresetSelected = [this](const juce::String &presetName) {
    browseButton.setButtonText(presetName);
    audioProcessor.setLastPresetName(presetName);
    presetBrowser.setVisible(false);
  };

  // Setup License Overlay
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

  // Force initial layout and paint to prevent stall
  resized();
  repaint();

  // Start UI refresh timer
  startTimerHz(30);
}

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor() {
  // CRITICAL: Clear all onClick callbacks that capture [this]
  // These can fire during/after destruction causing crashes
  browseButton.onClick = nullptr;
  saveButton.onClick = nullptr;
  settingsButton.onClick = nullptr;
  tipsButton.onClick = nullptr;
  savePresetOverlay.commitButton.onClick = nullptr;
  savePresetOverlay.cancelButton.onClick = nullptr;

  // Clean up look and feel
  stopTimer();
  setLookAndFeel(nullptr);
  tabs.setLookAndFeel(nullptr);
}

//==============================================================================
void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g) {
  // Draw cave background
  g.drawImage(backgroundImage, getLocalBounds().toFloat(),
              juce::RectanglePlacement::fillDestination);

  // Logo removed per user request
  // if (logoImage.isValid()) { ... }
}

void HowlingWolvesAudioProcessorEditor::timerCallback() {
  // Refresh UI components that might need it (e.g. Visualizers)
  repaint();
}

void HowlingWolvesAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Top bar (35px)
  auto topBar = area.removeFromTop(35);

  // Top bar buttons (right side)
  auto buttonArea = topBar.removeFromRight(360).reduced(5); // Increased width
  browseButton.setBounds(buttonArea.removeFromLeft(150).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(
      buttonArea.removeFromLeft(90).reduced(2));                  // Settings
  tipsButton.setBounds(buttonArea.removeFromLeft(50).reduced(2)); // Tips

  // Settings overlay: always keep bounds in sync so first open has non-zero size
  settingsTab.setBounds(getLocalBounds().reduced(40));

  // Save preset overlay (full editor bounds when visible)
  if (savePresetOverlay.isVisible())
    savePresetOverlay.setBounds(getLocalBounds());
  else
    savePresetOverlay.setBounds(0, 0, 0, 0);

  // Browser Overlay Position
  if (presetBrowser.isVisible()) {
    presetBrowser.setBounds(browseButton.getX(), browseButton.getBottom() + 5,
                            220, 350);
  }

  // Keyboard (bottom, 80px, stretches full width)
  auto keyboardArea = area.removeFromBottom(80);
  keyboardComponent.setBounds(keyboardArea);

  // Calculate key width for full-width stretching
  int totalKeys = 72; // 6 octaves
  float keyWidth = keyboardArea.getWidth() / (float)(totalKeys * 0.7f);
  keyboardComponent.setKeyWidth(keyWidth);

  // Tabs (remaining space)
  tabs.setBounds(area);

  // License Overlay (Top Most, Full Screen)
  if (licenseOverlay != nullptr) {
    licenseOverlay->setBounds(getLocalBounds());
  }
}
