#pragma once
#include <JuceHeader.h>

class ObsidianLookAndFeel : public juce::LookAndFeel_V4 {
public:
  ObsidianLookAndFeel() {
    setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    setColour(juce::Label::textColourId, juce::Colours::silver);

    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::ComboBox::outlineColourId,
              juce::Colours::silver.withAlpha(0.1f));
    setColour(juce::ComboBox::arrowColourId, juce::Colours::cyan);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::PopupMenu::textColourId, juce::Colours::silver);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,
              juce::Colours::cyan.withAlpha(0.2f));
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, const float rotaryStartAngle,
                        const float rotaryEndAngle,
                        juce::Slider &slider) override {
    // Override angles to user preference: 7 o'clock to 5 o'clock
    float startAngle = 2.5f; // Approx 7 o'clock
    float endAngle =
        2.5f + (juce::MathConstants<float>::twoPi * 0.82f); // Approx 5 o'clock

    auto radius = (float)juce::jmin(width / 2, height / 2);
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;

    // Geometry
    auto knobRadius = radius - 8.0f; // Smaller body to separate ticks

    // 1. Draw Background Ticks (Dim) & Active Ticks (Cyan)
    int numTicks = 24;
    float tickLen = 3.0f;
    float tickOuterR = radius;
    float tickInnerR = tickOuterR - tickLen;

    for (int i = 0; i < numTicks; ++i) {
      float prop = (float)i / (float)(numTicks - 1);
      float angle = startAngle + prop * (endAngle - startAngle);

      // "Lit" condition: Ticks up to the current pointer are lit
      // Using a small epsilon to ensure the tick AT the pointer is lit
      bool isLit = (sliderPos >= (prop - 0.02f));

      g.setColour(isLit ? juce::Colours::cyan
                        : juce::Colours::grey.withAlpha(0.3f));

      float thickness = isLit ? 2.0f : 1.5f;

      juce::Line<float> tick(centreX + std::cos(angle) * tickInnerR,
                             centreY + std::sin(angle) * tickInnerR,
                             centreX + std::cos(angle) * tickOuterR,
                             centreY + std::sin(angle) * tickOuterR);
      g.drawLine(tick, thickness);
    }

    // 2. Draw Knob Body
    g.setColour(juce::Colour(0xff181818)); // Very dark grey
    g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                  knobRadius * 2.0f);

    // Body Border/Edge
    g.setColour(juce::Colours::black);
    g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                  knobRadius * 2.0f, 1.5f);

    // Gloss/Gradient hint (optional, keeping ample for now)
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawEllipse(centreX - knobRadius + 2.0f, centreY - knobRadius + 2.0f,
                  (knobRadius - 2.0f) * 2.0f, (knobRadius - 2.0f) * 2.0f, 1.0f);

    // 3. Draw Pointer (Short Notch/Dot)
    // Points exactly to the current value angle
    auto currentAngle = startAngle + sliderPos * (endAngle - startAngle);

    // Draw a shorter "notch" indicator instead of a long line
    float notchLen = 6.0f;
    float notchStartR = knobRadius - notchLen - 2.0f;
    float notchEndR = knobRadius - 2.0f;

    juce::Line<float> pointer(centreX + std::cos(currentAngle) * notchStartR,
                              centreY + std::sin(currentAngle) * notchStartR,
                              centreX + std::cos(currentAngle) * notchEndR,
                              centreY + std::sin(currentAngle) * notchEndR);

    g.setColour(juce::Colours::cyan);
    g.drawLine(pointer, 3.0f); // Slightly thicker for visibility
  }

  // NEW: Linear Slider for Effects Tab
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    // Only handle horizontal for now as per design
    if (style == juce::Slider::LinearHorizontal) {
      auto trackHeight = 3.0f;
      auto thumbRadius = 5.0f;

      auto centreY = (float)y + (float)height * 0.5f;
      auto trackX = (float)x;
      auto trackW = (float)width;

      // Background Track
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRoundedRectangle(trackX, centreY - trackHeight * 0.5f, trackW,
                             trackHeight, 1.5f);

      // Active Track
      g.setColour(juce::Colours::cyan);
      auto activeW = sliderPos - trackX;
      g.fillRoundedRectangle(trackX, centreY - trackHeight * 0.5f, activeW,
                             trackHeight, 1.5f);

      // Thumb
      g.setColour(juce::Colours::cyan);
      g.fillEllipse(sliderPos - thumbRadius, centreY - thumbRadius,
                    thumbRadius * 2.0f, thumbRadius * 2.0f);

      // Thumb Glow
      g.setColour(juce::Colours::cyan.withAlpha(0.3f));
      g.fillEllipse(sliderPos - thumbRadius - 2.0f,
                    centreY - thumbRadius - 2.0f, (thumbRadius + 2.0f) * 2.0f,
                    (thumbRadius + 2.0f) * 2.0f);
    } else {
      // Fallback to default for vertical etc
      juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                             minSliderPos, maxSliderPos, style,
                                             slider);
    }
  }

  void drawGlassPanel(juce::Graphics &g, juce::Rectangle<int> area) {
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(area.toFloat(), 10.0f);
    g.setColour(juce::Colours::silver.withAlpha(0.1f));
    g.drawRoundedRectangle(area.toFloat(), 10.0f, 1.5f);
  }
  // --- Tooltip Implementation ---
  juce::Rectangle<int>
  getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
                   juce::Rectangle<int> parentArea) override {
    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths(juce::AttributedString(tipText),
                                               200.0f);
    auto width = (int)layout.getWidth() + 20;
    auto height = (int)layout.getHeight() + 10;
    return juce::Rectangle<int>(screenPos.x, screenPos.y + 24, width, height)
        .constrainedWithin(parentArea);
  }

  void drawTooltip(juce::Graphics &g, const juce::String &text, int width,
                   int height) override {
    auto bounds = juce::Rectangle<float>(width, height);

    // Background (Dark Glass)
    g.setColour(juce::Colour(0xff111111));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border (Cyan Glow)
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Text
    g.setColour(juce::Colours::white);
    g.setFont(14.0f); // Use older Font constructor for now if Options fails
                      // or just 14.0f if compatible
    g.drawFittedText(text, bounds.reduced(5).toNearestInt(),
                     juce::Justification::centred, 3);
  }
};
