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

  // Wire this component to the Processor's lock-free FIFO.
  // Call once after construction (message thread, before audio starts).
  // Both pointers must remain valid for the lifetime of this component;
  // using the Processor's own members satisfies that requirement.
  void setSourceFifo(juce::AbstractFifo *fifo, const float *data);

private:
  // Source FIFO owned by the Processor — never null-dereferenced because
  // timerCallback checks before reading.
  juce::AbstractFifo *sourceFifo = nullptr;
  const float *sourceFifoData = nullptr;

  juce::AudioBuffer<float> displayBuffer;

  // Path for drawing
  juce::Path waveformPath;
  float sensitivity = 1.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualizerComponent)
};
