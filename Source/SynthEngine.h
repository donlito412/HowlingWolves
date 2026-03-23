#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A sound that holds the sample data.
    Wrapper around juce::SamplerSound to allow for custom expansion if needed,
    and to maintain the "HowlingSound" type name used in the codebase.
*/
class HowlingSound : public juce::SamplerSound {
public:
  HowlingSound(const juce::String &name, juce::AudioFormatReader &source,
               const juce::BigInteger &midiNotes, int midiNoteForNormalPitch,
               double attackTimeSecs, double releaseTimeSecs,
               double maxSampleLengthSeconds, bool isBassSound = false,
               bool isOneShotSound = false)
      : juce::SamplerSound(name, source, midiNotes, midiNoteForNormalPitch,
                           attackTimeSecs, releaseTimeSecs,
                           maxSampleLengthSeconds),
        isBass(isBassSound), isOneShot(isOneShotSound) {}

  bool isBassSample() const { return isBass; }
  bool isOneShotSample() const { return isOneShot; }

private:
  bool isBass;
  bool isOneShot;
};

//==============================================================================
/**
    A voice that plays back the HowlingSound (Sample).
    Inherits from juce::SamplerVoice to handle pitch shifting and resampling.
    Adds custom Filter and LFO processing.
*/
class HowlingVoice : public juce::SamplerVoice {
public:
  HowlingVoice();

  bool canPlaySound(juce::SynthesiserSound *sound) override {
    return dynamic_cast<juce::SamplerSound *>(sound) != nullptr;
  }

  // DSP Parameters
  void updateFilter(float cutoff, float resonance, int filterType);
  void updateLFO(float rate, float depth, float phase01);
  void prepare(double sampleRate, int samplesPerBlock);

  // Overrides for ADSR control
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;

  // New Sample Params
  void updateSampleParams(float tune, float sampleStart, float sampleEnd,
                          bool loop);
  void setPan(float newPan);
  void setAmpVelocity(float amount01);
  void setFilterDrive(float drive01);
  void setModSmooth(float smooth01);
  void setLFOPhase(float phase01);

  // Override render to add post-processing (Filter)
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  // Custom ADSR access
  void updateADSR(float attack, float decay, float sustain, float release);

private:
  juce::dsp::StateVariableTPTFilter<float> filter;
  // Manual LFO so we can support phase offset deterministically
  double lfoSampleRate = 44100.0;
  float lfoPhaseAcc = 0.0f;        // radians
  float lfoPhaseOffset = 0.0f;     // radians
  float lfoIncrement = 0.0f;       // radians per sample
  float lfoDepth = 0.0f;
  float pan = 0.0f; // -1.0 (Left) to 1.0 (Right)
  float ampVelocityAmount = 1.0f; // 0..1
  float noteVelocity = 1.0f;      // 0..1 (captured at noteOn)
  float filterDrive = 0.0f;       // 0..1
  float modSmooth = 0.1f;         // 0..1
  float smoothedModEnv = 0.0f;

  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;
  float lfoRate = 0.0f;

  // Sample Parameters
  float tuneSemitones = 0.0f;
  float sampleStartPercent = 0.0f;
  float sampleEndPercent = 1.0f;
  bool isLooping = true;

  // Modulation Envelope
  juce::ADSR modAdsr;
  juce::ADSR::Parameters modAdsrParams;
  float modAmount = 0.5f;
  int modTarget = 0; // 0=None/Filter, 1=Vol, 2=Pan, 3=Pitch

public: // Accessor needed for setup
  void updateModADSR(float attack, float decay, float sustain, float release,
                     float amount, int target);

  // Bass processing
  bool isCurrentSoundBass = false;
  juce::dsp::LinkwitzRileyFilter<float> crossoverFilter; // For splitting Bass

  // One-Shot processing
  bool isCurrentSoundOneShot = false;

  // Base parameters for modulation
  float baseCutoff = 20000.0f;
  float baseResonance = 0.1f;
  bool isNotch = false;

  juce::AudioBuffer<float> tempBuffer;
  juce::AudioBuffer<float> bassHighBuffer;

  JUCE_LEAK_DETECTOR(HowlingVoice)
};

//==============================================================================
/**
    The main synthesizer engine.
    Manages voices and sounds.
*/
class SynthEngine : public juce::Synthesiser {
public:
  SynthEngine();

  void initialize();
  void prepare(double sampleRate, int samplesPerBlock);

  /** Stop every voice before clearSounds() so no voice references freed sounds. */
  void prepareToRemoveAllSounds() { allNotesOff(0, false); }

  void updateParams(float attack, float decay, float sustain, float release,
                    float cutoff, float resonance, int filterType,
                    float lfoRate, float lfoDepth);

  void updateVoiceControls(float ampPan, float ampVelocity, float filterDrive,
                           float lfoPhase, float modSmooth);

  void updateModParams(float attack, float decay, float sustain, float release,
                       float amount, int target); // New Mod Env Params

  void updateSampleParams(float tune, float sampleStart, float sampleEnd,
                          bool loop);

  // Unison (Pack Mode) parameters
  void setPackMode(int size, float spread); // size 1-8, spread 0.0-1.0

  void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;

private:
  int packSize = 1;
  float packSpread = 0.0f; // Detune and Pan spread amount
};
