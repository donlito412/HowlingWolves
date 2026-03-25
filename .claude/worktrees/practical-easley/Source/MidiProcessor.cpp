#include "MidiProcessor.h"

//==============================================================================
// Arpeggiator
//==============================================================================

Arpeggiator::Arpeggiator() {
  // Init sequence to -1 (empty) or 0 (root note)
  // Let's init to a simple pattern like Root every quarter?
  // Or just empty. User wants to "switch to step sequencer".
  // Let's init to -1.
  sequence.fill(-1);
}

void Arpeggiator::prepare(double sampleRate) { currentSampleRate = sampleRate; }

void Arpeggiator::reset() {
  sortedNotes.clear();
  currentStep = 0;
  noteTime = 0.0;
  // Panic: Clear active notes to stop stuck ones on reset
  activeNotes.clear();
}

void Arpeggiator::setParameters(float rate, int mode, int octaves, float gate,
                                bool on, float density, float complexity,
                                float spread) {
  rateDiv = rate;
  arpMode = mode;
  numOctaves = octaves;
  gateLength = gate;
  enabled = on;
  arpDensity = density;       // 0.0-1.0
  arpComplexity = complexity; // 0.0-1.0
  arpSpread = spread;
}

void Arpeggiator::setRhythmStep(int step, int semitoneOffset) {
  if (step >= 0 && step < 16)
    sequence[(size_t)step] = semitoneOffset;
}

int Arpeggiator::getRhythmStep(int step) const {
  if (step >= 0 && step < 16)
    return sequence[(size_t)step];
  return -1;
}

void Arpeggiator::handleNoteOn(int note, int velocity) {
  bool wasEmpty = sortedNotes.empty();

  // Add unique and sort
  bool found = false;
  for (int n : sortedNotes)
    if (n == note)
      found = true;

  if (!found) {
    sortedNotes.push_back(note);
    std::sort(sortedNotes.begin(), sortedNotes.end());
  }

  // Instant Trigger: If this is the first note, force trigger ASAP via flag
  if (wasEmpty) {
    currentStep = 0;
    pendingTrigger = true;
  }
}

void Arpeggiator::handleNoteOff(int note) {
  // Remove
  auto it = std::remove(sortedNotes.begin(), sortedNotes.end(), note);
  sortedNotes.erase(it, sortedNotes.end());
}

// Helper to check if grid is empty
bool Arpeggiator::isGridEmpty() const {
  for (int s : sequence) {
    if (s != -1)
      return false;
  }
  return true;
}

int Arpeggiator::getNextNote() {
  if (sortedNotes.empty())
    return -1;

  int numNotes = (int)sortedNotes.size();
  int root = sortedNotes[0];

  // --- MODE 1: STEP SEQUENCER (Arp Button OFF) ---
  if (!enabled) {
    int stepIdx = currentStep % 16;
    int seqVal = sequence[(size_t)stepIdx];

    if (seqVal == -1)
      return -1; // Rest

    // Scale Mapping (Minor-ish)
    constexpr int offsets[] = {0, 2, 3, 5, 7, 9, 10, 12};
    int offset = 0;
    if (seqVal >= 0 && seqVal < 8)
      offset = offsets[seqVal];

    return root + offset;
  }

  // --- MODE 2: ARPEGGIATOR (Arp Button ON) ---
  int noteIdx = 0;

  switch (arpMode) {
  case 1: // UP
    noteIdx = currentNoteIndex % numNotes;
    break;

  case 2: // DOWN
    noteIdx = (numNotes - 1) - (currentNoteIndex % numNotes);
    break;

  case 3: // UP/DOWN (Inclusive)
  {
    if (numNotes < 2) {
      noteIdx = 0;
    } else {
      int span = (numNotes * 2) - 2;
      int pos = currentNoteIndex % span;
      if (pos < numNotes)
        noteIdx = pos;
      else
        noteIdx = span - pos;
    }
  } break;

  case 4: // RANDOM
    noteIdx = juce::Random::getSystemRandom().nextInt(numNotes);
    break;

  default: // OFF or Manual
    return -1;
  }

  // Handle Octave Wrapping
  if (numOctaves > 1 && (arpMode == 1 || arpMode == 2 || arpMode == 3)) {
    int totalSteps = numNotes * numOctaves;

    // Re-calculate effective index based on Virtual Octave Range
    if (arpMode == 3) { // Up/Down
      if (totalSteps < 2) {
        noteIdx = 0;
      } else {
        int span = (totalSteps * 2) - 2;
        int pos = currentNoteIndex % span;
        int effective = (pos < totalSteps) ? pos : (span - pos);

        int actualNote = effective % numNotes;
        int oct = effective / numNotes;
        return sortedNotes[actualNote] + (oct * 12);
      }
    } else { // UP or DOWN
      int effective = currentNoteIndex % totalSteps;
      if (arpMode == 2) { // Down
        effective = (totalSteps - 1) - effective;
      }
      int actualNote = effective % numNotes;
      int oct = effective / numNotes;
      return sortedNotes[actualNote] + (oct * 12);
    }
  }

  return sortedNotes[noteIdx];
}

