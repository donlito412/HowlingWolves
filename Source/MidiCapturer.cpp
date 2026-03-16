#include "MidiCapturer.h"

MidiCapturer::MidiCapturer() {}
MidiCapturer::~MidiCapturer() {}

void MidiCapturer::prepare(double sr) {
  sampleRate = sr;
  currentSampleTime = 0.0;
}

void MidiCapturer::clear() {
  sequence.clear();
  isFirstMessage = true;
  startTime = 0.0;
  currentSampleTime = 0.0;
}

void MidiCapturer::setBpm(double bpm) { currentBpm = bpm; }

void MidiCapturer::setRecording(bool shouldRecord) {
  if (shouldRecord) {
    clear(); // Clear on start resets currentSampleTime to 0.0
    currentSampleTime = 0.0; // explicit safety
  }
  recording = shouldRecord;
}

void MidiCapturer::addMessage(const juce::MidiMessage &message) {
  // Direct add (unused mostly now)
  sequence.addEvent(message);
}

void MidiCapturer::processMidi(const juce::MidiBuffer &buffer, int numSamples) {
  // If we assume this is only called when recording is desired (controlled by
  // Processor) Or we should have setRecording(bool)? For now, Processor
  // controls flow.

  if (!recording) {
    currentSampleTime += numSamples; // Keep time running or pause?
    // Usually, we want to capture relative to RECORD START.
    // So if we stop and start, we reset time?
    // clear() resets time. So setRecording(true) -> clear() -> time=0.
    return;
  }

  for (const auto metadata : buffer) {
    auto msg = metadata.getMessage();
    // Time in seconds
    double eventTimeSeconds =
        (currentSampleTime + metadata.samplePosition) / sampleRate;
    msg.setTimeStamp(eventTimeSeconds);
    sequence.addEvent(msg);
  }
  currentSampleTime += numSamples;
}

void MidiCapturer::startRecording() { setRecording(true); }

void MidiCapturer::stopRecording() { setRecording(false); }

bool MidiCapturer::isRecording() const { return recording; }

bool MidiCapturer::hasRecording() const { return sequence.getNumEvents() > 0; }

juce::File MidiCapturer::getLastRecording() const { return lastFile; }

juce::File MidiCapturer::saveToTempFile() {
  auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                      .getChildFile("drag_export.mid");

  if (tempFile.exists())
    tempFile.deleteFile();

  // Create MidiFile
  juce::MidiFile midiFile;
  midiFile.setTicksPerQuarterNote(960);

  // Convert seconds to ticks (Approximate)
  // Ticks = Seconds * (BPM / 60) * TPQN
  // We need to re-timestamp events from seconds (JUCE time) to ticks
  juce::MidiMessageSequence trackSeq;

  // Add Tempo Meta Event
  // 60,000,000 / BPM = Microseconds per quarter
  int microsPerQuarter = (int)(60000000.0 / currentBpm);
  juce::MidiMessage tempoMsg =
      juce::MidiMessage::tempoMetaEvent(microsPerQuarter);
  tempoMsg.setTimeStamp(0);
  trackSeq.addEvent(tempoMsg);

  for (int i = 0; i < sequence.getNumEvents(); ++i) {
    auto *ev = sequence.getEventPointer(i);
    auto msg = ev->message;

    double seconds = msg.getTimeStamp(); // Relative seconds
    double quarters = seconds * (currentBpm / 60.0);
    double ticks = quarters * 960.0;

    msg.setTimeStamp(ticks);
    trackSeq.addEvent(msg);
  }

  trackSeq.updateMatchedPairs();
  midiFile.addTrack(trackSeq);

  juce::FileOutputStream stream(tempFile);
  if (stream.openedOk()) {
    midiFile.writeTo(stream);
    stream.flush(); // CRITICAL: Force OS flush before drag initiates
  }

  lastFile = tempFile;
  return tempFile;
}
