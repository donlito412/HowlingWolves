#pragma once
#include <JuceHeader.h>

class TransientShaper {
public:
  TransientShaper();

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  // Process a block
  void process(juce::AudioBuffer<float> &buffer);

  // Parameters
  // Amount: -1.0 (Soften) to 1.0 (Punch)
  void setAmount(float amount) { biteAmount.setTargetValue(amount); }

  // Speed definitions (optional control)
  void setAttackSpeed(float fastMs, float slowMs);

private:
  float sampleRate = 44100.0f;
  juce::LinearSmoothedValue<float> biteAmount;

  // Envelope followers
  // We can use simple one-pole filters for envelopes logic
  // env = prev + coeff * (in - prev)

  struct EnvelopeFollower {
    float value = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    void setCoefficients(float attackMs, float releaseMsArg, float sr) {
      attackCoeff = std::exp(-1000.0f / (attackMs * sr));
      releaseCoeff = std::exp(-1000.0f / (releaseMsArg * sr));
    }

    float process(float input) {
      float absIn = std::abs(input);
      if (absIn > value)
        value = attackCoeff * value + (1.0f - attackCoeff) * absIn;
      else
        value = releaseCoeff * value + (1.0f - releaseCoeff) * absIn;
      return value;
    }

    void reset() { value = 0.0f; }
  };

  // Per-channel state
  std::vector<EnvelopeFollower> fastEnvs;
  std::vector<EnvelopeFollower> slowEnvs;

  float fastAttackMs = 2.0f;
  float slowAttackMs = 20.0f; // Difference defines transient width
  float releaseMs = 100.0f;   // Generally longer
};
