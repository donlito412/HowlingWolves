#pragma once
#include <JuceHeader.h>

class FilterProcessor {
public:
  FilterProcessor();

  enum FilterType { LowPass = 0, HighPass, BandPass, Notch, Formant };

  void prepare(const juce::dsp::ProcessSpec &spec);
  void process(juce::AudioBuffer<float> &buffer);
  void reset();

  void setFilterType(FilterType type);
  void setCutoff(float cutoffHz);
  void setResonance(float resonance);
  void setVowel(float vowelPos); // 0.0 (A) to 1.0 (U)

private:
  struct FormantBand {
    juce::dsp::IIR::Filter<float> filter;
    float targetFreq = 1000.0f;
    float targetQ = 1.0f;
    float targetGain = 1.0f;
  };

  // 5 parallel bands for formants
  std::array<FormantBand, 5> formantBands;
  void updateFormantCoefficients();

  float vowelPosition = 0.0f; // 0.0 - 1.0
  juce::dsp::StateVariableTPTFilter<float> filter;
  FilterType currentType = LowPass;
  float sampleRate = 44100.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterProcessor)
};
