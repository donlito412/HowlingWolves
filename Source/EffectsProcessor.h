#pragma once

#include "TransientShaper.h"
#include <JuceHeader.h>

class EffectsProcessor {
public:
  enum class EffectType { Distortion, TransientShaper, Delay, Reverb };

  EffectsProcessor();
  ~EffectsProcessor();

  void prepare(juce::dsp::ProcessSpec &spec);
  void process(juce::AudioBuffer<float> &buffer);
  void reset();

  void setChainOrder(const std::array<EffectType, 4> &newOrder) {
    chainOrder = newOrder;
  }

  const std::array<EffectType, 4> &getChainOrder() const { return chainOrder; }

  void updateParameters(float distDrive, float distMix, float delayTime,
                        float delayFeedback, float delayMix, float reverbSize,
                        float reverbDamping, float reverbMix, float biteAmount);

private:
  // Helper for smoothing
  void generateRamp(juce::LinearSmoothedValue<float> &smoother, int numSamples);
  std::vector<float> rampBuffer;

  // --- Distortion ---
  // Use Waveshaper for standard tanh distortion (Josh Hodge style)
  juce::LinearSmoothedValue<float> distDriveParam;
  juce::LinearSmoothedValue<float> distMixParam;
  juce::dsp::WaveShaper<float> distortion;

  // --- Transient Shaper ---
  TransientShaper transientShaper;

  // --- Delay ---
  juce::LinearSmoothedValue<float> delayTimeParam;
  juce::LinearSmoothedValue<float> delayFeedbackParam;
  juce::LinearSmoothedValue<float> delayMixParam;

  // Choose Lagrange for better pitch shifting/modulation support, or Linear for
  // efficiency. Using Linear for standard echo.
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>
      delayLine;
  static constexpr float maxDelayTime = 2.0f;
  std::vector<float>
      delayFeedbackBuffer; // To store feedback values per channel

  // --- Reverb ---
  juce::dsp::Reverb reverb;
  juce::dsp::Reverb::Parameters reverbParams;
  juce::LinearSmoothedValue<float> reverbMixParam;

  double currentSampleRate = 44100.0;

  // Helper for Dry/Wet mixing
  // We'll do simple linear mix implementation inline for clarity

  void processDistortion(juce::AudioBuffer<float> &buffer);
  void processTransientShaper(juce::AudioBuffer<float> &buffer);
  void processDelay(juce::AudioBuffer<float> &buffer);
  void processReverb(juce::AudioBuffer<float> &buffer);

  std::array<EffectType, 4> chainOrder = {
      EffectType::Distortion, EffectType::TransientShaper, EffectType::Delay,
      EffectType::Reverb};

  // --- Metering ---
public:
  // Public for easy access by Editor
  std::atomic<float> eqLow{0.0f}, eqMid{0.0f}, eqHigh{0.0f};

  // --- New Effects ---
  void setHuntEnabled(bool enabled) { huntEnabled = enabled; }
  void setBitcrushEnabled(bool enabled) { bitcrushEnabled = enabled; }

private:
  bool huntEnabled = false;
  bool bitcrushEnabled = false;

  // Analysis Filters for Metering
  juce::dsp::StateVariableTPTFilter<float> meterFilterLow;
  juce::dsp::StateVariableTPTFilter<float> meterFilterMid;
  juce::dsp::StateVariableTPTFilter<float> meterFilterHigh;

  // Bitcrusher
  float bitcrushPhase = 0.0f;
  float lastCrushedSampleL = 0.0f;
  float lastCrushedSampleR = 0.0f;

  void processBitcrusher(juce::AudioBuffer<float> &buffer);
  void processMetering(const juce::AudioBuffer<float> &buffer);
};