double Arpeggiator::getSamplesPerStep(juce::AudioPlayHead *playHead,
                                      float fallbackBPM) {
  double bpm = fallbackBPM; // Default to fallback
  if (playHead) {
    if (auto pos = playHead->getPosition()) {
      if (pos->getBpm().hasValue())
        bpm = *pos->getBpm();
    }
  }

  // Safety Clamp
  if (bpm < 20.0)
    bpm = fallbackBPM; // Keep using fallback if weird value

  // RateDiv: Passed as Index cast to float (0.0, 1.0, 2.0, 3.0)
  // or Normalized? Current usage in PluginProcessor is `(float)rateIdx`.
  // So we expect 0.0, 1.0, 2.0, 3.0.

  double quarterNoteSamples = (60.0 / bpm) * currentSampleRate;

  // Safe integer cast
  int r = (int)(rateDiv + 0.1f); // Round nearest

  double speed = quarterNoteSamples;
  switch (r) {
  case 0:
    speed = quarterNoteSamples;
    break; // 1/4
  case 1:
    speed = quarterNoteSamples / 2.0;
    break; // 1/8
  case 2:
    speed = quarterNoteSamples / 4.0;
    break; // 1/16
  case 3:
    speed = quarterNoteSamples / 8.0;
    break; // 1/32
  default:
    speed = quarterNoteSamples;
    break; // Fallback
  }
  return speed;
}

void Arpeggiator::process(juce::MidiBuffer &midiMessages, int numSamples,
                          juce::AudioPlayHead *playHead, float fallbackBPM) {

  // --- 1. Update State from Input ---
  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      handleNoteOn(msg.getNoteNumber(), msg.getVelocity());
    } else if (msg.isNoteOff()) {
      handleNoteOff(msg.getNoteNumber());
    } else if (msg.isAllNotesOff()) {
      reset();
    }
  }

  // --- 2. Check if we should generate anything ---
  // New Logic: If Disabled AND Grid is Empty -> Passthrough (Do nothing)
  // If Disabled BUT Grid has steps -> Run Sequencer Logic
  // If Enabled -> Run Arp Logic

  // New Logic: If Disabled OR Mode is 0 (- Select -) AND Grid Empty -> Bypass
  bool bypassed = !enabled || (arpMode == 0);

  if (bypassed && isGridEmpty()) {
    // Standard Passthrough of events handled by flushing active notes logic
    // below? No, if we return here, we might leave active notes hanging if we
    // don't process offs. "activeNotes" tracks generated notes. Input notes
    // passed through? Wait, typical Arp replaces input. If Bypass: we should
    // just let input through? Current architecture checks
    // "MidiProcessor::process":
    // 1. Chords -> 2. Arp.
    // If Arp Bypassed, Chords output should pass.

    // BUT we must flush any *internally generated* notes from previous arp
    // state.
    for (auto it = activeNotes.begin(); it != activeNotes.end();) {
      if (it->samplesRemaining < numSamples) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                              it->samplesRemaining);
        it = activeNotes.erase(it);
      } else {
        it->samplesRemaining -= numSamples;
        ++it;
      }
    }
    // And logic to pass original notes?
    // The loop at start "Update State" consumes messages? No, just reads.
    // The "3. Generate Sequence" block copies non-notes.
    // If we are in passthrough, we should normally just return?
    // MidiProcessor passes `midiMessages`. We just modified `arp` state.
    // If we do nothing, `midiMessages` stays as is (Chord Output).
    // PERFECT.
    return;
  }

  // --- 3. Generate Sequence (Arp or Seq) ---
  juce::MidiBuffer processedMidi;

  // Pass through non-note events (CC, etc)
  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    // Block original notes (we are replacing them with Arp/Seq notes)
    if (!msg.isNoteOn() && !msg.isNoteOff())
      processedMidi.addEvent(msg, metadata.samplePosition);
  }

  // Handle active note-offs
  for (auto it = activeNotes.begin(); it != activeNotes.end();) {
    if (it->samplesRemaining < numSamples) {
      processedMidi.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                             it->samplesRemaining);
      it = activeNotes.erase(it);
    } else {
      it->samplesRemaining -= numSamples;
      ++it;
    }
  }

  // Arp Logic
  if (sortedNotes.empty()) {
    midiMessages.swapWith(processedMidi);
    return;
  }

  // Arp Timing Logic
  double samplesPerStep = getSamplesPerStep(playHead, fallbackBPM);
  if (samplesPerStep < 100.0)
    samplesPerStep = 100.0;

  // Immediate Trigger Logic
  if (pendingTrigger) {
    noteTime = samplesPerStep + 1.0;
    pendingTrigger = false;
    currentNoteIndex = 0;
  }

  // Robust Accumulator Loop (The Audio Programmer Method)
  // Simply add samples and check if we passed the threshold.
  noteTime += numSamples;

  while (noteTime >= samplesPerStep) {
    noteTime -= samplesPerStep;

    // Trigger Note
    int note = getNextNote();
    if (note > 0) {
      // Add Note On at offset 0 (simplified for block alignment)
      processedMidi.addEvent(
          juce::MidiMessage::noteOn(1, note, (juce::uint8)100), 0);

      // Calculate Duration
      int gateSamples = static_cast<int>(samplesPerStep * gateLength);

      ActiveNote an;
      an.noteNumber = note;
      an.samplesRemaining = gateSamples;
      activeNotes.push_back(an);

      // Advance Pattern
      currentNoteIndex++;
      currentStep++;
    }
  }

  midiMessages.swapWith(processedMidi);
}

