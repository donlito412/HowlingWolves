#pragma once

#include "SynthEngine.h"
#include <JuceHeader.h>

//==============================================================================
/**
    Manages loading of samples and mapping them to the synth.
*/
enum class LoadedInstrumentKind {
  None,
  Pluck,
  Bass,
  Sustained, // Keys, Leads, Pads, Strings, Horns, Woodwinds
  Hit,       // FX, drums, one-shots
  Sequence,
  Texture,
  Generic
};

class SampleManager : public juce::ChangeBroadcaster {
public:
  SampleManager(SynthEngine &synth);
  ~SampleManager();

  void loadSamples(); // Initial load (optional)
  /** If applyHostEnvelopeDefaults is false (e.g. XML preset), host ADSR is left as-is. */
  void loadSound(const juce::File &file, bool applyHostEnvelopeDefaults = true);
  void loadDrumKit(const juce::File &kitDirectory);

  juce::String getCurrentSamplePath() const;
  LoadedInstrumentKind getLastLoadedKind() const { return lastLoadedKind; }
  /** Cleared after read; used by processor after sample change notification. */
  bool consumeApplyHostEnvelopeDefaults();

private:
  SynthEngine &synthEngine;
  juce::AudioFormatManager formatManager;
  juce::String currentSamplePath;
  LoadedInstrumentKind lastLoadedKind{LoadedInstrumentKind::None};
  bool applyHostEnvelopeAfterLoad{true};
};
