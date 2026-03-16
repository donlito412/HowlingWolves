#pragma once

#include <JuceHeader.h>

class CustomKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
  CustomKnobLookAndFeel();

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  enum KnobStyle { ModernMinimalist, GlassCrystal };

  void setKnobStyle(KnobStyle style) { currentStyle = style; }

private:
  KnobStyle currentStyle = ModernMinimalist;

  void drawModernMinimalistKnob(juce::Graphics &g, int x, int y, int width,
                                int height, float angle, float value);
  void drawGlassCrystalKnob(juce::Graphics &g, int x, int y, int width,
                            int height, float angle, float value);
};
