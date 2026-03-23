#include "DrumTab.h"

//==============================================================================
// PadButton Implementation
//==============================================================================

void PadButton::paintButton(juce::Graphics &g, bool shouldDrawButtonAsMouseOver,
                            bool shouldDrawButtonAsDown) {
  auto bounds = getLocalBounds().toFloat();
  float corner = 4.0f;

  juce::Colour baseColour = WolfColors::PANEL_DARKER;
  juce::Colour glowColour = WolfColors::ACCENT_CYAN.withAlpha(0.0f);
  juce::Colour borderColour = WolfColors::BORDER_PANEL;

  if (shouldDrawButtonAsDown || flashing) {
    baseColour = WolfColors::ACCENT_GLOW.withAlpha(0.6f);
    glowColour = WolfColors::ACCENT_CYAN.withAlpha(0.8f);
    borderColour = WolfColors::ACCENT_CYAN;
  } else if (shouldDrawButtonAsMouseOver) {
    baseColour = WolfColors::PANEL_DARK.brighter(0.1f);
    borderColour = WolfColors::BORDER_SUBTLE;
  }

  // Fill
  g.setColour(baseColour);
  g.fillRoundedRectangle(bounds, corner);

  // Glow Effect (Center)
  if (shouldDrawButtonAsDown || flashing) {
    juce::ColourGradient grad(glowColour, bounds.getCentre(),
                              baseColour.withAlpha(0.0f), bounds.getTopLeft(),
                              true);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, corner);
  }

  // Border
  g.setColour(borderColour);
  g.drawRoundedRectangle(bounds, corner, 1.5f);

  // Text (Note Name/Number)
  g.setColour(WolfColors::TEXT_PRIMARY);
  // Convert MIDI to Name (e.g. 36 -> C1)
  juce::String noteName =
      juce::MidiMessage::getMidiNoteName(noteNumber, true, true, 3);
  g.drawText(noteName, bounds, juce::Justification::centred, true);
}

void PadButton::setFlashing(bool isFlashing) {
  flashing = isFlashing;
  repaint();
}

//==============================================================================
// DrumTab Implementation
//==============================================================================

DrumTab::DrumTab(SampleManager &sm, SynthEngine &se)
    : sampleManager(sm), synthEngine(se) {

  addAndMakeVisible(kitSelector);
  kitSelector.onChange = [this] {
    int id = kitSelector.getSelectedId();
    if (id == 9999) {
      browseUserKit();
    } else if (id > 0) {
      juce::String kitName = kitSelector.getText();

      // Check Factory
      juce::File factoryPath(
          "/Users/Shared/Wolf Instruments/Samples/Drum Kits");
      juce::File kit = factoryPath.getChildFile(kitName);
      if (kit.isDirectory()) {
        loadKit(kit);
        return;
      }

      // Check User
      juce::File userPath =
          juce::File::getSpecialLocation(juce::File::userMusicDirectory)
              .getChildFile("Howling Wolves/Drum Kits");
      kit = userPath.getChildFile(kitName);
      if (kit.isDirectory()) {
        loadKit(kit);
        return;
      }
    }
  };

  scanForKits();

  // Create 16 Pads (4x4)
  int startNote = 36;
  for (int i = 0; i < 16; ++i) {
    auto *pad = new PadButton(startNote + i, "Pad " + juce::String(i + 1));
    pad->addListener(this);
    pads.add(pad);
    addAndMakeVisible(pad);
  }
}

void DrumTab::scanForKits() {
  kitSelector.clear();
  int id = 1;

  // 1. Factory Kits
  kitSelector.addSectionHeading("Factory Kits");
  juce::File factoryPath("/Users/Shared/Wolf Instruments/Samples/Drum Kits");
  if (factoryPath.isDirectory()) {
    for (const auto &dir :
         factoryPath.findChildFiles(juce::File::findDirectories, false)) {
      kitSelector.addItem(dir.getFileName(), id++);
    }
  }

  // 2. User Kits
  kitSelector.addSectionHeading("User Kits");
  juce::File userPath =
      juce::File::getSpecialLocation(juce::File::userMusicDirectory)
          .getChildFile("Howling Wolves/Drum Kits");

  // Ensure User Path Exists
  if (!userPath.exists())
    userPath.createDirectory();

  if (userPath.isDirectory()) {
    for (const auto &dir :
         userPath.findChildFiles(juce::File::findDirectories, false)) {
      kitSelector.addItem(dir.getFileName(), id++);
    }
  }

  kitSelector.addSeparator();
  kitSelector.addItem("Browse Folder...", 9999);

  kitSelector.setText("Select Drum Kit");
}

DrumTab::~DrumTab() {}

void DrumTab::paint(juce::Graphics &g) {
  // Background is handled by parent or transparent
}

void DrumTab::resized() {
  auto area = getLocalBounds().reduced(20);

  // Header Control
  auto header = area.removeFromTop(40);
  kitSelector.setBounds(header.removeFromLeft(200));

  area.removeFromTop(20); // Spacer

  // Grid Layout
  int cols = 4;
  int rows = 4;
  int gap = 10;

  int padW = (area.getWidth() - (gap * (cols - 1))) / cols;
  int padH = (area.getHeight() - (gap * (rows - 1))) / rows;

  for (int i = 0; i < 16; ++i) {
    int row = i / cols; // 0..3
    int col = i % cols; // 0..3

    // Bottom-Up layout
    int drawRow = (rows - 1) - (i / cols);
    int drawCol = i % cols;

    if (i < pads.size()) {
      pads[i]->setBounds(area.getX() + drawCol * (padW + gap),
                         area.getY() + drawRow * (padH + gap), padW, padH);
    }
  }
}

void DrumTab::buttonClicked(juce::Button *button) {
  // Only pads use this now
}

void DrumTab::buttonStateChanged(juce::Button *button) {
  // Identify if it's a pad
  if (auto *pad = dynamic_cast<PadButton *>(button)) {
    if (pad->isDown()) {
      // Note On
      synthEngine.noteOn(1, pad->getNoteNumber(), 1.0f);
    } else {
      // Note Off
      synthEngine.noteOff(1, pad->getNoteNumber(), 0.0f, true);
    }
  }
}

void DrumTab::loadKit(const juce::File &dir) {
  if (dir.isDirectory()) {
    sampleManager.loadDrumKit(dir);
  }
}

void DrumTab::browseUserKit() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Select Drum Kit Folder",
      juce::File::getSpecialLocation(juce::File::userMusicDirectory)
          .getChildFile("Howling Wolves/Drum Kits"),
      "*");

  fileChooser->launchAsync(juce::FileBrowserComponent::canSelectDirectories,
                           [this](const juce::FileChooser &fc) {
                             auto dir = fc.getResult();
                             if (dir.exists()) {
                               if (dir.isDirectory()) {
                                 loadKit(dir);
                               }
                             }
                           });
}
