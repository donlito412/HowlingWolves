#include "FilterProcessor.h"

// Approximate Formant Frequencies (Male)
// Vowel: A, E, I, O, U
// Each has F1, F2, F3, F4, F5
const float formantFreqs[5][5] = {
    {730.0f, 1090.0f, 2440.0f, 3500.0f, 4500.0f}, // A
    {530.0f, 1840.0f, 2480.0f, 3500.0f, 4500.0f}, // E
    {270.0f, 2290.0f, 3010.0f, 3500.0f, 4500.0f}, // I
    {570.0f, 840.0f, 2410.0f, 3500.0f, 4500.0f},  // O
    {300.0f, 870.0f, 2240.0f, 3500.0f, 4500.0f}   // U
};

// Gains for bands (empirically tweaked for balance)
const float formantGains[5][5] = {
    {1.0f, 0.5f, 0.2f, 0.1f, 0.05f}, // A
    {1.0f, 0.3f, 0.2f, 0.1f, 0.05f}, // E
    {0.8f, 1.0f, 0.2f, 0.1f, 0.05f}, // I
    {1.0f, 0.6f, 0.2f, 0.1f, 0.05f}, // O
    {1.0f, 0.5f, 0.1f, 0.1f, 0.05f}  // U
};

FilterProcessor::FilterProcessor() {
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  // Default formant init
  updateFormantCoefficients();
}

void FilterProcessor::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = (float)spec.sampleRate;
  filter.prepare(spec);
  filter.reset();

  // Prepare formant bands
  for (auto &band : formantBands) {
    band.filter.prepare(spec);
    band.filter.reset();
  }
}

void FilterProcessor::process(juce::AudioBuffer<float> &buffer) {
  if (currentType == Formant) {
    // Parallel implementation
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // We need a scratch buffer to sum into, because we can't process in-place
    // effectively in parallel logic without copying the input first. Easiest:
    // Copy input to scratch, clear output, add formatted bands to output.

    juce::AudioBuffer<float> scratchBuffer;
    scratchBuffer.makeCopyOf(buffer);
    buffer.clear();

    for (int i = 0; i < 5; ++i) {
      // Create a temp copy for this band processing
      juce::AudioBuffer<float> bandBuffer;
      bandBuffer.makeCopyOf(scratchBuffer);

      juce::dsp::AudioBlock<float> block(bandBuffer);
      juce::dsp::ProcessContextReplacing<float> context(block);
      formantBands[i].filter.process(context);

      // Apply Gain
      bandBuffer.applyGain(formantBands[i].targetGain);

      // Sum into main buffer
      for (int ch = 0; ch < numChannels; ++ch) {
        buffer.addFrom(ch, 0, bandBuffer, ch, 0, numSamples);
      }
    }

  } else {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
  }
}

void FilterProcessor::reset() {
  filter.reset();
  for (auto &band : formantBands)
    band.filter.reset();
}

void FilterProcessor::setFilterType(FilterType type) {
  currentType = type;

  switch (type) {
  case LowPass:
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    break;
  case HighPass:
    filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    break;
  case BandPass:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    break;
  case Notch:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    break;
  case Formant:
    updateFormantCoefficients();
    break;
  }
}

void FilterProcessor::setCutoff(float cutoffHz) {
  filter.setCutoffFrequency(cutoffHz);
}

void FilterProcessor::setResonance(float resonance) {
  float q = 0.5f + resonance * 9.5f;
  filter.setResonance(q);

  // Also update formant Q if we want global Q control
  // For formants, usually Q is fixed or high, but let's allow "singing" vs
  // "mumbling"
  for (auto &band : formantBands) {
    band.targetQ = q * 2.0f; // Formants usually fairly resonant
  }
  updateFormantCoefficients();
}

void FilterProcessor::setVowel(float vowelPos) {
  if (std::abs(vowelPosition - vowelPos) > 0.001f) {
    vowelPosition =
        juce::jlimit(0.0f, 4.0f, vowelPos * 4.0f); // Map 0-1 to 0-4 range
    updateFormantCoefficients();
  }
}

void FilterProcessor::updateFormantCoefficients() {
  // Interpolate between vowels
  // 0 = A, 1 = E, 2 = I, 3 = O, 4 = U

  int index1 = (int)vowelPosition;
  int index2 = std::min(index1 + 1, 4);
  float alpha = vowelPosition - (float)index1;

  for (int i = 0; i < 5; ++i) {
    float f1 = formantFreqs[index1][i];
    float f2 = formantFreqs[index2][i];
    float freq = f1 + (f2 - f1) * alpha;

    float g1 = formantGains[index1][i];
    float g2 = formantGains[index2][i];
    float gain = g1 + (g2 - g1) * alpha;

    // Update filters
    formantBands[i].filter.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeBandPass(
            sampleRate, freq, formantBands[i].targetQ);
    formantBands[i].targetGain = gain;
  }
}
