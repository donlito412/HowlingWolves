#pragma once

#include <JuceHeader.h>
#include <atomic>

class MidiCapturer {
public:
  MidiCapturer();
  ~MidiCapturer();

  void prepare(double sampleRate);
  void clear();
  void addMessage(const juce::MidiMessage &message);
  void processMidi(const juce::MidiBuffer &buffer, int numSamples);

  void startRecording();
  void stopRecording();
  bool isRecording() const;

  juce::File getLastRecording() const;
  bool hasRecording() const;

  // Creates the .mid file and returns it
  juce::File saveToTempFile();

  void setRecording(bool shouldRecord);
  void setBpm(double bpm);

private:
private:
  juce::MidiMessageSequence sequence;
  double currentBpm = 120.0;
  double startTime = 0.0;
  bool isFirstMessage = true;

  double currentSampleTime = 0.0;
  double sampleRate = 44100.0;
  bool recording = false;

  juce::CriticalSection lock; // Protect sequence access
  juce::File lastFile;
};
