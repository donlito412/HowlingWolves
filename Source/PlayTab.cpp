#include "PlayTab.h"

PlayTab::PlayTab(HowlingWolvesAudioProcessor &p)
    : audioProcessor(p), thumbnail(512, p.formatManager, p.thumbCache) {
  // --- 1. SAMPLE / OSC A (TOP LEFT) ---
  setupLabel(sampleTitle, "SAMPLE // OSC A");
  setupKnob(sampleStart, "START", "sampleStart", startAtt);
  setupLabel(startLabel, "START");

  setupKnob(sampleLength, "LENGTH", "sampleLength", lenAtt);
  setupLabel(lenLabel, "LENGTH");

  setupKnob(volSlider, "VOLUME", "gain", volAtt);
  setupLabel(volLabel, "MASTER VOL");

  setupButton(revBtn, "REV");
  setupButton(loopBtn, "LOOP");

  // --- 2. AMP ENVELOPE (TOP RIGHT) ---
  setupLabel(envTitle, "AMP ENVELOPE");
  setupKnob(att, "ATT", "ampAttack", attAtt);
  setupLabel(attLabel, "ATTACK");
  setupKnob(dec, "DEC", "ampDecay", decAtt);
  setupLabel(decLabel, "DECAY");
  setupKnob(sus, "SUS", "ampSustain", susAtt);
  setupLabel(susLabel, "SUSTAIN");
  setupKnob(rel, "REL", "ampRelease", relAtt);
  setupLabel(relLabel, "RELEASE");

  setupSlider(velocity, "VELOCITY", true, "ampVelocity", velAtt);
  setupLabel(velLabel, "VELOCITY SENSITIVITY");

  setupSlider(pan, "PAN", true, "ampPan", panAtt);
  setupLabel(panLabel, "STEREO PANNING");

  // --- 3. BOTTOM ROW ---
  setupLabel(vcfTitle, "VCF FILTER");
  setupKnob(cutoff, "CUTOFF", "filterCutoff", cutAtt);
  setupLabel(cutLabel, "CUTOFF");
  setupKnob(res, "RES", "filterRes", resAtt);
  setupLabel(resLabel, "RES");
  setupKnob(drive, "DRIVE", "filterDrive", driveAtt);
  setupLabel(driveLabel, "DRIVE");

  setupLabel(lfoTitle, "LFO MOD");
  setupKnob(lfoRate, "RATE", "lfoRate", rateAtt);
  setupLabel(rateLabel, "RATE");
  setupKnob(lfoDepth, "DEPTH", "lfoDepth", depthAtt);
  setupLabel(depthLabel, "DEPTH");

  setupLabel(macroTitle, "MACROS");
  setupKnob(crushMacro, "CRUSH", "macroCrush", crushAtt);
  setupLabel(crushLabel, "CRUSH");
  setupKnob(spaceMacro, "SPACE", "macroSpace", spaceAtt);
  setupLabel(spaceLabel, "SPACE");

  // Register Listener
  audioProcessor.getSampleManager().addChangeListener(this);
}

PlayTab::~PlayTab() {
  audioProcessor.getSampleManager().removeChangeListener(this);
}

void PlayTab::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &audioProcessor.getSampleManager()) {
    auto path = audioProcessor.getSampleManager().getCurrentSamplePath();
    if (path.isNotEmpty()) {
      juce::File f(path);
      if (f.existsAsFile()) {
        thumbnail.setSource(new juce::FileInputSource(f));
        repaint();
      }
    }
  }
}

