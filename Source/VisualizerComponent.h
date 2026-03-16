#pragma once

#include "ModernCyberLookAndFeel.h"
#include <JuceHeader.h>

class VisualizerComponent : public juce::Component, public juce::Timer {
public:
  VisualizerComponent();
  ~VisualizerComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  void pushBuffer(const juce::AudioBuffer<float> &buffer);

private:
  juce::AudioBuffer<float> displayBuffer;
  juce::AbstractFifo fifo{4096};
  std::vector<float> fifoBuffer;

  // Path for drawing
  juce::Path waveformPath;
  float sensitivity = 1.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualizerComponent)
};
