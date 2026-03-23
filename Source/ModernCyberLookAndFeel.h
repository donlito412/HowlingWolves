#pragma once
#include <JuceHeader.h>

namespace WolfColors {
// Backgrounds
const juce::Colour BG_CAVE = juce::Colour(0xff0a0a0a);
const juce::Colour PANEL_DARK =
    juce::Colour(0x661a1a1a); // 40% opacity (Glass Look)
const juce::Colour PANEL_DARKER = juce::Colour(0x800f0f0f); // 50% opacity
const juce::Colour CONTROL_BG = juce::Colour(0xff252525);

// Borders
const juce::Colour BORDER_SUBTLE = juce::Colour(0x33ffffff); // 20% white
const juce::Colour BORDER_PANEL = juce::Colour(0x22ffffff);  // 13% white

// Text
const juce::Colour TEXT_PRIMARY = juce::Colour(0xffe8f0ff);   // Cool white
const juce::Colour TEXT_SECONDARY = juce::Colour(0xff88ccff); // Light cyan
const juce::Colour TEXT_DISABLED = juce::Colour(0xff555555);

// Accent - Cyan/Teal (Absynth-inspired)
const juce::Colour ACCENT_CYAN = juce::Colour(0xff00d9ff);
const juce::Colour ACCENT_TEAL = juce::Colour(0xff00b8b8);
const juce::Colour ACCENT_RED = juce::Colour(0xffff4444);
const juce::Colour ACCENT_GLOW = juce::Colour(0x4400d9ff); // 27% opacity

// Waveforms
const juce::Colour WAVE_CYAN = juce::Colour(0xff00d9ff);
const juce::Colour WAVE_GRID = juce::Colour(0x22ffffff);
} // namespace WolfColors

class ModernCyberLookAndFeel : public juce::LookAndFeel_V4 {
public:
  ModernCyberLookAndFeel();

  // Tab styling (Absynth-inspired)
  void drawTabButton(juce::TabBarButton &button, juce::Graphics &g,
                     bool isMouseOver, bool isMouseDown) override;

  void drawTabbedButtonBarBackground(juce::TabbedButtonBar &bar,
                                     juce::Graphics &g) override;

  int getTabButtonBestWidth(juce::TabBarButton &button, int tabDepth) override;

  // Button styling (flat, minimal)
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool isMouseOverButton, bool isButtonDown) override;

  // Slider styling (horizontal, cyan fill)
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override;

  // Knob styling (30px, cyan arc, minimal)
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  // Tooltip styling
  juce::Rectangle<int>
  getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
                   juce::Rectangle<int> parentArea) override;

  void drawTooltip(juce::Graphics &g, const juce::String &text, int width,
                   int height) override;

  // Custom ComboBox drawing to allow transparency
  void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &box) override;
};
