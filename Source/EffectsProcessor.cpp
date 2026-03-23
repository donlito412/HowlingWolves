#include "EffectsProcessor.h"

EffectsProcessor::EffectsProcessor() {
  // Initialize Waveshaper with tanh transfer function
  distortion.functionToUse = [](float x) { return std::tanh(x); };

  // Delay setup
  delayLine.setMaximumDelayInSamples(44100 * 2);
}

EffectsProcessor::~EffectsProcessor() {}

void EffectsProcessor::prepare(juce::dsp::ProcessSpec &spec) {
  currentSampleRate = spec.sampleRate;

  // Prepare Distortion
  distortion.prepare(spec);
  distDriveParam.reset(currentSampleRate, 0.05); // 50ms ramp
  distMixParam.reset(currentSampleRate, 0.05);

  // Prepare Transient Shaper
  transientShaper.prepare(spec);

  // Prepare Delay
  delayLine.prepare(spec);
  // Important: Set correct max delay based on sample rate
  delayLine.setMaximumDelayInSamples(
      static_cast<int>(spec.sampleRate * maxDelayTime));
  delayLine.reset();

  delayTimeParam.reset(currentSampleRate, 0.05);
  delayFeedbackParam.reset(currentSampleRate, 0.05);
  delayMixParam.reset(currentSampleRate, 0.05);
  delayWidthParam.reset(currentSampleRate, 0.05);

  // Prepare Reverb
  reverb.prepare(spec);
  reverb.reset();
  reverbMixParam.reset(currentSampleRate, 0.05);

  // Prepare Analysis Filters
  meterFilterLow.prepare(spec);
  meterFilterLow.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  meterFilterLow.setCutoffFrequency(300.0f); // Low band < 300Hz

  meterFilterMid.prepare(spec);
  meterFilterMid.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
  meterFilterMid.setCutoffFrequency(1000.0f); // Mid band center 1kHz

  meterFilterHigh.prepare(spec);
  meterFilterHigh.setType(juce::dsp::StateVariableTPTFilterType::highpass);
  meterFilterHigh.setCutoffFrequency(5000.0f); // High band > 5kHz

  // Reserve ramp buffer
  rampBuffer.reserve(spec.maximumBlockSize);

  // Preallocate metering scratch buffers
  const int channels = (int)spec.numChannels;
  const int maxBlock = (int)spec.maximumBlockSize;
  meterScratchLow.setSize(channels, maxBlock, false, false, true);
  meterScratchMid.setSize(channels, maxBlock, false, false, true);
  meterScratchHigh.setSize(channels, maxBlock, false, false, true);
}

void EffectsProcessor::reset() {
  distortion.reset();
  transientShaper.reset();
  delayLine.reset();
  reverb.reset();

  meterFilterLow.reset();
  meterFilterMid.reset();
  meterFilterHigh.reset();
  bitcrushPhase = 0.0f;
  lastCrushedSampleL = 0.0f;
  lastCrushedSampleR = 0.0f;

  // Reset smoothers to target ?? No, usually just keep current.
}

void EffectsProcessor::updateParameters(float distDrive, float distMix,
                                        float delayTime, float delayFeedback,
                                        float delayMix, float delayWidth,
                                        float reverbSize, float reverbDecay,
                                        float reverbDamping, float reverbMix,
                                        float biteAmount) {
  distDriveParam.setTargetValue(distDrive);
  distMixParam.setTargetValue(distMix);
  transientShaper.setAmount(biteAmount); // Shaper handles its own smoothing

  delayTimeParam.setTargetValue(delayTime);
  delayFeedbackParam.setTargetValue(delayFeedback);
  delayMixParam.setTargetValue(delayMix);
  delayWidthParam.setTargetValue(delayWidth);

  // Map Reverb params
  // JUCE Reverb doesn't expose a direct "decay time" parameter.
  // We map DECAY -> roomSize and SIZE -> stereo width to keep both controls
  // meaningful and independent.
  reverbParams.roomSize = reverbDecay;
  reverbParams.damping = reverbDamping;

  // Revert to internal mixing to ensure Dry/Wet balance works without temp
  // buffer
  reverbParams.wetLevel = reverbMix;
  reverbParams.dryLevel = 1.0f - reverbMix;

  reverbParams.width = reverbSize;
  reverbParams.freezeMode = 0.0f;

  reverb.setParameters(reverbParams);
  reverbMixParam.setTargetValue(
      reverbMix); // Keep for the on/off check in processReverb
}

