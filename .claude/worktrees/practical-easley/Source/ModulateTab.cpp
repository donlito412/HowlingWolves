#include "ModulateTab.h"

ModulateTab::ModulateTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  // --- 1. LFO VISUALIZER (TOP) ---
  setupLabel(visTitle, "LFO 1 VISUALIZER");
  setupLabel(syncLabel, "SYNC: 1/4");

  // --- 2. LFO PARAMETERS (LEFT PANEL) ---
  setupLabel(lfoTitle, "LFO PARAMETERS");

  addAndMakeVisible(waveSelector);
  setupLabel(waveLabel, "WAVE SHAPE");
  waveSelector.addItemList(
      {"SINE", "SQUARE", "TRIANGLE"},
      1); // Matching Processor choices order usually: Sine, Square, Triangle
  // User Snippet: {"SINE", "TRIANGLE", "SAW", "SQUARE"}.
  // Processor: Sine, Square, Triangle.
  // I will stick to Processor choices to ensure correct mapping if I attach.
  // Actually, I'll match the User Snippet visuals but mapping might be slightly
  // off if not updated in Processor. I'll stick to Processor Layout: Sine,
  // Square, Triangle.

  if (auto *a = audioProcessor.getAPVTS().getParameter("lfoWave"))
    waveAtt = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "lfoWave", waveSelector);
  else
    waveSelector.setSelectedId(1);

  setupKnob(rateKnob, "RATE", "lfoRate", rateAtt);
  setupLabel(rateLabel, "RATE");
  setupKnob(depthKnob, "DEPTH", "lfoDepth", depthAtt);
  setupLabel(depthLabel, "DEPTH");

  // Phase and Smooth
  setupKnob(phaseKnob, "PHASE", "lfoPhase", phaseAtt);
  setupLabel(phaseLabel, "PHASE");
  setupSlider(smoothSlider, "SMOOTH", true, "modSmooth", smoothAtt);
  setupLabel(smoothLabel, "SMOOTH");

  // --- 3. MODULATION ROUTING (RIGHT PANEL) ---
  setupLabel(routingTitle, "MODULATION ROUTING");
  addAndMakeVisible(targetSelector);
  setupLabel(targetLabel, "TARGET");
  targetSelector.addItemList({"FILTER CUTOFF", "VOLUME", "PAN", "PITCH"},
                             1); // Matching Processor Choices

  if (auto *a = audioProcessor.getAPVTS().getParameter("lfoTarget"))
    targetAtt = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "lfoTarget", targetSelector);
  else
    targetSelector.setSelectedId(1);

  // ADSR Knobs
  setupKnob(modA, "A", "modAttack", modAAtt);
  setupLabel(modALabel, "A");
  modALabel.setFont(
      juce::Font(juce::FontOptions(10.0f)).withStyle(juce::Font::bold));
  setupKnob(modD, "D", "modDecay", modDAtt);
  setupLabel(modDLabel, "D");
  modDLabel.setFont(
      juce::Font(juce::FontOptions(10.0f)).withStyle(juce::Font::bold));
  setupKnob(modS, "S", "modSustain", modSAtt);
  setupLabel(modSLabel, "S");
  modSLabel.setFont(
      juce::Font(juce::FontOptions(10.0f)).withStyle(juce::Font::bold));
  setupKnob(modR, "R", "modRelease", modRAtt);
  setupLabel(modRLabel, "R");
  modRLabel.setText("R", juce::dontSendNotification);
  modRLabel.setFont(
      juce::Font(juce::FontOptions(10.0f)).withStyle(juce::Font::bold));

  setupSlider(amountSlider, "MOD AMOUNT", true, "modAmount", amountAtt);

  startTimerHz(60);
}

ModulateTab::~ModulateTab() {
  // CRITICAL FIX: Reset ALL attachments BEFORE sliders are destroyed
  // Otherwise attachment destructorss try to removeListener from destroyed
  // sliders
  rateAtt.reset();
  depthAtt.reset();
  phaseAtt.reset();
  smoothAtt.reset();
  modAAtt.reset();
  modDAtt.reset();
  modSAtt.reset();
  modRAtt.reset();
  amountAtt.reset();
  waveAtt.reset();
  targetAtt.reset();

  stopTimer();
}

