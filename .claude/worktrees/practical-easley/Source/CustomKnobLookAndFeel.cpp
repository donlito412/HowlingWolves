#include "CustomKnobLookAndFeel.h"

CustomKnobLookAndFeel::CustomKnobLookAndFeel() {
  // Set default colors for the look and feel
  setColour(juce::Slider::thumbColourId, juce::Colour(0xff00d9ff));
  setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d9ff));
  setColour(juce::Slider::rotarySliderOutlineColourId,
            juce::Colour(0xff2a2a2a));
}

void CustomKnobLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
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

  if (currentStyle == ModernMinimalist) {
    drawModernMinimalistKnob(g, x, y, width, height, angle,
                             sliderPosProportional);
  } else {
    drawGlassCrystalKnob(g, x, y, width, height, angle, sliderPosProportional);
  }
}

void CustomKnobLookAndFeel::drawModernMinimalistKnob(juce::Graphics &g, int x,
                                                     int y, int width,
                                                     int height, float angle,
                                                     float value) {
  auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = x + width * 0.5f;
  auto centreY = y + height * 0.5f;

  // Outer glow (subtle)
  g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.2f));
  g.fillEllipse(centreX - radius - 2, centreY - radius - 2, (radius + 2) * 2,
                (radius + 2) * 2);

  // Shiny metallic base with radial gradient (darker to lighter)
  juce::ColourGradient metalGradient(juce::Colour(0xff5a5a5a), centreX, centreY,
                                     juce::Colour(0xff2a2a2a), centreX,
                                     centreY + radius, true);
  g.setGradientFill(metalGradient);
  g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

  // Specular highlight (top-left, shiny spot)
  juce::ColourGradient specular(
      juce::Colours::white.withAlpha(0.4f), centreX - radius * 0.3f,
      centreY - radius * 0.4f, juce::Colours::white.withAlpha(0.0f),
      centreX - radius * 0.3f + 15, centreY - radius * 0.4f + 20, true);
  g.setGradientFill(specular);
  g.fillEllipse(centreX - radius * 0.5f, centreY - radius * 0.6f, 30, 35);

  // Metallic edge ring
  g.setColour(juce::Colour(0xff6a6a6a));
  g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2,
                1.5f);

  // Inner shadow for depth
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.drawEllipse(centreX - radius + 2, centreY - radius + 2, radius * 2 - 4,
                radius * 2 - 4, 1.0f);

  // Progress arc track (background)
  g.setColour(juce::Colour(0xff1a1a1a));
  juce::Path arcTrack;
  arcTrack.addCentredArc(centreX, centreY, radius - 6, radius - 6,
                         0.0f, // rotation
                         juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f, true);
  g.strokePath(arcTrack, juce::PathStrokeType(6.0f)); // Thicker: 6px

  // Progress arc with GRADIENT (cyan to white)
  if (value > 0.0f) {
    juce::Path arcProgress;
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = startAngle + value * juce::MathConstants<float>::pi * 1.5f;
    arcProgress.addCentredArc(centreX, centreY, radius - 6, radius - 6, 0.0f,
                              startAngle, endAngle, true);

    // Gradient from cyan to white along the arc
    auto arcBounds = arcProgress.getBounds();
    juce::ColourGradient arcGradient(juce::Colour(0xff00d9ff), arcBounds.getX(),
                                     arcBounds.getCentreY(),
                                     juce::Colours::white, arcBounds.getRight(),
                                     arcBounds.getCentreY(), false);
    g.setGradientFill(arcGradient);
    g.strokePath(arcProgress, juce::PathStrokeType(6.0f)); // Thicker: 6px

    // Add glow to arc
    g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.3f));
    g.strokePath(arcProgress, juce::PathStrokeType(8.0f));
  }

  // Indicator line (brighter, thicker)
  juce::Path indicator;
  indicator.startNewSubPath(centreX, centreY);
  indicator.lineTo(centreX + radius * 0.65f * std::sin(angle),
                   centreY - radius * 0.65f * std::cos(angle));

  // Glow behind indicator
  g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.4f));
  g.strokePath(indicator,
               juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Main indicator
  g.setColour(juce::Colour(0xff00d9ff));
  g.strokePath(indicator,
               juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Center dot (shiny)
  juce::ColourGradient centerGradient(juce::Colours::white, centreX,
                                      centreY - 2, juce::Colour(0xff88ccff),
                                      centreX, centreY + 2, false);
  g.setGradientFill(centerGradient);
  g.fillEllipse(centreX - 4, centreY - 4, 8, 8);
}

void CustomKnobLookAndFeel::drawGlassCrystalKnob(juce::Graphics &g, int x,
                                                 int y, int width, int height,
                                                 float angle, float value) {
  auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = x + width * 0.5f;
  auto centreY = y + height * 0.5f;

  // Outer glow
  g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.15f));
  g.fillEllipse(centreX - radius - 3, centreY - radius - 3, (radius + 3) * 2,
                (radius + 3) * 2);

  // Main glass body with gradient
  juce::ColourGradient glassGradient(juce::Colour(0xffffffff).withAlpha(0.4f),
                                     centreX, centreY,
                                     juce::Colour(0xffb0d0e8).withAlpha(0.5f),
                                     centreX, centreY + radius, true);
  g.setGradientFill(glassGradient);
  g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

  // Inner glow
  juce::ColourGradient innerGlow(juce::Colour(0xffffffff).withAlpha(0.6f),
                                 centreX, centreY,
                                 juce::Colour(0xff00d9ff).withAlpha(0.1f),
                                 centreX, centreY + radius * 0.9f, true);
  g.setGradientFill(innerGlow);
  g.fillEllipse(centreX - radius * 0.9f, centreY - radius * 0.9f, radius * 1.8f,
                radius * 1.8f);

  // Specular highlight
  juce::ColourGradient specular(
      juce::Colour(0xffffffff).withAlpha(0.8f), centreX - radius * 0.3f,
      centreY - radius * 0.3f, juce::Colour(0xffffffff).withAlpha(0.0f),
      centreX - radius * 0.3f + 20, centreY - radius * 0.3f + 25, true);
  g.setGradientFill(specular);
  g.fillEllipse(centreX - radius * 0.5f, centreY - radius * 0.6f, 40, 50);

  // Glass edges
  g.setColour(juce::Colours::white.withAlpha(0.4f));
  g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2,
                1.0f);
  g.setColour(juce::Colour(0xffb0d0e8).withAlpha(0.6f));
  g.drawEllipse(centreX - radius + 1, centreY - radius + 1, radius * 2 - 2,
                radius * 2 - 2, 0.5f);

  // Etched indicator line (double layer for depth)
  juce::Path indicator;
  indicator.startNewSubPath(centreX, centreY);
  indicator.lineTo(centreX + radius * 0.8f * std::sin(angle),
                   centreY - radius * 0.8f * std::cos(angle));

  // White layer
  g.setColour(juce::Colours::white.withAlpha(0.7f));
  g.strokePath(indicator,
               juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Cyan layer
  g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.5f));
  g.strokePath(indicator,
               juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));
}
