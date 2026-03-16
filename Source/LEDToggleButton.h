#pragma once
#include <JuceHeader.h>

class LEDToggleButton : public juce::ToggleButton {
public:
  LEDToggleButton();

  void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted,
                   bool shouldDrawButtonAsDown) override;

private:
  const juce::Colour CYAN_PRIMARY = juce::Colour(0xff00d9ff);
  const juce::Colour BG_OFF = juce::Colour(0xff2a2a2a);
  const juce::Colour TEXT_ON = juce::Colours::white;
  const juce::Colour TEXT_OFF = juce::Colour(0xff666666);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LEDToggleButton)
};
