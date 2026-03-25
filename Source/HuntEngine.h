#pragma once
#include <JuceHeader.h>
#include <random>

class HuntEngine {
public:
  enum class Mode {
    Stalk, // Subtle variation (Analog drift, slight filter changes)
    Chase, // Moderate variation (New effects settings, LFO rates)
    Kill   // Aggressive variation (Total randomization)
  };

  HuntEngine();

  // Randomize parameters based on the selected mode
  void hunt(juce::AudioProcessorValueTreeState &apvts, Mode mode);

private:
  std::mt19937 randomGenerator;

  float getRandomFloat(float min, float max);
  bool flipCoin(float probability);

  // Helpers
  void randomizeParameter(juce::RangedAudioParameter *param, float minInfo,
                          float maxInfo, float variationAmount);
};
