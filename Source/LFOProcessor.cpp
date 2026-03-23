#include "LFOProcessor.h"

LFOProcessor::LFOProcessor() {}

void LFOProcessor::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  phaseIncrement = currentRate / sampleRate;
  phase = 0.0;
}

void LFOProcessor::reset() { phase = 0.0; }

float LFOProcessor::getNextSample() {
  float output = 0.0f;

  switch (currentWaveform) {
  case Sine:
    output = std::sin(phase * juce::MathConstants<double>::twoPi);
    break;

  case Square:
    output = (phase < 0.5) ? 1.0f : -1.0f;
    break;

  case Triangle:
    if (phase < 0.5)
      output = -1.0f + 4.0f * phase;
    else
      output = 3.0f - 4.0f * phase;
    break;
  }

  // Advance phase
  phase += phaseIncrement;
  if (phase >= 1.0)
    phase -= 1.0;

  // Apply depth (0.0 to 1.0 range)
  return output * currentDepth;
}

void LFOProcessor::setWaveform(Waveform wave) { currentWaveform = wave; }

void LFOProcessor::setRate(float rateHz) {
  currentRate = rateHz;
  phaseIncrement = currentRate / currentSampleRate;
}

void LFOProcessor::setDepth(float depth) {
  currentDepth = juce::jlimit(0.0f, 1.0f, depth);
}

void LFOProcessor::setTarget(Target target) { currentTarget = target; }
