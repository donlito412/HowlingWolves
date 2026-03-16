#pragma once

#include "MidiDragComponent.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class MidiTab : public juce::Component {
public:
  MidiTab(HowlingWolvesAudioProcessor &);
  ~MidiTab() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // ARPEGGIATOR
  juce::Label arpLabel{"Arp Settings", "ARPEGGIATOR"};
  juce::ToggleButton arpEnableToggle{"Enable"};

  juce::Label rateLabel{"Rate", "Rate"};
  juce::ComboBox rateCombo;

  juce::Label modeLabel{"Mode", "Mode"};
  juce::ComboBox modeCombo;

  juce::Label octLabel{"Octave", "Octave"};
  juce::Slider octSlider;

  juce::Label gateLabel{"Gate", "Gate"};
  juce::Slider gateSlider;

  // KEY / CHORDS
  juce::Label chordLabel{"Chord Settings", "CHORD ENGINE"};
  juce::Label typeLabel{"Type", "Chord Type"};
  juce::ComboBox typeCombo;

  // MIDI Capture
  MidiDragComponent midiDrag{audioProcessor};

  // Attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      arpEnableAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      rateAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      modeAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> octAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      typeAtt;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiTab)
};
