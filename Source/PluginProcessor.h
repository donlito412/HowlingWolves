#pragma once

#include "EffectsProcessor.h"
#include "FilterProcessor.h"
#include "HuntEngine.h"
#include "LFOProcessor.h"
#include "LicenseManager.h"
#include "MidiCapturer.h"
#include "MidiProcessor.h"
#include "PresetManager.h"
#include "SampleManager.h"
#include "SynthEngine.h"
#include <JuceHeader.h>
#include <atomic>

class HowlingWolvesAudioProcessor : public juce::AudioProcessor,
                                    private juce::ChangeListener {
public:
  //==============================================================================
  HowlingWolvesAudioProcessor();
  ~HowlingWolvesAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }
  SynthEngine &getSynth() { return synthEngine; }
  SampleManager &getSampleManager() { return sampleManager; }

  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }
  PresetManager &getPresetManager() { return presetManager; }
  HuntEngine &getHuntEngine() { return huntEngine; }

  MidiCapturer &getMidiCapturer() { return midiCapturer; }
  MidiProcessor &getMidiProcessor() { return midiProcessor; }

  // Transport Control (Internal; reserved for future host-less transport UI)
  void setTransportPlaying(bool shouldPlay) {
    transportPlaying.store(shouldPlay);
  }
  bool isTransportPlaying() const { return transportPlaying.load(); }

  // Metering Accessors
  float getEqLow() const { return effectsProcessor.eqLow; }
  float getEqMid() const { return effectsProcessor.eqMid; }
  float getEqHigh() const { return effectsProcessor.eqHigh; }

  // License and Security
  LicenseManager &getLicenseManager() { return licenseManager; }
  bool checkLicenseValid() const { return isLicenseValid.load(); }
  void setLicenseValid(bool valid) { isLicenseValid.store(valid); }

  // Shared Resources for UI
  juce::AudioFormatManager formatManager;
  juce::AudioThumbnailCache thumbCache{512};

  // Lock-free visualizer FIFO.
  // The audio thread (processBlock) writes the L-channel into this. The UI
  // (VisualizerComponent) reads from it via setSourceFifo(). Because the FIFO
  // lives here in the Processor — which outlives any editor — there are no
  // dangling-reference crashes on teardown.
  static constexpr int kVisFifoSize = 4096;
  juce::AbstractFifo visualizerFifo{kVisFifoSize};
  float visualizerFifoData[kVisFifoSize]{};

private:
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;
  void applyInstrumentEnvelopeDefaults();

  //==============================================================================
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  juce::AudioProcessorValueTreeState apvts;

  SampleManager sampleManager;
  SynthEngine synthEngine;
  juce::MidiKeyboardState keyboardState;
  PresetManager presetManager;

  // Filter and LFO
  FilterProcessor filterProcessor;
  LFOProcessor lfoProcessor;
  EffectsProcessor effectsProcessor;
  MidiProcessor midiProcessor;
  HuntEngine huntEngine;
  MidiCapturer midiCapturer;

  std::atomic<bool> transportPlaying{false};
  std::atomic<float> internalBPM{120.0f};

  LicenseManager licenseManager;
  std::atomic<bool> isLicenseValid{false};

  //==============================================================================

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HowlingWolvesAudioProcessor)
};
