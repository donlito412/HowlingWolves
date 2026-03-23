#include "HuntEngine.h"

HuntEngine::HuntEngine() {
  // initialize random generator with a safer seed (e.g. clock)
  // to avoid potential random_device issues on some systems
  std::random_device rd;
  unsigned int seed = 0;
  try {
    seed = rd();
  } catch (...) {
    seed = static_cast<unsigned int>(std::time(nullptr));
  }
  randomGenerator.seed(seed);
}

float HuntEngine::getRandomFloat(float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(randomGenerator);
}

bool HuntEngine::flipCoin(float probability) {
  std::bernoulli_distribution dist(probability);
  return dist(randomGenerator);
}

void HuntEngine::randomizeParameter(juce::RangedAudioParameter *param,
                                    float minInfo, float maxInfo,
                                    float variationAmount) {
  if (!param)
    return;

  // Get current normalized value
  float currentNorm = param->getValue();
  float currentReal = param->convertFrom0to1(currentNorm);

  // Calculate new real value based on variation
  // variationAmount is a percentage of the total range (approx) or a multiplier

  // Simple approach: New = Old + Random(-Variation, +Variation) * Range
  float range = maxInfo - minInfo;
  float delta = range * variationAmount;
  float randomDelta = getRandomFloat(-delta, delta);

  float newReal = juce::jlimit(minInfo, maxInfo, currentReal + randomDelta);

  // Apply
  param->setValueNotifyingHost(param->convertTo0to1(newReal));
}

void HuntEngine::hunt(juce::AudioProcessorValueTreeState &apvts, Mode mode) {
  // Define intensity
  float variation = 0.0f;
  float probability = 0.0f;

  switch (mode) {
  case Mode::Stalk:
    variation = 0.05f;  // 5% variation
    probability = 0.3f; // Change 30% of params
    break;
  case Mode::Chase:
    variation = 0.3f;
    probability = 0.6f;
    break;
  case Mode::Kill:
    variation = 1.0f;   // Full range
    probability = 1.0f; // Change almost everything
    break;
  }

  // Iterate through parameters
  // We need to know Parameter IDs or categorize them.
  // Implementing a rough categorization based on ID string content is pragmatic
  // for now.

  // Iterate through parameters
  // Use apvts.processor to access parameters (AudioProcessor reference is
  // public in APVTS)
  auto &processor = apvts.processor;

  for (auto *param : processor.getParameters()) {
    if (auto *p = dynamic_cast<juce::RangedAudioParameter *>(param)) {
      juce::String id = p->getParameterID();

      // Skip critical global params if needed (like Gain, or maybe not?)
      if (id == "gain")
        continue;

      // Determine range (we have to know it, or rely on NormalisableRange if
      // accessible via RangedAudioParameter) convertFrom0to1(0) gives min,
      // convertFrom0to1(1) gives max usually
      float minVal = p->convertFrom0to1(0.0f);
      float maxVal = p->convertFrom0to1(1.0f);

      // Filter Params logic
      if (id.containsIgnoreCase("filter")) {
        if (flipCoin(probability)) {
          randomizeParameter(p, minVal, maxVal, variation);
        }
      }
      // Envelope logic
      else if (id.containsIgnoreCase("attack") ||
               id.containsIgnoreCase("decay") ||
               id.containsIgnoreCase("sustain") ||
               id.containsIgnoreCase("release")) {
        if (mode == Mode::Kill ||
            flipCoin(probability * 0.5f)) { // Be careful with envelope
          randomizeParameter(p, minVal, maxVal, variation);
        }
      }
      // Effects logic
      else if (id.containsIgnoreCase("dist") ||
               id.containsIgnoreCase("delay") ||
               id.containsIgnoreCase("reverb") ||
               id.containsIgnoreCase("bite")) {
        if (flipCoin(probability)) {
          randomizeParameter(p, minVal, maxVal, variation);
        }
      }
      // LFO
      else if (id.containsIgnoreCase("lfo")) {
        if (flipCoin(probability)) {
          randomizeParameter(p, minVal, maxVal, variation);
        }
      }
      // Sample
      else if (id.containsIgnoreCase("sample") || id.contains("tune")) {
        if (mode == Mode::Kill) {
          randomizeParameter(p, minVal, maxVal, variation);
        }
      }
    }
  }
}
