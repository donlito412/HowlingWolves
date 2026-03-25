#include "VerticalFaderLookAndFeel.h"

VerticalFaderLookAndFeel::VerticalFaderLookAndFeel() {
  setColour(juce::Slider::trackColourId, CYAN_PRIMARY);
  setColour(juce::Slider::thumbColourId, juce::Colour(0xff4a4a4a));
  setColour(juce::Slider::textBoxTextColourId, TEXT_PRIMARY);
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
}

void VerticalFaderLookAndFeel::drawLinearSlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider &slider) {

  if (style != juce::Slider::LinearVertical)
    return;

  // Track dimensions
  float trackWidth = 8.0f;
  float trackX = x + (width - trackWidth) * 0.5f;
  float trackY = y;
  float trackHeight = height;

  // Background track (dark)
  g.setColour(BG_TRACK);
  g.fillRoundedRectangle(trackX, trackY, trackWidth, trackHeight, 4.0f);

  // Filled track (cyan gradient from bottom to slider position)
  if (sliderPos < trackY + trackHeight) {
    float fillHeight = (trackY + trackHeight) - sliderPos;

    // Glow layer (wider, transparent)
    g.setColour(CYAN_PRIMARY.withAlpha(0.4f));
    g.fillRoundedRectangle(trackX - 2, sliderPos, trackWidth + 4, fillHeight,
                           5.0f);

    // Main fill with gradient
    juce::ColourGradient fillGradient(CYAN_PRIMARY, trackX, sliderPos,
                                      CYAN_DARK, trackX, trackY + trackHeight,
                                      false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(trackX, sliderPos, trackWidth, fillHeight, 4.0f);
  }

  // Thumb (rectangular handle)
  float thumbWidth = width * 0.8f;
  float thumbHeight = 12.0f;
  float thumbX = x + (width - thumbWidth) * 0.5f;
  float thumbY = sliderPos - thumbHeight * 0.5f;

  // Thumb shadow
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.fillRoundedRectangle(thumbX + 1, thumbY + 1, thumbWidth, thumbHeight, 2.0f);

  // Thumb body with gradient
  juce::ColourGradient thumbGradient(juce::Colour(0xff5a5a5a), thumbX, thumbY,
                                     juce::Colour(0xff3a3a3a), thumbX,
                                     thumbY + thumbHeight, false);
  g.setGradientFill(thumbGradient);
  g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

  // Thumb highlight
  g.setColour(juce::Colour(0xff6a6a6a));
  g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);
}

juce::Label *
VerticalFaderLookAndFeel::createSliderTextBox(juce::Slider &slider) {
  auto *label = new juce::Label();
  label->setJustificationType(juce::Justification::centred);
  label->setColour(juce::Label::textColourId, TEXT_PRIMARY);
  label->setColour(juce::Label::backgroundColourId,
                   juce::Colours::transparentBlack);
  label->setColour(juce::Label::outlineColourId,
                   juce::Colours::transparentBlack);
  label->setFont(juce::Font(11.0f, juce::Font::bold));
  return label;
}
