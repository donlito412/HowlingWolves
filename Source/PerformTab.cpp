#include "PerformTab.h"
#include "PianoRollComponent.h"

PerformTab::PerformTab(HowlingWolvesAudioProcessor &p)
    : audioProcessor(p) {
  addAndMakeVisible(midiDrag);

  // --- 2. GRID (NO TITLE) ---
  // setupLabel(arpTitle, "ARPEGGIATOR GRID"); // Removed

  // --- 3. VOICING ENGINE (BOTTOM LEFT) ---
  setupLabel(voicingTitle, "VOICING ENGINE");
  setupKnob(densityKnob, "DENSITY", "arpDensity", densityAtt);
  densityKnob.setTooltip("Adjusts the density of generated notes");

  setupLabel(densityLabel, "DENSITY");
  setupKnob(complexityKnob, "COMPLEXITY", "arpComplexity", complexityAtt);
  complexityKnob.setTooltip("Adjusts the rhythmic complexity");
  setupLabel(complexityLabel, "COMPLEXITY");

  // --- 4. SPREAD & RANGE (BOTTOM CENTER) ---
  setupLabel(spreadTitle, "SPREAD & RANGE");
  setupKnob(spreadWidth, "SPREAD", "arpSpread", spreadAtt);
  spreadWidth.setTooltip("Adjusts the stereo spread width");
  setupLabel(spreadLabel, "SPREAD WIDTH");
  spreadLabel.setFont(juce::Font(10.0f, juce::Font::bold));
  setupKnob(octaveRange, "OCTAVES", "arpOctave", octaveAtt);
  octaveRange.setTooltip("Sets the octave range for the arpeggiator");
  setupLabel(octaveLabel, "OCTAVE RANGE");
  octaveLabel.setFont(juce::Font(10.0f, juce::Font::bold));

  // --- 5. ARP CONTROLS (BOTTOM RIGHT) ---
  setupLabel(controlsTitle, "ARP CONTROLS");
  // Buttons setup without parameter attachments
  setupButton(chordHoldBtn, "CHORDS", juce::Colours::grey);
  chordHoldBtn.setTooltip("Click to select chord mode");

  setupButton(arpSyncBtn, "ARP SYNC", juce::Colours::grey);
  arpSyncBtn.setTooltip("Click to enable arpeggiator");

  // Selector Setup
  setupComboBox(arpModeSelector, "arpMode", arpModeAtt);
  arpModeSelector.setTooltip(
      "Select Arpeggiator Pattern (Up, Down, Random...)");

  setupComboBox(chordModeSelector, "chordMode", chordModeAtt);
  chordModeSelector.setTooltip("Select Chord Generation Mode");


  // Chord Button Logic (Popup Menu)
  // Fix: Button should NOT act as a toggle switch itself.
  // It opens menu. Menu selection updates the PARAMETER.
  // The Parameter Listener updates the Button's visual State.
  // Wait, these buttons are attached to params via `arpSyncAtt` etc.
  // If attached, the attachment handles the toggle state automatically based on
  // param! BUT: clicking toggles it *locally* too. Solution: Use
  // setClickingTogglesState(false) so clicking ONLY fires onClick.
  chordHoldBtn.setClickingTogglesState(false);
  chordHoldBtn.onClick = [this] {
    // Logic: If currently ON (Held), clicking allows us to turn OFF or Change
    // Mode? User wants 1-click access to menu.

    juce::PopupMenu m;
    m.addItem(1, "OFF", true, false);
    m.addItem(2, "Major", true, false);
    m.addItem(3, "Minor", true, false);
    m.addItem(4, "7th", true, false);
    m.addItem(5, "9th", true, false);

    m.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(chordHoldBtn),
        [this](int length) {
          if (length == 0)
            return; // Cancelled

          auto *modeParam = dynamic_cast<juce::AudioParameterChoice *>(
              audioProcessor.getAPVTS().getParameter("chordMode"));
          auto *holdParam = audioProcessor.getAPVTS().getParameter("chordHold");

          // Logic:
          // If "OFF" (1) -> Set Mode to 0, Hold to 0.
          // If Others -> Set Mode to Index, Hold to 1.

          if (length == 1) { // OFF
            if (holdParam)
              holdParam->setValueNotifyingHost(0.0f);
            if (modeParam)
              *modeParam = 0;
          } else {
            if (holdParam)
              holdParam->setValueNotifyingHost(1.0f);
            if (modeParam)
              *modeParam = length - 1;
          }
        });
  };

  // Arp Button Logic (Popup Menu)
  arpSyncBtn.setClickingTogglesState(false);
  arpSyncBtn.onClick = [this] {
    juce::PopupMenu m;
    m.addItem(1, "OFF", true, false);
    m.addItem(2, "Up", true, false);
    m.addItem(3, "Down", true, false);
    m.addItem(4, "Up/Down", true, false);
    m.addItem(5, "Random", true, false);

    m.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(arpSyncBtn),
        [this](int length) {
          if (length == 0)
            return;

          auto *modeParam = dynamic_cast<juce::AudioParameterChoice *>(
              audioProcessor.getAPVTS().getParameter("arpMode"));
          auto *enParam = audioProcessor.getAPVTS().getParameter("arpEnabled");

          if (length == 1) { // OFF
            if (enParam)
              enParam->setValueNotifyingHost(0.0f);
            if (modeParam)
              *modeParam = 0;
          } else {
            if (enParam)
              enParam->setValueNotifyingHost(1.0f);
            if (modeParam)
              *modeParam = length - 1;
          }
        });
  };

  // Removed old ComboBox Logic (Conflicting)
  // chordModeSelector.onChange...
  // arpModeSelector.onChange...

  // Removed old ComboBox and Play/Stop/Rec Button logic

  // Add parameter listeners
  audioProcessor.getAPVTS().addParameterListener("chordHold", this);
  audioProcessor.getAPVTS().addParameterListener("arpEnabled", this);

  // Initialize button colors based on current parameter values
  auto *chordParam = audioProcessor.getAPVTS().getParameter("chordHold");
  if (chordParam) {
    bool isActive = chordParam->getValue() > 0.5f;
    chordHoldBtn.setColour(juce::TextButton::buttonColourId,
                           isActive ? juce::Colours::cyan
                                    : juce::Colours::grey);
  }

  auto *arpParam = audioProcessor.getAPVTS().getParameter("arpEnabled");
  if (arpParam) {
    bool isActive = arpParam->getValue() > 0.5f;
    arpSyncBtn.setColour(juce::TextButton::buttonColourId,
                         isActive ? juce::Colours::cyan : juce::Colours::grey);
  }
}