void EffectsProcessor::process(juce::AudioBuffer<float> &buffer) {
  juce::ScopedNoDenormals noDenormals;

  for (auto effect : chainOrder) {
    switch (effect) {
    case EffectType::Distortion:
      processDistortion(buffer);
      // Bitcrusher is logically part of Distortion block in this context
      if (bitcrushEnabled) {
        processBitcrusher(buffer);
      }
      break;
    case EffectType::TransientShaper:
      processTransientShaper(buffer);
      break;
    case EffectType::Delay:
      processDelay(buffer);
      break;
    case EffectType::Reverb:
      processReverb(buffer);
      break;
    }
  }

  // Metering after all effects (or strictly after distortion? User said
  // "DISTORTION EQ") But if it's "Effects Tab" metering, user probably wants to
  // see the overall spectrum. However, "DISTORTION EQ" implies it visualizes
  // the distortion character. Let's measure effectively after the
  // Distortion/Shaper block, but before Delay/Reverb to be accurate "Distortion
  // EQ" Wait, the loop processes in mutable order. I should probably analyze
  // *at the end of processDistortion*? But if the chain order is reversed
  // (Reverb -> Distortion), measuring at the end of the whole chain is safer to
  // be visible. But strictly "Distortion EQ" implies shaping. I will measure at
  // the VERY END of the chain so the user sees "Output Spectrum".
  processMetering(buffer);
}

void EffectsProcessor::processBitcrusher(juce::AudioBuffer<float> &buffer) {
  // Simple Bitcrush/Downsample
  // Downsample factor: 4x (roughly 11kHz SR)
  // Bit depth: 8 bit via truncation
  float downsampleFactor = 4.0f;
  int numSamples = buffer.getNumSamples();
  auto *ch0 = buffer.getWritePointer(0);
  auto *ch1 = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    bitcrushPhase += 1.0f;
    if (bitcrushPhase >= downsampleFactor) {
      bitcrushPhase -= downsampleFactor;

      // Quantize
      float sample = ch0[i];
      float levels = 256.0f; // 8 bit
      sample = std::round(sample * levels) / levels;
      lastCrushedSampleL = sample;

      if (ch1) {
        float sampleR = ch1[i];
        sampleR = std::round(sampleR * levels) / levels;
        lastCrushedSampleR = sampleR;
      }
    }

    ch0[i] = lastCrushedSampleL;
    if (ch1)
      ch1[i] = lastCrushedSampleR;
  }
}

void EffectsProcessor::processMetering(const juce::AudioBuffer<float> &buffer) {
  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();

  if (numSamples <= 0 || numChannels <= 0)
    return;

  // Ensure scratch buffers are big enough (should already be sized in prepare)
  if (meterScratchLow.getNumSamples() < numSamples ||
      meterScratchLow.getNumChannels() < numChannels) {
    meterScratchLow.setSize(numChannels, numSamples, false, false, true);
    meterScratchMid.setSize(numChannels, numSamples, false, false, true);
    meterScratchHigh.setSize(numChannels, numSamples, false, false, true);
  }

  meterScratchLow.makeCopyOf(buffer, true);
  meterScratchMid.makeCopyOf(buffer, true);
  meterScratchHigh.makeCopyOf(buffer, true);

  {
    juce::dsp::AudioBlock<float> lowBlock(meterScratchLow);
    juce::dsp::ProcessContextReplacing<float> lowCtx(lowBlock);
    meterFilterLow.process(lowCtx);
    eqLow = meterScratchLow.getRMSLevel(0, 0, numSamples) * 5.0f;
  }

  {
    juce::dsp::AudioBlock<float> midBlock(meterScratchMid);
    juce::dsp::ProcessContextReplacing<float> midCtx(midBlock);
    meterFilterMid.process(midCtx);
    eqMid = meterScratchMid.getRMSLevel(0, 0, numSamples) * 5.0f;
  }

  {
    juce::dsp::AudioBlock<float> highBlock(meterScratchHigh);
    juce::dsp::ProcessContextReplacing<float> highCtx(highBlock);
    meterFilterHigh.process(highCtx);
    eqHigh = meterScratchHigh.getRMSLevel(0, 0, numSamples) * 5.0f;
  }
}

