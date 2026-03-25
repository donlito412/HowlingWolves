#include "SynthEngine.h"

//==============================================================================
// HowlingVoice
//==============================================================================

HowlingVoice::HowlingVoice() {
  // Initialize ADSR with default
  adsr.setSampleRate(44100.0); // Will be updated in prepare
  adsrParams = {0.1f, 0.1f, 1.0f, 0.1f};
  adsr.setParameters(adsrParams);

  // Initialize Mod ADSR
  modAdsr.setSampleRate(44100.0);
  modAdsrParams = {0.1f, 0.1f, 1.0f, 0.1f};
  modAdsr.setParameters(modAdsrParams);
}

void HowlingVoice::updateModADSR(float attack, float decay, float sustain,
                                 float release, float amount, int target) {
  constexpr float tMin = 0.002f;
  modAdsrParams.attack = juce::jmax(tMin, attack);
  modAdsrParams.decay = juce::jmax(tMin, decay);
  modAdsrParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
  modAdsrParams.release = juce::jmax(tMin, release);
  modAdsr.setParameters(modAdsrParams);

  modAmount = amount;
  modTarget = target;
}

void HowlingVoice::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = 1; // Mono voice

  filter.prepare(spec);
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

  lfoSampleRate = sampleRate;
  lfoPhaseAcc = 0.0f;

  adsr.setSampleRate(sampleRate);

  // Prepare crossover filter for Bass (120Hz)
  crossoverFilter.prepare(spec);
  crossoverFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
  crossoverFilter.setCutoffFrequency(120.0f);

  // Resize temp buffer for processing
  tempBuffer.setSize(1, samplesPerBlock); // Mono voice
  bassHighBuffer.setSize(1, samplesPerBlock);
}

void HowlingVoice::updateFilter(float cutoff, float resonance, int filterType) {
  baseCutoff = cutoff;
  baseResonance = resonance;
  filter.setCutoffFrequency(cutoff);
  filter.setResonance(resonance);

  switch (filterType) {
  case 0:
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    isNotch = false;
    break;
  case 1:
    filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    isNotch = false;
    break;
  case 2:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    isNotch = false;
    break;
  case 3:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    isNotch = true;
    break;
  default:
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    isNotch = false;
    break;
  }
}

void HowlingVoice::updateLFO(float rate, float depth, float phase01) {
  lfoRate = rate;
  lfoIncrement = (float)(juce::MathConstants<double>::twoPi * (double)rate /
                         (lfoSampleRate > 1.0 ? lfoSampleRate : 44100.0));
  lfoDepth = depth;
  lfoPhaseOffset =
      juce::jlimit(0.0f, 1.0f, phase01) * juce::MathConstants<float>::twoPi;
}

void HowlingVoice::updateADSR(float attack, float decay, float sustain,
                              float release) {
  constexpr float tMin = 0.002f;
  adsrParams.attack = juce::jmax(tMin, attack);
  adsrParams.decay = juce::jmax(tMin, decay);
  adsrParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
  adsrParams.release = juce::jmax(tMin, release);
  adsr.setParameters(adsrParams);
}

void HowlingVoice::updateSampleParams(float tune, float sampleStart,
                                      float sampleEnd, bool loop) {
  tuneSemitones = tune;
  sampleStartPercent = sampleStart;
  sampleEndPercent = sampleEnd;
  isLooping = loop;
}

void HowlingVoice::setPan(float newPan) { pan = newPan; }

void HowlingVoice::setAmpVelocity(float amount01) {
  ampVelocityAmount = juce::jlimit(0.0f, 1.0f, amount01);
}

void HowlingVoice::setFilterDrive(float drive01) {
  filterDrive = juce::jlimit(0.0f, 1.0f, drive01);
}

void HowlingVoice::setModSmooth(float smooth01) {
  modSmooth = juce::jlimit(0.0f, 1.0f, smooth01);
}

void HowlingVoice::setLFOPhase(float phase01) {
  lfoPhaseOffset =
      juce::jlimit(0.0f, 1.0f, phase01) * juce::MathConstants<float>::twoPi;
}

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  // Check if it's Bass or One-Shot
  isCurrentSoundBass = false;
  isCurrentSoundOneShot = false;

  if (auto *hs = dynamic_cast<HowlingSound *>(sound)) {
    isCurrentSoundBass = hs->isBassSample();
    isCurrentSoundOneShot = hs->isOneShotSample();
  }

  // 1. Base startNote
  juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                currentPitchWheelPosition);

  crossoverFilter.reset();

  noteVelocity = juce::jlimit(0.0f, 1.0f, velocity);
  adsr.noteOn();
  modAdsr.noteOn(); // Trigger Mod Env
  filter.reset();
  lfoPhaseAcc = 0.0f;
  smoothedModEnv = 0.0f;
}