// drawPianoRoll removed - now handled by PianoRollComponent

// (Removed duplicate resized)

PerformTab::~PerformTab() {
  // CRITICAL: Clear all onClick callbacks that capture [this]
  chordHoldBtn.onClick = nullptr;
  arpSyncBtn.onClick = nullptr;

  // Remove parameter listeners BEFORE components destruct
  audioProcessor.getAPVTS().removeParameterListener("arpEnabled", this);
  audioProcessor.getAPVTS().removeParameterListener("chordHold", this);
}

void PerformTab::parameterChanged(const juce::String &parameterID,
                                  float newValue) {
  // SAFETY CHECK - prevent crashes during destruction
  if (!isShowing())
    return; // Don't update if not visible/being destroyed

  if (parameterID == "chordHold") {
    bool isActive = newValue > 0.5f;
    chordHoldBtn.setColour(juce::TextButton::buttonColourId,
                           isActive ? juce::Colours::cyan
                                    : juce::Colours::grey);
  } else if (parameterID == "arpEnabled") {
    bool isActive = newValue > 0.5f;
    arpSyncBtn.setColour(juce::TextButton::buttonColourId,
                         isActive ? juce::Colours::cyan : juce::Colours::grey);
  }
}

void PerformTab::setupKnob(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(
      500); // Standard is 250, 500 = finer control/less loose
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PerformTab::setupSlider(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::LinearHorizontal);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(500);
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PerformTab::setupComboBox(
    juce::ComboBox &c, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        &att) {
  addAndMakeVisible(c);
  c.setJustificationType(juce::Justification::centred);

  // Add Items based on Param ID
  if (paramId == "arpMode") {
    c.addItem("OFF", 1);
    c.addItem("UP", 2);
    c.addItem("DOWN", 3);
    c.addItem("UP/DOWN", 4);
    c.addItem("RANDOM", 5);
  } else if (paramId == "chordMode") {
    c.addItem("OFF", 1);
    c.addItem("MAJOR", 2);
    c.addItem("MINOR", 3);
    c.addItem("7TH", 4);
    c.addItem("9TH", 5);
  }

  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), paramId, c);
}

// Removed old setupButton - now using single implementation without attachments

// Button setup without parameter attachment - manual color management
void PerformTab::setupButton(juce::TextButton &b, const juce::String &t,
                             juce::Colour c) {
  addAndMakeVisible(b);
  b.setButtonText(t);
  b.setColour(juce::TextButton::buttonOnColourId, c);
  b.setColour(juce::TextButton::buttonColourId, c);
  b.setClickingTogglesState(false); // Prevent toggle behavior
}

