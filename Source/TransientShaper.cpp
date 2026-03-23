#include "TransientShaper.h"

TransientShaper::TransientShaper() {}

void TransientShaper::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = (float)spec.sampleRate;

  fastEnvs.resize(spec.numChannels);
  slowEnvs.resize(spec.numChannels);

  setAttackSpeed(fastAttackMs, slowAttackMs);
  reset();
}

void TransientShaper::reset() {
  for (auto &env : fastEnvs)
    env.reset();
  for (auto &env : slowEnvs)
    env.reset();
}

void TransientShaper::setAttackSpeed(float fastMs, float slowMs) {
  fastAttackMs = fastMs;
  slowAttackMs = slowMs;

  for (auto &env : fastEnvs)
    env.setCoefficients(fastAttackMs, releaseMs, sampleRate);
  for (auto &env : slowEnvs)
    env.setCoefficients(slowAttackMs, releaseMs, sampleRate);
}

void TransientShaper::process(juce::AudioBuffer<float> &buffer) {
  // Always process if smoothing or active
  if (std::abs(biteAmount.getCurrentValue()) < 0.01f &&
      !biteAmount.isSmoothing() &&
      std::abs(biteAmount.getTargetValue()) < 0.01f)
    return;

  auto numChannels = buffer.getNumChannels();
  auto numSamples = buffer.getNumSamples();

  // Resize safety
  if (fastEnvs.size() != numChannels) {
    fastEnvs.resize(numChannels);
    slowEnvs.resize(numChannels);
    setAttackSpeed(fastAttackMs, slowAttackMs);
    // Reset envs
    for (auto &env : fastEnvs)
      env.reset();
    for (auto &env : slowEnvs)
      env.reset();
  }

  auto *ch0 = buffer.getWritePointer(0);
  auto *ch1 = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    float bite = biteAmount.getNextValue();

    // Ch 0
    {
      float input = ch0[i];
      float fast = fastEnvs[0].process(input);
      float slow = slowEnvs[0].process(input);
      float transient = fast - slow;
      // Boosted intensity for more noticeable "Bite"
      float gainChange = 1.0f + (transient * bite * 6.0f);
      gainChange = juce::jlimit(0.1f, 4.0f, gainChange);
      ch0[i] = input * gainChange;
    }

    // Ch 1
    if (ch1) {
      float input = ch1[i];
      float fast = fastEnvs[1].process(input);
      float slow = slowEnvs[1].process(input);
      float transient = fast - slow;
      float gainChange = 1.0f + (transient * bite * 6.0f);
      gainChange = juce::jlimit(0.1f, 4.0f, gainChange);
      ch1[i] = input * gainChange;
    }
  }
}