void EffectsProcessor::processDistortion(juce::AudioBuffer<float> &buffer) {
  auto totalNumInputChannels = buffer.getNumChannels();
  auto numSamples = buffer.getNumSamples();

  auto *ch0 = buffer.getWritePointer(0);
  auto *ch1 = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    float drive = distDriveParam.getNextValue();
    float mix = distMixParam.getNextValue();

    // Hunt Mode Logic ("Hunt" button essentially boosts Input Drive)
    if (huntEnabled) {
      drive = std::min(drive * 1.5f + 0.2f, 1.0f);
    }

    float gain = 1.0f + (drive * 49.0f);

    float dry0 = ch0[i];
    float wet0 = std::tanh(dry0 * gain);
    ch0[i] = (dry0 * (1.0f - mix)) + (wet0 * mix);

    if (ch1) {
      float dry1 = ch1[i];
      float wet1 = std::tanh(dry1 * gain);
      ch1[i] = (dry1 * (1.0f - mix)) + (wet1 * mix);
    }
  }
}

void EffectsProcessor::processTransientShaper(
    juce::AudioBuffer<float> &buffer) {
  transientShaper.process(buffer);
}

void EffectsProcessor::processDelay(juce::AudioBuffer<float> &buffer) {
  auto totalNumInputChannels = buffer.getNumChannels();
  auto numSamples = buffer.getNumSamples();

  auto *ch0 = buffer.getWritePointer(0);
  auto *ch1 = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    float time = delayTimeParam.getNextValue();
    float fdbk = delayFeedbackParam.getNextValue();
    float mix = delayMixParam.getNextValue();
    float width = delayWidthParam.getNextValue(); // 0 = mono, 1 = ping-pong

    float delaySamples = time * (float)currentSampleRate;
    delaySamples = juce::jlimit(1.0f, (float)(maxDelayTime * currentSampleRate),
                                delaySamples);
    delayLine.setDelay(delaySamples);

    float inputL = ch0[i];
    float inputR = ch1 ? ch1[i] : inputL;

    // Pop the current delayed output from both lines.
    float delayedL = delayLine.popSample(0, -1.0f);
    float delayedR = ch1 ? delayLine.popSample(1, -1.0f) : delayedL;

    // Width crossfeed:
    //   width = 0  →  both lines fed with the mono sum  (mono delay)
    //   width = 1  →  L line fed with R input, R line fed with L input
    //                 (classic ping-pong)
    float monoFeed = (inputL + inputR) * 0.5f;
    float feedL = monoFeed + (inputR - monoFeed) * width;
    float feedR = monoFeed + (inputL - monoFeed) * width;

    delayLine.pushSample(0, feedL + delayedL * fdbk);
    if (ch1)
      delayLine.pushSample(1, feedR + delayedR * fdbk);

    ch0[i] = inputL + delayedL * mix;
    if (ch1)
      ch1[i] = inputR + delayedR * mix;
  }
}

void EffectsProcessor::processReverb(juce::AudioBuffer<float> &buffer) {
  // Use TargetValue because we are using block-based mixing via setParameters,
  // so the Smoother isn't technically advanced per sample, but Target holds
  // current setting.
  if (reverbMixParam.getTargetValue() > 0.0f) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
  }
}