void PerformTab::setupLabel(juce::Label &l, const juce::String &t) {
  addAndMakeVisible(l);
  l.setText(t, juce::dontSendNotification);
  l.setColour(juce::Label::textColourId, juce::Colours::silver);
  l.setJustificationType(juce::Justification::centred);
  l.setFont(juce::Font(12.0f, juce::Font::bold));
}

void PerformTab::paint(juce::Graphics &g) {
  auto *lnf = dynamic_cast<ObsidianLookAndFeel *>(&getLookAndFeel());
  if (lnf) {
    // Draw Obsidian Glass Panels
    lnf->drawGlassPanel(g, transportPanel);
    lnf->drawGlassPanel(g, voicingPanel);
    lnf->drawGlassPanel(g, spreadPanel);
    lnf->drawGlassPanel(g, controlsPanel);
  }
}



// (Removed duplicate resized and drawArpMatrix)
void PerformTab::resized() {
  auto area = getLocalBounds().reduced(15);

  // Transport Bar for MIDI Drag
  transportPanel = area.removeFromTop(80).reduced(5);
  auto tArea = transportPanel.reduced(10);
  
  // Center the MidiDragComponent
  midiDrag.setBounds(tArea.withSizeKeepingCentre(120, 40));

  // The rest of the space is empty since we removed the piano roll.
  // We can just leave it empty or expand the bottom modules.
  area.removeFromTop(10);

  // Bottom Modules
  auto bottomArea = area.reduced(0, 10);
  voicingPanel = bottomArea.removeFromLeft((int)(bottomArea.getWidth() * 0.33f))
                     .reduced(5);
  controlsPanel =
      bottomArea.removeFromRight((int)(bottomArea.getWidth() * 0.5f))
          .reduced(5);
  spreadPanel = bottomArea.reduced(5);

  // Layout internal components
  layoutVoicing();
  layoutSpread();
  layoutControls();
}

void PerformTab::layoutVoicing() {
  auto a = voicingPanel.reduced(15);
  voicingTitle.setBounds(a.removeFromTop(25));

  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  densityKnob.setBounds(left.withSizeKeepingCentre(45, 45).translated(0, -10));
  densityLabel.setBounds(left.getX(), densityKnob.getBottom(), left.getWidth(),
                         15);

  complexityKnob.setBounds(a.withSizeKeepingCentre(45, 45).translated(0, -10));
  complexityLabel.setBounds(a.getX(), complexityKnob.getBottom(), a.getWidth(),
                            15);
}

void PerformTab::layoutSpread() {
  auto a = spreadPanel.reduced(15);
  spreadTitle.setBounds(a.removeFromTop(25));

  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  spreadWidth.setBounds(left.withSizeKeepingCentre(45, 45).translated(0, -10));
  spreadLabel.setBounds(left.getX(), spreadWidth.getBottom(), left.getWidth(),
                        15);

  octaveRange.setBounds(a.withSizeKeepingCentre(45, 45).translated(0, -10));
  octaveLabel.setBounds(a.getX(), octaveRange.getBottom(), a.getWidth(), 15);
}

void PerformTab::layoutControls() {
  auto a = controlsPanel.reduced(15);
  controlsTitle.setBounds(a.removeFromTop(25));

  // Row 1: Buttons
  auto row1 = a.removeFromTop(30);
  int w = row1.getWidth() / 2;
  chordHoldBtn.setBounds(row1.removeFromLeft(w).withSizeKeepingCentre(90, 25));
  arpSyncBtn.setBounds(row1.withSizeKeepingCentre(90, 25));

  // Row 2: Combos
  auto row2 = a.removeFromTop(35);
  int w2 = row2.getWidth() / 2;
  // User requested swap: Arp Sync is left, so Arp Mode should be left.
  // Wait, current code:
  // chordHoldBtn (Left), arpSyncBtn (Right)
  // arpModeSelector (Left), chordModeSelector (Right)
  // Mismatch: Chord Button (Left) <-> Arp Selector (Left) ??
  // Let's align them:
  // Left Side: Chord Hold Btn + Chord Mode Selector
  // Right Side: Arp Sync Btn + Arp Mode Selector

  // Previously:
  // chordHoldBtn.setBounds(row1.removeFromLeft(w)...); // Left
  // arpSyncBtn.setBounds(row1...); // Right

  // So Row 2 should be:
  // chordModeSelector (Left)
  // arpModeSelector (Right)

  chordModeSelector.setBounds(row2.removeFromLeft(w2).reduced(5, 2));
  arpModeSelector.setBounds(row2.reduced(5, 2));
}
