#pragma once

#include "ModernCyberLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class MidiDragComponent : public juce::Component,
                          public juce::DragAndDropContainer,
                          public juce::TooltipClient {
public:
  MidiDragComponent(HowlingWolvesAudioProcessor &p);
  ~MidiDragComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  juce::String getTooltip() override {
    return "Drag 'MIDI' icon to export recording";
  }

  void mouseDrag(const juce::MouseEvent &e) override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;
  juce::TextButton recordButton;

  bool isRecording = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiDragComponent)
};
