#pragma once
#include "ObsidianLookAndFeel.h"
#include <JuceHeader.h>

// A simple utility component to check "Obsidian Glass" backgrounds
class GlassPanel : public juce::Component {
public:
  GlassPanel() {
    // Transparent so the parent background (Cave Image) shows through
    setOpaque(false);
  }

  void paint(juce::Graphics &g) override {
    // Delegate drawing to the static method in LookAndFeel for consistency
    ObsidianLookAndFeel::drawGlassPanel(g, getLocalBounds());
  }
};