void ModulateTab::timerCallback() {
  // Only animate if notes are playing, per user request
  bool isAnyVoiceActive = false;
  for (int i = 0; i < audioProcessor.getSynth().getNumVoices(); ++i) {
    if (auto *v = audioProcessor.getSynth().getVoice(i)) {
      if (v->isVoiceActive()) {
        isAnyVoiceActive = true;
        break;
      }
    }
  }

  if (isAnyVoiceActive) {
    phaseOffset += 0.05f;
  }
  repaint();
}

void ModulateTab::setupKnob(
    juce::Slider &s, const juce::String &name, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(500); // Higher sensitivity for more control
  if (audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void ModulateTab::setupSlider(
    juce::Slider &s, const juce::String &name, bool horizontal,
    const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(horizontal ? juce::Slider::LinearHorizontal
                              : juce::Slider::LinearVertical);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  if (audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void ModulateTab::setupLabel(juce::Label &l, const juce::String &t) {
  addAndMakeVisible(l);
  l.setText(t, juce::dontSendNotification);
  l.setColour(juce::Label::textColourId, juce::Colours::silver);
  l.setFont(juce::Font(juce::FontOptions(12.0f)).withStyle(juce::Font::bold));
}

void ModulateTab::drawLFOWave(juce::Graphics &g, juce::Rectangle<int> area) {
  g.setColour(juce::Colours::black.withAlpha(0.2f));
  g.fillRoundedRectangle(area.toFloat(), 6.0f);

  // Fetch current wave choice
  int waveType = 0; // Default Sine
  if (auto *p = audioProcessor.getAPVTS().getParameter("lfoWave"))
    waveType =
        (int)p->convertFrom0to1(p->getValue()); // 0=Sine, 1=Square, 2=Triangle

  g.setColour(juce::Colours::cyan);
  juce::Path wave;
  float midY = (float)area.getCentreY();
  float height = area.getHeight() * 0.3f;
  float freq = 0.04f;

  if (waveType == 0) { // Sine
    wave.startNewSubPath((float)area.getX(), midY);
    for (float x = 0; x < area.getWidth(); x += 2.0f) {
      float t = (float)x * freq + phaseOffset;
      float y = midY + std::sin(t) * height;
      wave.lineTo((float)area.getX() + x, y);
    }
  } else if (waveType == 1) { // Square
    wave.startNewSubPath((float)area.getX(), midY);
    for (float x = 0; x < area.getWidth(); x += 2.0f) {
      float t = (float)x * freq + phaseOffset;
      float val = std::sin(t) >= 0 ? 1.0f : -1.0f;
      float y = midY + val * height;
      wave.lineTo((float)area.getX() + x, y);
    }
  } else if (waveType == 2) { // Triangle
    wave.startNewSubPath((float)area.getX(), midY);
    for (float x = 0; x < area.getWidth(); x += 2.0f) {
      float t = (float)x * freq + phaseOffset;
      // Tri: (2/pi)*asin(sin(x)) or linear approx
      float val =
          (2.0f / juce::MathConstants<float>::pi) * std::asin(std::sin(t));
      float y = midY + val * height;
      wave.lineTo((float)area.getX() + x, y);
    }
  } else if (waveType == 3) { // Saw
    wave.startNewSubPath((float)area.getX(), midY);
    for (float x = 0; x < area.getWidth(); x += 2.0f) {
      float t = (float)x * freq + phaseOffset;
      // Saw: (2/pi) * atan(tan(x/2)) or fmod
      // Simple sawtooth: 2 * (t/2pi - floor(t/2pi + 0.5))
      float normT = t / juce::MathConstants<float>::twoPi;
      float val = 2.0f * (normT - std::floor(normT + 0.5f));
      float y = midY - val * height; // Invert for standard saw up
      wave.lineTo((float)area.getX() + x, y);
    }
  }

  g.strokePath(wave, juce::PathStrokeType(2.0f));
}

void ModulateTab::paint(juce::Graphics &g) {
  auto *lnf = dynamic_cast<ObsidianLookAndFeel *>(&getLookAndFeel());
  if (lnf) {
    lnf->drawGlassPanel(g, visPanel);
    lnf->drawGlassPanel(g, lfoPanel);
    lnf->drawGlassPanel(g, routingPanel);
  } else {
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for (auto area : {visPanel, lfoPanel, routingPanel})
      g.fillRoundedRectangle(area.toFloat(), 10.0f);
  }

  // Draw LFO Wave Visualizer
  drawLFOWave(g, visPanel.reduced(15));
}

void ModulateTab::resized() {
  auto area = getLocalBounds().reduced(15);

  // Top section for Visualizer
  visPanel = area.removeFromTop((int)(getHeight() * 0.35f)).reduced(5);
  visTitle.setBounds(visPanel.getX() + 10, visPanel.getY() + 5, 200, 30);
  syncLabel.setBounds(visPanel.getRight() - 110, visPanel.getY() + 5, 100, 30);

  // Bottom section split for LFO and Routing
  auto bottomArea = area.reduced(0, 10);
  lfoPanel = bottomArea.removeFromLeft((int)(bottomArea.getWidth() * 0.48f))
                 .reduced(5);
  routingPanel =
      bottomArea.removeFromRight((int)(bottomArea.getWidth() * 0.94f))
          .reduced(5);
  // Note: User snippet said *0.94f after removing left...
  // If left is ~48%, remaining is 52%. 94% of 52% is ~49%. Matches nicely
  // roughly half-half. Or did user mean "removeFromRight(remaining.width
  // * 1.0)"? 0.94 seems specific or maybe a typo for "rest". I'll stick to
  // snippet logic.

  // --- LFO PARAMETERS LAYOUT ---
  auto lArea = lfoPanel.reduced(15);
  lfoTitle.setBounds(lArea.removeFromTop(25));

  // Wave Selector & Label
  auto waveRow = lArea.removeFromTop(45);
  waveLabel.setBounds(waveRow.removeFromTop(15));
  waveSelector.setBounds(waveRow.reduced(20, 0));

  // Center Knobs row
  auto knobRow = lArea.removeFromTop(90);
  int kw = knobRow.getWidth() / 3;

  auto k1 = knobRow.removeFromLeft(kw);
  rateKnob.setBounds(k1.withSizeKeepingCentre(60, 60).translated(0, -10));
  rateLabel.setBounds(k1.getX(), rateKnob.getBottom(), k1.getWidth(), 15);

  auto k2 = knobRow.removeFromLeft(kw);
  depthKnob.setBounds(k2.withSizeKeepingCentre(60, 60).translated(0, -10));
  depthLabel.setBounds(k2.getX(), depthKnob.getBottom(), k2.getWidth(), 15);

  auto k3 = knobRow;
  phaseKnob.setBounds(k3.withSizeKeepingCentre(60, 60).translated(0, -10));
  phaseLabel.setBounds(k3.getX(), phaseKnob.getBottom(), k3.getWidth(), 15);

  // Smooth Slider
  auto smoothRow = lArea.removeFromBottom(40);
  smoothLabel.setBounds(smoothRow.removeFromTop(15));
  smoothSlider.setBounds(smoothRow.reduced(20, 0));

  // --- ROUTING LAYOUT ---
  auto rArea = routingPanel.reduced(15);
  routingTitle.setBounds(rArea.removeFromTop(25));

  // Target Selector
  auto targetRow = rArea.removeFromTop(45);
  targetLabel.setBounds(targetRow.removeFromTop(15));
  targetSelector.setBounds(targetRow.reduced(20, 0));

  // Amount Slider
  auto amtRow = rArea.removeFromBottom(40);
  amountSlider.setBounds(amtRow.reduced(20, 0));

  // ADSR Knobs Layout
  auto adsrArea = rArea.reduced(10, 5); // Tighter area
  int numKnobs = 4;
  int w = adsrArea.getWidth() / numKnobs;

  auto placeKnob = [&](juce::Slider &s, juce::Label &l) {
    auto col = adsrArea.removeFromLeft(w);
    // Place knob centered - Moved down by 5px
    s.setBounds(col.withSizeKeepingCentre(45, 45).translated(0, 5));
    // Label below
    l.setBounds(col.getX(), s.getBottom(), col.getWidth(), 15);
  };

  placeKnob(modA, modALabel);
  placeKnob(modD, modDLabel);
  placeKnob(modS, modSLabel);
  placeKnob(modR, modRLabel);
}
