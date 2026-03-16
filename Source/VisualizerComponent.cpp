#include "VisualizerComponent.h"

VisualizerComponent::VisualizerComponent() : fifoBuffer(4096) {
  displayBuffer.setSize(1, 512);
  displayBuffer.clear();
  startTimerHz(60); // 60 FPS
}

VisualizerComponent::~VisualizerComponent() { stopTimer(); }

void VisualizerComponent::pushBuffer(const juce::AudioBuffer<float> &buffer) {
  if (buffer.getNumChannels() > 0) {
    // Threshold to prevent noise from triggering visualizer
    if (buffer.getMagnitude(0, 0, buffer.getNumSamples()) < 0.01f)
      return;

    auto *channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
      std::copy(channelData, channelData + size1, fifoBuffer.begin() + start1);
    if (size2 > 0)
      std::copy(channelData + size1, channelData + size1 + size2,
                fifoBuffer.begin() + start2);

    fifo.finishedWrite(size1 + size2);
  }
}

void VisualizerComponent::timerCallback() {
  int numSamples = displayBuffer.getNumSamples();
  int start1, size1, start2, size2;
  // Use fifo.getNumReady() to see if we have valid data waiting?
  // Juce AbstractFifo doesn't have "getNumReady" for read directly without
  // prepare.

  fifo.prepareToRead(numSamples, start1, size1, start2, size2);

  if (size1 + size2 > 0) {
    if (size1 > 0)
      displayBuffer.copyFrom(0, 0, fifoBuffer.data() + start1, size1);
    if (size2 > 0)
      displayBuffer.copyFrom(0, size1, fifoBuffer.data() + start2, size2);

    fifo.finishedRead(size1 + size2);
    sensitivity = 1.0f; // Reset sensitivity on new data
  } else {
    // No new audio data, decay the display buffer to zero
    displayBuffer.applyGain(0.85f); // Fast decay
  }

  repaint();
}

void VisualizerComponent::paint(juce::Graphics &g) {
  auto area = getLocalBounds().toFloat();

  // Background
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(area, 4.0f);
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(area, 4.0f, 1.0f);

  // Grid lines
  g.setColour(WolfColors::BORDER_PANEL);
  g.drawHorizontalLine(getHeight() / 2, 0, getWidth());

  // Waveform
  waveformPath.clear();
  auto *data = displayBuffer.getReadPointer(0);
  int numSamples = displayBuffer.getNumSamples();

  float xRatio = area.getWidth() / (float)numSamples;
  float yMid = area.getCentreY();
  float yScale = area.getHeight() * 0.4f; // Scale factor

  waveformPath.startNewSubPath(area.getX(), yMid);

  for (int i = 0; i < numSamples; ++i) {
    // Simple decimation or just drawing all points
    // For 512 samples/pixels, we can just draw lines
    float x = area.getX() + i * xRatio;
    float y = yMid - (data[i] * yScale);
    waveformPath.lineTo(x, y);
  }

  // Draw Glow
  g.setColour(WolfColors::ACCENT_GLOW);
  g.strokePath(waveformPath, juce::PathStrokeType(4.0f));

  // Draw Core line
  g.setColour(WolfColors::WAVE_CYAN);
  g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
}

void VisualizerComponent::resized() {}