void PlayTab::setupKnob(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(500);
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PlayTab::setupSlider(
    juce::Slider &s, const juce::String &n, bool h, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(h ? juce::Slider::LinearHorizontal
                     : juce::Slider::LinearVertical);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(500);
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PlayTab::setupButton(juce::TextButton &b, const juce::String &t) {
  addAndMakeVisible(b);
  b.setButtonText(t);
  b.setClickingTogglesState(true);
  b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
  b.setColour(juce::TextButton::buttonColourId,
              juce::Colours::white.withAlpha(0.1f));
}

void PlayTab::setupLabel(juce::Label &l, const juce::String &t) {
  addAndMakeVisible(l);
  l.setText(t, juce::dontSendNotification);
  l.setColour(juce::Label::textColourId, juce::Colours::silver);

  if (t.contains("//") || t.contains("ENVELOPE") || t.contains("FILTER") ||
      t.contains("MOD") || t.contains("MACROS") || t.contains("VOL")) {
    l.setFont(juce::Font(juce::FontOptions(12.0f)).withStyle(juce::Font::bold));
    l.setJustificationType(juce::Justification::centred);
    l.setText(t.toUpperCase(), juce::dontSendNotification);
  } else {
    // Cosmetic Label
    l.setFont(juce::Font(juce::FontOptions(10.0f)).withStyle(juce::Font::bold));
    l.setJustificationType(juce::Justification::centred);
  }
}

void PlayTab::paint(juce::Graphics &g) {
  auto *lnf = dynamic_cast<ObsidianLookAndFeel *>(&getLookAndFeel());
  if (lnf) {
    lnf->drawGlassPanel(g, samplePanel);
    lnf->drawGlassPanel(g, envelopePanel);
    lnf->drawGlassPanel(g, filterPanel);
    lnf->drawGlassPanel(g, modPanel);
    lnf->drawGlassPanel(g, macroPanel);
  }

  // Draw Waveform in Sample Panel
  g.setColour(juce::Colours::cyan.withAlpha(0.8f));
  auto waveArea = samplePanel.reduced(15);
  waveArea = waveArea.withHeight(100); // Top 100px
  // Remove title space? No, logic inside resized handles components
  // Actually the user logic used "reduced(15).removeFromTop(100)" in Paint
  // *also*. I should match that.

  // Need to avoid drawing over title if possible, but title is Component on
  // top. The previous Resize logic places Title in top 25. So 25 + gap for
  // title. User paint logic: "samplePanel.reduced(15).removeFromTop(100)" That
  // suggests the wave fills that top area.

  auto wArea = samplePanel.reduced(15);
  wArea.removeFromTop(25);                  // Skip title
  auto finalWave = wArea.removeFromTop(80); // Matched logic in resized()

  if (thumbnail.getNumChannels() > 0)
    thumbnail.drawChannels(g, finalWave, 0.0, thumbnail.getTotalLength(), 1.0f);
  else {
    // Draw placeholder line
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.fillRect(finalWave.reduced(0, finalWave.getHeight() / 2 - 1));
  }
}

void PlayTab::resized() {
  auto area = getLocalBounds().reduced(15);
  auto topArea = area.removeFromTop((int)(getHeight() * 0.55f));
  auto bottomArea = area.reduced(0, 10);

  // --- TOP ROW ---
  samplePanel =
      topArea.removeFromLeft((int)(topArea.getWidth() * 0.65f)).reduced(5);
  envelopePanel = topArea.reduced(5);

  // --- BOTTOM ROW ---
  filterPanel = bottomArea.removeFromLeft((int)(bottomArea.getWidth() * 0.33f))
                    .reduced(5);
  macroPanel = bottomArea.removeFromRight((int)(bottomArea.getWidth() * 0.5f))
                   .reduced(5);
  modPanel = bottomArea.reduced(5);

  layoutSample();
  layoutEnvelope();
  layoutFilter();
  layoutMod();
  layoutMacros();
}

void PlayTab::layoutSample() {
  auto a = samplePanel.reduced(15);
  sampleTitle.setBounds(a.removeFromTop(25));

  // Reduced waveform height
  a.removeFromTop(80);

  // Knobs Row
  auto knobs = a.removeFromTop(80);
  int kw = knobs.getWidth() / 3;

  auto placeKnob = [&](juce::Slider &s, juce::Label &l) {
    auto zone = knobs.removeFromLeft(kw);
    // Draw knob slightly higher
    s.setBounds(zone.withSizeKeepingCentre(50, 50).translated(0, -5));
    // Label below
    l.setBounds(zone.getX(), s.getBottom(), zone.getWidth(), 15);
  };

  placeKnob(sampleStart, startLabel);
  placeKnob(sampleLength, lenLabel);
  placeKnob(volSlider, volLabel);

  auto btnRow = a.removeFromBottom(30);
  revBtn.setBounds(btnRow.removeFromLeft(60).reduced(2));
  loopBtn.setBounds(btnRow.removeFromLeft(60).reduced(2));
}

void PlayTab::layoutEnvelope() {
  auto a = envelopePanel.reduced(15);
  envTitle.setBounds(a.removeFromTop(25));

  auto knobs = a.removeFromTop(75);
  int kw = knobs.getWidth() / 4;

  auto placeKnob = [&](juce::Slider &s, juce::Label &l) {
    auto zone = knobs.removeFromLeft(kw);
    s.setBounds(zone.withSizeKeepingCentre(50, 50).translated(0, -5));
    l.setBounds(zone.withTrimmedTop(50).withHeight(15));
  };

  placeKnob(att, attLabel);
  placeKnob(dec, decLabel);
  placeKnob(sus, susLabel);
  placeKnob(rel, relLabel);

  auto vidArea = a.removeFromTop(45);
  velLabel.setBounds(vidArea.removeFromTop(15));
  velocity.setBounds(vidArea.reduced(0, 5));

  auto panArea = a.removeFromTop(45);
  panLabel.setBounds(panArea.removeFromTop(15));
  pan.setBounds(panArea.reduced(0, 5));
}

void PlayTab::layoutFilter() {
  auto a = filterPanel.reduced(15);
  vcfTitle.setBounds(a.removeFromTop(25));

  auto knobs = a.removeFromTop(80);
  int kw = knobs.getWidth() / 3;

  auto placeKnob = [&](juce::Slider &s, juce::Label &l) {
    auto zone = knobs.removeFromLeft(kw);
    s.setBounds(zone.withSizeKeepingCentre(50, 50).translated(0, -5));
    l.setBounds(zone.getX(), s.getBottom(), zone.getWidth(), 15);
  };

  placeKnob(cutoff, cutLabel);
  placeKnob(res, resLabel);
  placeKnob(drive, driveLabel);
}

void PlayTab::layoutMod() {
  auto a = modPanel.reduced(15);
  lfoTitle.setBounds(a.removeFromTop(25));
  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  lfoRate.setBounds(left.withSizeKeepingCentre(60, 60).translated(0, -10));
  rateLabel.setBounds(left.getX(), lfoRate.getBottom() - 5, left.getWidth(),
                      15);

  lfoDepth.setBounds(a.withSizeKeepingCentre(60, 60).translated(0, -10));
  depthLabel.setBounds(a.getX(), lfoDepth.getBottom() - 5, a.getWidth(), 15);
}

void PlayTab::layoutMacros() {
  auto a = macroPanel.reduced(15);
  macroTitle.setBounds(a.removeFromTop(25));
  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  crushMacro.setBounds(left.withSizeKeepingCentre(60, 60).translated(0, -10));
  crushLabel.setBounds(left.getX(), crushMacro.getBottom() - 5, left.getWidth(),
                       15);

  spaceMacro.setBounds(a.withSizeKeepingCentre(60, 60).translated(0, -10));
  spaceLabel.setBounds(a.getX(), spaceMacro.getBottom() - 5, a.getWidth(), 15);
}
