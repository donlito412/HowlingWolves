#include "PremiumKnobLookAndFeel.h"

PremiumKnobLookAndFeel::PremiumKnobLookAndFeel() {
  setColour(juce::Slider::thumbColourId, CYAN_PRIMARY);
  setColour(juce::Slider::rotarySliderFillColourId, CYAN_PRIMARY);
}

void PremiumKnobLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                              int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider &slider) {

  auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = x + width * 0.5f;
  auto centreY = y + height * 0.5f;
  auto angle = rotaryStartAngle +
               sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

  if (currentSize == Large) {
    drawLargeKnob(g, x, y, width, height, angle, sliderPosProportional);
  } else {
    drawSmallKnob(g, x, y, width, height, angle, sliderPosProportional);
  }
}

void PremiumKnobLookAndFeel::drawLargeKnob(juce::Graphics &g, int x, int y,
                                           int width, int height, float angle,
                                           float value) {
  auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = x + width * 0.5f;
  auto centreY = y + height * 0.5f;

  float startAngle = juce::MathConstants<float>::pi * 1.2f;
  float endAngle = juce::MathConstants<float>::pi * 2.8f;

  // 1. Outer glow
  g.setColour(CYAN_PRIMARY.withAlpha(0.15f));
  g.fillEllipse(centreX - radius - 3, centreY - radius - 3, (radius + 3) * 2,
                (radius + 3) * 2);

  // 2. Metallic surface
  drawMetallicSurface(g, centreX, centreY, radius);

  // 3. Tick marks (15 marks)
  drawTickMarks(g, centreX, centreY, radius, 15, startAngle, endAngle, angle);

  // 4. Progress arc
  drawProgressArc(g, centreX, centreY, radius - 8, startAngle, endAngle, angle);

  // 5. Center indicator dot
  g.setColour(CYAN_PRIMARY);
  float indicatorX = centreX + (radius * 0.6f) * std::sin(angle);
  float indicatorY = centreY - (radius * 0.6f) * std::cos(angle);
  g.fillEllipse(indicatorX - 3, indicatorY - 3, 6, 6);
}

void PremiumKnobLookAndFeel::drawSmallKnob(juce::Graphics &g, int x, int y,
                                           int width, int height, float angle,
                                           float value) {
  auto radius = juce::jmin(width / 2, height / 2) - 3.0f;
  auto centreX = x + width * 0.5f;
  auto centreY = y + height * 0.5f;

  float startAngle = juce::MathConstants<float>::pi * 1.2f;
  float endAngle = juce::MathConstants<float>::pi * 2.8f;

  // Outer glow (smaller)
  g.setColour(CYAN_PRIMARY.withAlpha(0.12f));
  g.fillEllipse(centreX - radius - 2, centreY - radius - 2, (radius + 2) * 2,
                (radius + 2) * 2);

  // Metallic surface
  drawMetallicSurface(g, centreX, centreY, radius);

  // Tick marks (10 marks for small knobs)
  drawTickMarks(g, centreX, centreY, radius, 10, startAngle, endAngle, angle);

  // Progress arc (thinner)
  drawProgressArc(g, centreX, centreY, radius - 6, startAngle, endAngle, angle);

  // Center indicator (smaller)
  g.setColour(CYAN_PRIMARY);
  float indicatorX = centreX + (radius * 0.55f) * std::sin(angle);
  float indicatorY = centreY - (radius * 0.55f) * std::cos(angle);
  g.fillEllipse(indicatorX - 2, indicatorY - 2, 4, 4);
}

void PremiumKnobLookAndFeel::drawMetallicSurface(juce::Graphics &g,
                                                 float centreX, float centreY,
                                                 float radius) {
  // Base metallic gradient (radial)
  juce::ColourGradient metalGradient(METAL_LIGHT, centreX, centreY, METAL_DARK,
                                     centreX, centreY + radius, true);
  g.setGradientFill(metalGradient);
  g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

  // Specular highlight (top-left)
  juce::ColourGradient specular(
      juce::Colours::white.withAlpha(0.35f), centreX - radius * 0.3f,
      centreY - radius * 0.4f, juce::Colours::white.withAlpha(0.0f),
      centreX - radius * 0.3f + 20, centreY - radius * 0.4f + 25, true);
  g.setGradientFill(specular);
  g.fillEllipse(centreX - radius * 0.6f, centreY - radius * 0.7f, 35, 40);

  // Outer edge ring
  g.setColour(juce::Colour(0xff6a6a6a));
  g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2,
                1.5f);

  // Inner shadow for depth
  g.setColour(juce::Colours::black.withAlpha(0.25f));
  g.drawEllipse(centreX - radius + 2, centreY - radius + 2, radius * 2 - 4,
                radius * 2 - 4, 1.0f);
}

void PremiumKnobLookAndFeel::drawTickMarks(juce::Graphics &g, float centreX,
                                           float centreY, float radius,
                                           int numTicks, float startAngle,
                                           float endAngle, float currentAngle) {
  float angleRange = endAngle - startAngle;
  float tickRadius = radius + 4;

  for (int i = 0; i < numTicks; ++i) {
    float tickAngle = startAngle + (angleRange * i / (numTicks - 1));

    // Determine if tick is in active range
    bool isActive = (tickAngle <= currentAngle);
    g.setColour(isActive ? CYAN_PRIMARY : TICK_INACTIVE);

    // Calculate tick position
    float innerX = centreX + (tickRadius - 4) * std::sin(tickAngle);
    float innerY = centreY - (tickRadius - 4) * std::cos(tickAngle);
    float outerX = centreX + tickRadius * std::sin(tickAngle);
    float outerY = centreY - tickRadius * std::cos(tickAngle);

    // Draw tick mark (2px line)
    g.drawLine(innerX, innerY, outerX, outerY, 2.0f);
  }
}

void PremiumKnobLookAndFeel::drawProgressArc(juce::Graphics &g, float centreX,
                                             float centreY, float radius,
                                             float startAngle, float endAngle,
                                             float currentAngle) {
  if (currentAngle <= startAngle)
    return;

  juce::Path arcPath;
  arcPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle,
                        currentAngle, true);

  // Glow layer (wider, transparent)
  g.setColour(CYAN_PRIMARY.withAlpha(0.3f));
  g.strokePath(arcPath, juce::PathStrokeType(8.0f));

  // Main arc with gradient
  auto arcBounds = arcPath.getBounds();
  juce::ColourGradient arcGradient(
      CYAN_PRIMARY, arcBounds.getX(), arcBounds.getCentreY(), CYAN_LIGHT,
      arcBounds.getRight(), arcBounds.getCentreY(), false);
  g.setGradientFill(arcGradient);
  g.strokePath(arcPath, juce::PathStrokeType(6.0f));
}
