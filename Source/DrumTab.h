#pragma once

#include "ModernCyberLookAndFeel.h"
#include "SampleManager.h"
#include "SynthEngine.h"
#include <JuceHeader.h>

//==============================================================================
class PadButton : public juce::Button {
public:
  PadButton(int midiNote, const juce::String &name)
      : juce::Button(name), noteNumber(midiNote) {}

  void paintButton(juce::Graphics &g, bool shouldDrawButtonAsMouseOver,
                   bool shouldDrawButtonAsDown) override;

  int getNoteNumber() const { return noteNumber; }
  void setFlashing(bool isFlashing); // For visual feedback

private:
  int noteNumber;
  bool flashing = false;
};

//==============================================================================
class DrumTab : public juce::Component, public juce::Button::Listener {
public:
  DrumTab(SampleManager &sm, SynthEngine &se);
  ~DrumTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void buttonClicked(juce::Button *button) override;
  void buttonStateChanged(juce::Button *button) override;

private:
  SampleManager &sampleManager;
  SynthEngine &synthEngine;

  juce::ComboBox kitSelector;
  std::unique_ptr<juce::FileChooser> fileChooser;

  juce::OwnedArray<PadButton> pads;

  void scanForKits();
  void loadKit(const juce::File &dir);
  void browseUserKit();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumTab)
};
