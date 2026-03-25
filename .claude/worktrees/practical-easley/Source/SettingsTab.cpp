#include "SettingsTab.h"

SettingsTab::SettingsTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  juce::ignoreUnused(audioProcessor);
  // --- MIDI Section ---
  addAndMakeVisible(midiLabel);
  midiLabel.setText("MIDI SETTINGS", juce::dontSendNotification);
  midiLabel.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
  midiLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  addAndMakeVisible(midiChannelLabel);
  midiChannelLabel.setText("Channel:", juce::dontSendNotification);
  midiChannelLabel.setColour(juce::Label::textColourId,
                             WolfColors::TEXT_SECONDARY);

  addAndMakeVisible(midiChannelBox);
  midiChannelBox.addItem("Omni", 1);
  for (int i = 1; i <= 16; ++i)
    midiChannelBox.addItem(juce::String(i), i + 1);
  midiChannelBox.setSelectedId(1); // Default to Omni
  midiChannelBox.setJustificationType(juce::Justification::centred);
  midiChannelBox.setTooltip("Selects the MIDI input channel.");

  // --- UI Section ---
  addAndMakeVisible(uiLabel);
  uiLabel.setText("INTERFACE", juce::dontSendNotification);
  uiLabel.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
  uiLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  addAndMakeVisible(scaleLabel);
  scaleLabel.setText("Scale:", juce::dontSendNotification);
  scaleLabel.setColour(juce::Label::textColourId, WolfColors::TEXT_SECONDARY);

  addAndMakeVisible(scaleBox);
  scaleBox.addItem("100%", 1);
  scaleBox.addItem("125%", 2);
  scaleBox.addItem("150%", 3);
  scaleBox.setSelectedId(1); // Default 100%
  scaleBox.setJustificationType(juce::Justification::centred);
  scaleBox.setTooltip("Adjusts the plugin window size.");
  scaleBox.onChange = [this] {
    if (auto *editor =
            findParentComponentOfClass<juce::AudioProcessorEditor>()) {
      switch (scaleBox.getSelectedId()) {
      case 1:
        editor->setSize(800, 545);
        break;
      case 2:
        editor->setSize(1000, 681);
        break;
      case 3:
        editor->setSize(1200, 817);
        break;
      }
    }
  };

  // --- About Section ---
  addAndMakeVisible(aboutLabel);
  aboutLabel.setText("WOLF INSTRUMENTS", juce::dontSendNotification);
  aboutLabel.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
  aboutLabel.setColour(juce::Label::textColourId, WolfColors::TEXT_PRIMARY);
  aboutLabel.setJustificationType(juce::Justification::centred);

  addAndMakeVisible(versionLabel);
  versionLabel.setText("Version 1.0.0", juce::dontSendNotification);
  versionLabel.setColour(juce::Label::textColourId, WolfColors::TEXT_SECONDARY);
  versionLabel.setJustificationType(juce::Justification::centred);

  addAndMakeVisible(panicButton);
  panicButton.setButtonText("PANIC / ALL OFF");
  panicButton.setTooltip("Stops all playing notes immediately.");
  panicButton.setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0xff4a0000));
  panicButton.onClick = [] {
    // Implement panic functionality (clear voices)
    // audioProcessor.clearVoices(); // Need to implement this in processor
  };
}

SettingsTab::~SettingsTab() {}

void SettingsTab::paint(juce::Graphics &g) {
  auto area = getLocalBounds().reduced(20);

  auto topArea = area.removeFromTop(area.getHeight() / 2).reduced(10);
  auto midiArea = topArea.removeFromLeft(topArea.getWidth() / 2).reduced(10);
  auto uiArea = topArea.reduced(10);
  auto aboutArea = area.reduced(10);

  // Backgrounds
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(midiArea.toFloat(), 6.0f);
  g.fillRoundedRectangle(uiArea.toFloat(), 6.0f);

  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(midiArea.toFloat(), 6.0f, 1.0f);
  g.drawRoundedRectangle(uiArea.toFloat(), 6.0f, 1.0f);

  // About separator
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawHorizontalLine(aboutArea.getY(), aboutArea.getX() + 100.0f,
                       aboutArea.getRight() - 100.0f);
}

void SettingsTab::resized() {
  auto area = getLocalBounds().reduced(20);

  auto topArea = area.removeFromTop(area.getHeight() / 2).reduced(10);
  auto midiArea = topArea.removeFromLeft(topArea.getWidth() / 2).reduced(10);
  auto uiArea = topArea.reduced(10);
  auto aboutArea = area.reduced(20);

  // Layout MIDI
  midiLabel.setBounds(midiArea.removeFromTop(30));

  juce::FlexBox midiFlex;
  midiFlex.justifyContent = juce::FlexBox::JustifyContent::center;
  midiFlex.alignItems = juce::FlexBox::AlignItems::center;
  midiFlex.items.add(
      juce::FlexItem(midiChannelLabel).withWidth(60).withHeight(30));
  midiFlex.items.add(
      juce::FlexItem(midiChannelBox).withWidth(100).withHeight(30));
  midiFlex.performLayout(midiArea);

  // Layout UI
  uiLabel.setBounds(uiArea.removeFromTop(30));

  juce::FlexBox uiFlex;
  uiFlex.justifyContent = juce::FlexBox::JustifyContent::center;
  uiFlex.alignItems = juce::FlexBox::AlignItems::center;
  uiFlex.items.add(juce::FlexItem(scaleLabel).withWidth(50).withHeight(30));
  uiFlex.items.add(juce::FlexItem(scaleBox).withWidth(100).withHeight(30));
  uiFlex.performLayout(uiArea);

  // Layout About
  juce::FlexBox aboutFlex;
  aboutFlex.flexDirection = juce::FlexBox::Direction::column;
  aboutFlex.justifyContent = juce::FlexBox::JustifyContent::center;
  aboutFlex.alignItems = juce::FlexBox::AlignItems::center;

  aboutFlex.items.add(juce::FlexItem(aboutLabel).withHeight(40));
  aboutFlex.items.add(juce::FlexItem(versionLabel).withHeight(20));
  aboutFlex.items.add(juce::FlexItem(panicButton)
                          .withWidth(150)
                          .withHeight(30)
                          .withMargin({20, 0, 0, 0}));

  aboutFlex.performLayout(aboutArea);
}