void HowlingVoice::stopNote(float velocity, bool allowTailOff) {
  // Immediate stop (allNotesOff false, sample swap): must run for one-shots too,
  // or voices keep playing and hold pointers into freed SamplerSound memory.
  if (!allowTailOff) {
    adsr.reset();
    modAdsr.reset();
    juce::SamplerVoice::stopNote(velocity, false);
    return;
  }

  // One-shot: natural decay; key-up does not start ADSR release.
  if (isCurrentSoundOneShot)
    return;

  adsr.noteOff();
  modAdsr.noteOff();
  juce::SamplerVoice::stopNote(velocity, true);
}

void HowlingVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample, int numSamples) {
  if (!isVoiceActive())
    return;

  if (tempBuffer.getNumSamples() < numSamples) {
    tempBuffer.setSize(1, numSamples, false, false, true);
  }
  tempBuffer.clear();

  // 1. Render Raw Sample
  juce::SamplerVoice::renderNextBlock(tempBuffer, 0, numSamples);

  // 2. ADSR
  adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

  // Calculate Mod Envelope Value (per block implies stepped, per sample is
  // better) We'll calculate per sample for Filter/Audio targets But for
  // optimization, let's keep it simple-ish or do per-sample loop.

  auto *bufferData = tempBuffer.getWritePointer(0);

  // 3. Filter Processing & Mod Env Application
  for (int i = 0; i < numSamples; ++i) {
    float lfoValue = std::sin(lfoPhaseAcc + lfoPhaseOffset);
    lfoPhaseAcc += lfoIncrement;
    if (lfoPhaseAcc >= juce::MathConstants<float>::twoPi)
      lfoPhaseAcc -= juce::MathConstants<float>::twoPi;

    float modEnvVal = modAdsr.getNextSample(); // 0..1
    // Simple one-pole smoothing: 0=more smoothing, 1=less smoothing
    float alpha = 0.02f + (1.0f - modSmooth) * 0.18f; // 0.02..0.20
    smoothedModEnv += (modEnvVal - smoothedModEnv) * alpha;
    modEnvVal = smoothedModEnv;

    // Calculate effective LFO + Mod modulation
    // Mod Target 0: Cutoff (Default)
    // We mix LFO and Mod Env.

    // Base cutoff modulation from LFO
    float combinedMod = (lfoValue * lfoDepth);

    // Add Mod Env if Target is Cutoff
    if (modTarget == 0) {
      // Mod Amount is 0.0-1.0.
      // Let's say max amount is +/- 2 octaves or similar.
      // Or simply scale like LFO.
      combinedMod += (modEnvVal * modAmount);
    }

    float modFactor = std::pow(2.0f, combinedMod * 2.0f); // 2 octaves range
    float modCutoff = baseCutoff * modFactor;
    modCutoff = juce::jlimit(20.0f, 20000.0f, modCutoff);

    filter.setCutoffFrequency(modCutoff);
    filter.setResonance(baseResonance);

    float input = bufferData[i];

    // Extra velocity sensitivity control (0=flat, 1=full)
    float velGain = (1.0f - ampVelocityAmount) + (noteVelocity * ampVelocityAmount);
    input *= velGain;

    // Filter drive (simple saturation pre-filter)
    if (filterDrive > 0.001f) {
      float driveGain = 1.0f + (filterDrive * 12.0f);
      input = std::tanh(input * driveGain);
    }

    // Apply Mod Env to Vol/Pan/Pitch if selected
    if (modTarget == 1) { // Volume
      // Amount determines how much Env affects Vol
      // If amount 0, no effect. If amount 1, full effect (multiply)
      // Actually, "Amount" often means "Depth".
      // Let's simply multiply gain.
      // input *= (1.0f + (modEnvVal * modAmount)); // This adds gain.
      // Or subtractive? Standard is Positive Mod adds, Negative (in bipolar)
      // subtracts. Here modAmount is 0-1 unipolar? Let's assume bipolar range
      // mapping or just unidirectional. User knob is 0-1. So adds volume.
      input *= (1.0f - (modAmount * 0.5f) + (modEnvVal * modAmount));
    }

    // Pitch (Target 3) would need resampling rate update (expensive inside loop
    // per sample without interpolator update) Pan (Target 2) handled at end.

    if (std::isnan(input))
      input = 0.0f;
    float filtered = filter.processSample(0, input);

    if (isNotch) {
      filtered = input - filtered;
    }

    // Safety Check for NaN/Infinity
    if (std::isnan(filtered) || std::isinf(filtered)) {
      filtered = 0.0f;
      filter.reset();
    }

    bufferData[i] = filtered;
  }

  if (!adsr.isActive()) {
    clearCurrentNote();
    return;
  }

  // FORCE STOP if One-Shot and Sample has finished playing
  // SamplerVoice::isVoiceActive() returns false when sample finishes (if not
  // looping)
  if (isCurrentSoundOneShot && !juce::SamplerVoice::isVoiceActive()) {
    clearCurrentNote();
    return;
  }

  // 4. Panning and Output Mix
  if (isCurrentSoundBass) {
    // Bass Logic: Lows (<120Hz) -> Mono, Highs -> Panned

    // We can't easily do per-sample crossover efficiently here without
    // block processing or another temp buffer, but LinkwitzRiley is per-sample
    // capable. However, L-R is usually 2 channels (stereo). Here we have mono
    // voice signal `tempBuffer`. We want to split it: Lows, Highs.

    // Process the whole block through a filter to get Lows?
    // But filters are stateful.

    // Let's use two filters? Or process tempBuffer in place for Lows,
    // and subtract from original to get Highs?
    // (Linkwitz-Riley sums flat).

    // Copy for Highs (preallocated)
    if (bassHighBuffer.getNumSamples() < numSamples)
      bassHighBuffer.setSize(1, numSamples, false, false, true);
    bassHighBuffer.makeCopyOf(tempBuffer, true);

    // Process tempBuffer (Lows)
    juce::dsp::AudioBlock<float> block(tempBuffer);
    // Since we only have 1 channel in tempBuffer
    juce::dsp::ProcessContextReplacing<float> context(block);
    crossoverFilter.process(context);

    // Now tempBuffer contains Lows.
    // Highs = Original (bassHighBuffer) - Lows (tempBuffer)
    bassHighBuffer.addFrom(0, 0, tempBuffer.getReadPointer(0), numSamples,
                           -1.0f); // Subtract

    // Mix to Output
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      // Pan Highs
      float panGain = 1.0f;
      if (outputBuffer.getNumChannels() == 2) {
        float panRad = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        if (ch == 0)
          panGain = std::cos(panRad);
        if (ch == 1)
          panGain = std::sin(panRad);
      }

      // Mono Lows (Center panned -> equal gain 0.707 or 1.0 depending on law,
      // let's use 1.0 for kick/sub power)
      float bassGain = 1.0f;
      // Actually standard constant power center is 0.707, but for sub we often
      // want full power. Let's stick to standard pan center gain approx 0.707
      // if we want consistency with other sounds. But typically "Mono" means
      // equal in both.
      bassGain = (outputBuffer.getNumChannels() == 2) ? 0.707f : 1.0f;

      // Add Lows (Center)
      outputBuffer.addFrom(ch, startSample, tempBuffer, 0, 0, numSamples,
                           bassGain);

      // Add Highs (Panned)
      outputBuffer.addFrom(ch, startSample, bassHighBuffer, 0, 0, numSamples,
                           panGain);
    }
  } else {
    // Standard processing
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      float gain = 1.0f;
      if (outputBuffer.getNumChannels() == 2) {
        float panRad = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        if (ch == 0)
          gain = std::cos(panRad);
        if (ch == 1)
          gain = std::sin(panRad);
      }

      outputBuffer.addFrom(ch, startSample, tempBuffer, 0, 0, numSamples, gain);
    }
  }
}

