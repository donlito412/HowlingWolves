#pragma once
#include <JuceHeader.h>

class VerticalFaderLookAndFeel : public juce::LookAndFeel_V4 {
public:
  VerticalFaderLookAndFeel();

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override;

  juce::Label *createSliderTextBox(juce::Slider &slider) override;

private:
  // Colors
  const juce::Colour CYAN_PRIMARY = juce::Colour(0xff00d9ff);
  const juce::Colour CYAN_DARK = juce::Colour(0xff00b8dd);
  const juce::Colour BG_TRACK = juce::Colour(0xff1a1a1a);
  const juce::Colour TEXT_PRIMARY = juce::Colour(0xffffffff);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalFaderLookAndFeel)
};
