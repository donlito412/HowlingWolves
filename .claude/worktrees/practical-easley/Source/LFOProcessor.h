#pragma once
#include <JuceHeader.h>

class LFOProcessor {
public:
  LFOProcessor();

  enum Waveform { Sine = 0, Square, Triangle };

  enum Target { FilterCutoff = 0, Volume, Pan, Pitch };

  void prepare(double sampleRate);
  void reset();
  float getNextSample();

  void setWaveform(Waveform wave);
  void setRate(float rateHz);
  void setDepth(float depth);
  void setTarget(Target target);

  Target getTarget() const { return currentTarget; }
  float getDepth() const { return currentDepth; }

private:
  Waveform currentWaveform = Sine;
  Target currentTarget = FilterCutoff;
  float currentRate = 1.0f;
  float currentDepth = 0.5f;

  double phase = 0.0;
  double phaseIncrement = 0.0;
  double currentSampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOProcessor)
};
