#pragma once
#include <JuceHeader.h>

class PremiumKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
  PremiumKnobLookAndFeel();

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  enum KnobSize {
    Large, // 80x80px - main knobs
    Small  // 45x45px - secondary knobs
  };

  void setKnobSize(KnobSize size) { currentSize = size; }

private:
  void drawLargeKnob(juce::Graphics &g, int x, int y, int width, int height,
                     float angle, float value);
  void drawSmallKnob(juce::Graphics &g, int x, int y, int width, int height,
                     float angle, float value);

  void drawTickMarks(juce::Graphics &g, float centreX, float centreY,
                     float radius, int numTicks, float startAngle,
                     float endAngle, float currentAngle);
  void drawProgressArc(juce::Graphics &g, float centreX, float centreY,
                       float radius, float startAngle, float endAngle,
                       float currentAngle);
  void drawMetallicSurface(juce::Graphics &g, float centreX, float centreY,
                           float radius);

  KnobSize currentSize = Large;

  // Colors
  const juce::Colour CYAN_PRIMARY = juce::Colour(0xff00d9ff);
  const juce::Colour CYAN_LIGHT = juce::Colour(0xff88ccff);
  const juce::Colour CYAN_DARK = juce::Colour(0xff00b8dd);
  const juce::Colour METAL_LIGHT = juce::Colour(0xff5a5a5a);
  const juce::Colour METAL_DARK = juce::Colour(0xff2a2a2a);
  const juce::Colour TICK_INACTIVE = juce::Colour(0xff4a4a4a);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PremiumKnobLookAndFeel)
};