//==============================================================================
// SynthEngine
//==============================================================================

SynthEngine::SynthEngine() {
  // Add voices
  for (int i = 0; i < 8; ++i) {
    addVoice(new HowlingVoice());
  }
}

void SynthEngine::initialize() {
  prepareToRemoveAllSounds();
  clearSounds();
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock) {
  setCurrentPlaybackSampleRate(sampleRate);
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->prepare(sampleRate, samplesPerBlock);
    }
  }
}

// ... (existing updateSampleParams)
void SynthEngine::updateSampleParams(float tune, float sampleStart,
                                     float sampleEnd, bool loop) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateSampleParams(tune, sampleStart, sampleEnd, loop);
    }
  }
}

void SynthEngine::updateParams(float attack, float decay, float sustain,
                               float release, float cutoff, float resonance,
                               int filterType, float lfoRate, float lfoDepth) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateADSR(attack, decay, sustain, release);
      voice->updateFilter(cutoff, resonance, filterType);
      voice->updateLFO(lfoRate, lfoDepth, 0.0f);
    }
  }
}

void SynthEngine::updateVoiceControls(float ampPan, float ampVelocity,
                                      float filterDrive, float lfoPhase,
                                      float modSmooth) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->setPan(ampPan);
      voice->setAmpVelocity(ampVelocity);
      voice->setFilterDrive(filterDrive);
      voice->setLFOPhase(lfoPhase);
      voice->setModSmooth(modSmooth);
    }
  }
}

void SynthEngine::updateModParams(float attack, float decay, float sustain,
                                  float release, float amount, int target) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateModADSR(attack, decay, sustain, release, amount, target);
    }
  }
}

void SynthEngine::setPackMode(int size, float spread) {
  packSize = size;
  packSpread = spread;
}

void SynthEngine::noteOn(int midiChannel, int midiNoteNumber, float velocity) {
  // Standard note on
  // If Unison is active (packSize > 1), trigger multiple voices

  if (packSize > 1 && packSpread > 0.0f) {
    // Basic Unison: Trigger extra voices with detune
    // Since SamplerVoice doesn't support fine detune easily without pitch ratio
    // hack, we might just trigger neighbor notes or same note multiple times if
    // we have voice stealing? Standard JUCE Synthesiser handles note
    // allocation. For now, just call base.
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
  } else {
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
  }
}
