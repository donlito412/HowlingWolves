#include "LEDToggleButton.h"

LEDToggleButton::LEDToggleButton() { setClickingTogglesState(true); }

void LEDToggleButton::paintButton(juce::Graphics &g,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) {
  auto bounds = getLocalBounds().toFloat();
  bool isOn = getToggleState();

  // Background
  if (isOn) {
    // ON state: Cyan background with glow
    g.setColour(CYAN_PRIMARY.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.expanded(2), 3.0f);

    g.setColour(CYAN_PRIMARY);
    g.fillRoundedRectangle(bounds, 3.0f);
  } else {
    // OFF state: Dark background
    g.setColour(BG_OFF);
    g.fillRoundedRectangle(bounds, 3.0f);
  }

  // Border
  g.setColour(isOn ? CYAN_PRIMARY.brighter(0.3f) : juce::Colour(0xff4a4a4a));
  g.drawRoundedRectangle(bounds, 3.0f, 1.5f);

  // Hover effect
  if (shouldDrawButtonAsHighlighted && !isOn) {
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.fillRoundedRectangle(bounds, 3.0f);
  }

  // Text
  g.setColour(isOn ? TEXT_ON : TEXT_OFF);
  g.setFont(juce::Font(12.0f, juce::Font::bold));
  g.drawText(getButtonText(), bounds, juce::Justification::centred);

  // LED indicator (small dot in corner)
  if (isOn) {
    float ledSize = 4.0f;
    float ledX = bounds.getRight() - ledSize - 4;
    float ledY = bounds.getY() + 4;

    // LED glow
    g.setColour(CYAN_PRIMARY.withAlpha(0.5f));
    g.fillEllipse(ledX - 1, ledY - 1, ledSize + 2, ledSize + 2);

    // LED dot
    g.setColour(CYAN_PRIMARY.brighter(0.5f));
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
  }
}
