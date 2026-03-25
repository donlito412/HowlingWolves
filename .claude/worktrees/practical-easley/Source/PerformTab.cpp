#include "PerformTab.h"

namespace {
void setChoiceParamIndex(juce::AudioParameterChoice *p, int index) {
  if (p == nullptr || p->choices.isEmpty())
    return;
  index = juce::jlimit(0, p->choices.size() - 1, index);
  p->beginChangeGesture();
  p->setValueNotifyingHost(p->convertTo0to1(index));
  p->endChangeGesture();
}
} // namespace

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
            setChoiceParamIndex(modeParam, 0);
          } else {
            if (holdParam)
              holdParam->setValueNotifyingHost(1.0f);
            setChoiceParamIndex(modeParam, length - 1);
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
            setChoiceParamIndex(modeParam, 0);
          } else {
            if (enParam)
              enParam->setValueNotifyingHost(1.0f);
            setChoiceParamIndex(modeParam, length - 1);
          }
        });
  };

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

  auto row2 = a.removeFromTop(35);
  int w2 = row2.getWidth() / 2;

  chordModeSelector.setBounds(row2.removeFromLeft(w2).reduced(5, 2));
  arpModeSelector.setBounds(row2.reduced(5, 2));
}
