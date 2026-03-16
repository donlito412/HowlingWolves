#include "MidiDragComponent.h"

MidiDragComponent::MidiDragComponent(HowlingWolvesAudioProcessor &p)
    : audioProcessor(p) {

  recordButton.setButtonText("REC");
  recordButton.setClickingTogglesState(true);
  recordButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colours::red);
  recordButton.setTooltip("Click to Start/Stop Recording");
  // setTooltip is not a member of Component, implemented via getTooltip()
  // override if needed or handled by checking children? No, for this component
  // itself.

  recordButton.onClick = [this] {
    if (recordButton.getToggleState()) {
      audioProcessor.getMidiCapturer().startRecording();
      isRecording = true;
    } else {
      audioProcessor.getMidiCapturer().stopRecording();
      isRecording = false;
    }
    repaint();
  };

  addAndMakeVisible(recordButton);
}

MidiDragComponent::~MidiDragComponent() {}

void MidiDragComponent::paint(juce::Graphics &g) {
  // Draw Box
  g.setColour(
      WolfColors::ACCENT_CYAN.withAlpha(0.3f)); // Brighter background fill
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

  g.setColour(WolfColors::ACCENT_CYAN.withAlpha(0.5f)); // Revert alpha
  g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f,
                         1.0f); // Revert thickness

  // Draw Icon area
  auto iconArea = getLocalBounds().removeFromRight(getHeight());
  // Removed padding reduction to match original size behavior if needed,
  // or keep it simple. Original didn't have reduce(5,5).

  if (audioProcessor.getMidiCapturer().hasRecording() && !isRecording) {
    g.setColour(WolfColors::ACCENT_CYAN); // Active
    g.drawText("MIDI", iconArea, juce::Justification::centred);
    g.drawRect(iconArea, 1.0f);
  } else {
    g.setColour(WolfColors::TEXT_DISABLED); // Inactive
    g.drawText("MIDI", iconArea, juce::Justification::centred);
  }
}

void MidiDragComponent::resized() {
  auto area = getLocalBounds().reduced(5);
  recordButton.setBounds(area.removeFromLeft(50));
}

void MidiDragComponent::mouseDrag(const juce::MouseEvent &e) {
  if (isRecording)
    return;

  auto &capturer = audioProcessor.getMidiCapturer();
  if (capturer.hasRecording()) {
    auto file = capturer.saveToTempFile();
    if (file.existsAsFile()) {
      juce::StringArray files;
      files.add(file.getFullPathName());
      performExternalDragDropOfFiles(files, true);
    }
  }
}