//==============================================================================
// ChordEngine
//==============================================================================

ChordEngine::ChordEngine() {}

void ChordEngine::setParameters(int mode, int keys, bool hold) {
  chordMode = mode;
  bool wasHolding = holdEnabled;
  holdEnabled = hold;

  // If we turned OFF hold, kill all stuck notes
  if (wasHolding && !holdEnabled) {
    shouldFlushNotes = true;
  }
}

void ChordEngine::process(juce::MidiBuffer &midiMessages) {
  juce::MidiBuffer processedBuf;

  // 1. Flush Stuck Notes if Hold was just disabled
  if (shouldFlushNotes) {
    for (int note : heldNotes) {
      processedBuf.addEvent(juce::MidiMessage::noteOff(1, note), 0);
    }
    heldNotes.clear();
    shouldFlushNotes = false;
  }

  // "CHORDS" Button (was Hold) is now Master Switch
  // If button is OFF (holdEnabled == false), Bypass Chords completely.
  // If button is OFF (holdEnabled == false) OR Mode is 0 (Select), Bypass.
  if (!holdEnabled || chordMode == 0) {
    return; // Passthrough original notes
  }

  int effectiveMode = chordMode; // Direct map: 1=Major, 2=Minor...

  // Clear original output buffer (we rebuild it)
  // Actually, standard practice for these processors: read from 'midiMessages',
  // write to 'processedBuf', then swap. BUT if we 'return', 'midiMessages'
  // keeps original content (Passthrough). So for Mode=0 returns, it's
  // Passthrough.

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();

    if (msg.isNoteOn() || msg.isNoteOff()) {
      int root = msg.getNoteNumber();
      int vel = msg.getVelocity();
      bool isOn = msg.isNoteOn();

      // HOLD LOGIC REMOVED:
      // We now treat "holdEnabled" strictly as "Chords Enabled" (Momentary).
      // Note Offs are allowed to pass through and will be transposed to kill
      // chord voices.

      // if (!isOn && holdEnabled) {
      //   continue;
      // }

      auto addEvent = [&](int note) {
        if (isOn) {
          processedBuf.addEvent(
              juce::MidiMessage::noteOn(1, note, (juce::uint8)vel),
              metadata.samplePosition);
          if (holdEnabled)
            heldNotes.insert(note); // Track for flushing if disabled mid-play
        } else {
          processedBuf.addEvent(juce::MidiMessage::noteOff(1, note),
                                metadata.samplePosition);
          if (heldNotes.find(note) != heldNotes.end()) {
            heldNotes.erase(note);
          }
        }
      };

      addEvent(root);

      switch (effectiveMode) {
      case 1: // Major
        addEvent(root + 4);
        addEvent(root + 7);
        break;
      case 2: // Minor
        addEvent(root + 3);
        addEvent(root + 7);
        break;
      case 3: // 7th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 10);
        break;
      case 4: // 9th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 14);
        break;
      default:
        break;
      }

    } else {
      processedBuf.addEvent(msg, metadata.samplePosition);
    }
  }

  midiMessages.swapWith(processedBuf);
}

//==============================================================================
// MidiProcessor
//==============================================================================

MidiProcessor::MidiProcessor() {}

void MidiProcessor::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  arp.prepare(sampleRate);
}

void MidiProcessor::reset() { arp.reset(); }

void MidiProcessor::process(juce::MidiBuffer &midiMessages, int numSamples,
                            juce::AudioPlayHead *playHead, float fallbackBPM) {
  // 1. Chords First
  chordEngine.process(midiMessages);

  // 2. Arp Second
  arp.process(midiMessages, numSamples, playHead, fallbackBPM);
}
