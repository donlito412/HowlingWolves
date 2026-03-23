#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ModulateTab.h"
#include "PerformTab.h"

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

  // Top bar buttons
  browseButton.setButtonText("Select a Preset");
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
      presetBrowser.setVisible(false); // Hide browser if open
    }
  };

  // Save Button Logic
  saveButton.onClick = [this] {
    auto alert = std::make_unique<juce::AlertWindow>(
        "Save Preset",
        "Enter a name for your preset:", juce::AlertWindow::QuestionIcon);

    alert->addTextEditor("presetName", "My Preset");
    alert->addButton("Save", 1);
    alert->addButton("Cancel", 0);

    alert->enterModalState(
        true,
        juce::ModalCallbackFunction::create([this, a = alert.get()](int r) {
          if (r == 1) {
            auto name = a->getTextEditorContents("presetName");
            audioProcessor.getPresetManager().savePreset(name);
          }
        }));
    // AlertWindow is self-deleting by default when modal finishes if running
    // via enterModalState? No, we used unique_ptr, wait. Actually,
    // enterModalState with true takes ownership? Let's check docs or be safe.
    // Actually, enterModalState(true) deletes it. So we release.
    alert.release();
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

  // Settings Overlay Position
  if (settingsTab.isVisible()) {
    settingsTab.setBounds(getLocalBounds().reduced(40));
  }

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
