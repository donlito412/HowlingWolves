#include "VisualizerComponent.h"

VisualizerComponent::VisualizerComponent() {
  displayBuffer.setSize(1, 512);
  displayBuffer.clear();
  startTimerHz(60);
}

VisualizerComponent::~VisualizerComponent() { stopTimer(); }

void VisualizerComponent::setSourceFifo(juce::AbstractFifo *fifo,
                                        const float *data) {
  // Called once from the message thread before audio starts — no lock needed.
  sourceFifo = fifo;
  sourceFifoData = data;
}

void VisualizerComponent::timerCallback() {
  if (sourceFifo == nullptr) {
    // Not wired yet — just decay to silence
    displayBuffer.applyGain(0.85f);
    repaint();
    return;
  }

  int numSamples = displayBuffer.getNumSamples();
  int start1, size1, start2, size2;
  sourceFifo->prepareToRead(numSamples, start1, size1, start2, size2);

  if (size1 + size2 > 0) {
    if (size1 > 0)
      displayBuffer.copyFrom(0, 0, sourceFifoData + start1, size1);
    if (size2 > 0)
      displayBuffer.copyFrom(0, size1, sourceFifoData + start2, size2);

    sourceFifo->finishedRead(size1 + size2);
    sensitivity = 1.0f;
  } else {
    displayBuffer.applyGain(0.85f);
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
