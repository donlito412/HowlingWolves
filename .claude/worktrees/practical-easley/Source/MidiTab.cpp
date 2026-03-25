#include "MidiTab.h"
#include "ModernCyberLookAndFeel.h" // Ensure we use the Wolf Design System

MidiTab::MidiTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  // --- Arp Section ---
  addAndMakeVisible(arpLabel);
  arpLabel.setFont(juce::Font(18.0f, juce::Font::bold));
  arpLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  addAndMakeVisible(arpEnableToggle);
  arpEnableToggle.setTooltip("Enables the arpeggiator.");
  arpEnableAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.getAPVTS(), "arpEnabled", arpEnableToggle);

  addAndMakeVisible(rateLabel);
  addAndMakeVisible(rateCombo);
  rateCombo.addItemList({"1/4", "1/8", "1/16", "1/32"}, 1);
  rateCombo.setTooltip("Sets the arpeggiator speed.");
  rateAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          audioProcessor.getAPVTS(), "arpRate", rateCombo);

  addAndMakeVisible(modeLabel);
  addAndMakeVisible(modeCombo);
  modeCombo.addItemList({"Up", "Down", "Up/Down", "Random"}, 1);
  modeCombo.setTooltip("Sets the arpeggiator pattern.");
  modeAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          audioProcessor.getAPVTS(), "arpMode", modeCombo);

  addAndMakeVisible(octLabel);
  addAndMakeVisible(octSlider);
  octSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  octSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  octSlider.setTooltip("Sets the number of Octaves.");
  octAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.getAPVTS(), "arpOctave", octSlider);

  addAndMakeVisible(gateLabel);
  addAndMakeVisible(gateSlider);
  gateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  gateSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  gateSlider.setTooltip("Sets the note length.");
  gateAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.getAPVTS(), "arpGate", gateSlider);

  // --- Chord Section ---
  addAndMakeVisible(chordLabel);
  chordLabel.setFont(juce::Font(18.0f, juce::Font::bold));
  chordLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  addAndMakeVisible(typeLabel);
  addAndMakeVisible(typeCombo);
  typeCombo.addItemList({"Off", "Major", "Minor", "7th", "9th"}, 1);
  typeCombo.setTooltip("Automatically generates chords from single notes.");
  typeAtt =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          audioProcessor.getAPVTS(), "chordMode", typeCombo);

  // --- Capture Section ---
  addAndMakeVisible(midiDrag);
}

MidiTab::~MidiTab() {}

void MidiTab::paint(juce::Graphics &g) {
  // Background handled by parent, but let's draw panel outlines
  auto area = getLocalBounds().reduced(10);

  // Arp Panel
  auto arpArea = area.removeFromLeft(area.getWidth() / 2).reduced(10);
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(arpArea.toFloat(), 8.0f);
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(arpArea.toFloat(), 8.0f, 1.0f);

  // Chord Panel
  auto chordArea = area.reduced(10);
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(chordArea.toFloat(), 8.0f);
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(chordArea.toFloat(), 8.0f, 1.0f);
}

void MidiTab::resized() {
  auto area = getLocalBounds().reduced(10);

  // --- Left Column: Arpeggiator ---
  auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(20);

  arpLabel.setBounds(leftCol.removeFromTop(30));
  arpEnableToggle.setBounds(leftCol.removeFromTop(30));
  leftCol.removeFromTop(10);

  auto row1 = leftCol.removeFromTop(60);
  // Rate
  auto r1 = row1.removeFromLeft(row1.getWidth() / 2);
  rateLabel.setBounds(r1.removeFromTop(20));
  rateCombo.setBounds(r1.reduced(5));

  // Mode
  auto r2 = row1;
  modeLabel.setBounds(r2.removeFromTop(20));
  modeCombo.setBounds(r2.reduced(5));

  leftCol.removeFromTop(10);

  auto row2 = leftCol.removeFromTop(80);
  // Octave
  auto r3 = row2.removeFromLeft(row2.getWidth() / 2);
  octLabel.setBounds(r3.removeFromTop(20));
  octSlider.setBounds(r3.reduced(5));

  // Gate
  auto r4 = row2;
  gateLabel.setBounds(r4.removeFromTop(20));
  gateSlider.setBounds(r4.reduced(5));

  // --- Right Column: Chords ---
  auto rightCol = area.reduced(20);

  chordLabel.setBounds(rightCol.removeFromTop(30));
  rightCol.removeFromTop(40);

  auto cRow = rightCol.removeFromTop(60);
  typeLabel.setBounds(cRow.removeFromTop(20));
  typeCombo.setBounds(cRow.reduced(5));

  rightCol.removeFromTop(20);

  // Drag Component at the bottom of right column
  midiDrag.setBounds(rightCol.removeFromTop(40).reduced(0, 5));
}
