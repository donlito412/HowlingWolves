#pragma once

#include <JuceHeader.h>

//==============================================================================
// Arpeggiator Module
//==============================================================================
class Arpeggiator {
public:
  Arpeggiator();
  ~Arpeggiator() = default;

  void prepare(double sampleRate);
  void process(juce::MidiBuffer &midiMessages, int numSamples,
               juce::AudioPlayHead *playHead, float fallbackBPM = 120.0f);
  void reset();

  // Parameters
  void setParameters(float rate, int mode, int octaves, float gate, bool on,
                     float density, float complexity, float spread);
  void setRhythmStep(int step, int semitoneOffset);
  int getRhythmStep(int step) const;
  int getCurrentStep() const { return currentStep; }
  bool isGridEmpty() const;

private:
  double currentSampleRate = 44100.0;

  // State
  int currentNoteIndex = 0;     // Tracks position in the Arp Pattern (0 to N-1)
  int currentStep = 0;          // Tracks total steps triggers (for Random/Seq)
  double noteTime = 0.0;        // Sample Accumulator
  std::vector<int> sortedNotes; // Held notes sorted

  struct ActiveNote {
    int noteNumber;
    int samplesRemaining;
  };
  std::vector<ActiveNote> activeNotes;
  bool pendingTrigger = false;

  // Params
  bool enabled = false;
  float rateDiv = 1.0f; // 1 = quarter note? No, standard divisions.
  int arpMode = 0;      // 0=Up, 1=Down, 2=Up/Down, 3=Random
  int numOctaves = 1;
  float gateLength = 0.5f;

  float arpDensity = 1.0f;
  float arpComplexity = 0.0f;
  float arpSpread = 0.0f;

  // Helper
  void handleNoteOn(int note, int velocity);
  void handleNoteOff(int note);
  int getNextNote();
  double getSamplesPerStep(juce::AudioPlayHead *playHead, float fallbackBPM);

  // Step Sequencer Data: 16 steps.
  // Values: -1 = Rest, 0-12 = Semitone offset from root?
  // User said "grid lights up", "switch to step sequencer".
  // Grid is 16x8. So values should be 0-7 (representing Rows).
  // -1 for inactive.
  std::array<int, 16> sequence;

  // We initialize with a default pattern? Or empty?
  // Let's Init to -1 (empty)
  // Actually user might want a default diagonal or something to see it works.
  // No, safer to default to "Root Note" steps if we want sound immediately?
  // Let's init to all 0 (Row 0 active) for now, or just some pattern.
  // Constructor can init.
};

//==============================================================================
// Chord Engine Module
//==============================================================================
class ChordEngine {
public:
  ChordEngine();
  ~ChordEngine() = default;

  void process(juce::MidiBuffer &midiMessages);

  // Parameters
  void setParameters(int mode, int keys,
                     bool hold); // mode: 0=Off, 1=Maj, 2=Min...

private:
  int chordMode = 0;
  bool holdEnabled = false;
  bool shouldFlushNotes = false;
  std::set<int> heldNotes;
  // Temporary storage for generated notes to ensure NoteOffs match
  // For v1, we transform NoteOn(C) -> NoteOn(C, E, G).
  // And NoteOff(C) -> NoteOff(C, E, G).
};

//==============================================================================
// MidiProcessor (Main Handler)
//==============================================================================
class MidiProcessor {
public:
  MidiProcessor();
  ~MidiProcessor() = default;

  void prepare(double sampleRate);
  void process(juce::MidiBuffer &midiMessages, int numSamples,
               juce::AudioPlayHead *playHead, float fallbackBPM = 120.0f);
  void reset();

  // Accessors for Modules
  Arpeggiator &getArp() { return arp; }
  ChordEngine &getChordEngine() { return chordEngine; }

  int getCurrentArpStep() const { return arp.getCurrentStep(); }

private:
  Arpeggiator arp;
  ChordEngine chordEngine;

  double currentSampleRate = 44100.0;
};
