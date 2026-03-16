#include "SampleManager.h"

SampleManager::SampleManager(SynthEngine &s) : synthEngine(s) {
  formatManager.registerBasicFormats();
}

SampleManager::~SampleManager() {}

// Helper to get standard location with priority search
void SampleManager::loadSamples() {
  // Initial load can be left empty or load a default welcome sound.
  // We rely on the user selecting a preset.
  synthEngine.clearSounds();
}

void SampleManager::loadSound(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  // Clear current sounds first so we don't play the old one if this load fails
  synthEngine.clearSounds();
  currentSamplePath = file.getFullPathName();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));

  if (reader != nullptr) {
    juce::BigInteger allNotes;
    int rootNote = 60; // Default C3

    // Check parent folder name
    juce::String folder = file.getParentDirectory().getFileName();
    bool isLead = folder.equalsIgnoreCase("Leads");
    bool isKey = folder.equalsIgnoreCase("Keys");
    bool isPad = folder.equalsIgnoreCase("Pads");
    bool isPluck = folder.equalsIgnoreCase("Plucks");
    bool isBass = folder.equalsIgnoreCase("Bass");
    bool isFX = folder.equalsIgnoreCase("FX");
    bool isTexture = false; // folder.containsIgnoreCase("Texture"); // Removed
                            // per user request
    bool isSequence = folder.containsIgnoreCase("Sequence");
    bool isDrum = folder.containsIgnoreCase("Drum");

    bool isOneShot = false;

    if (isLead || isKey || isPad) {
      // Map Leads/Keys/Pads: MIDI 24..96 (C1..C7)
      // setRange(startBit, numBits, value) -> start 24, num 73 (24+72=96)
      allNotes.setRange(24, 73, true);

      // Root Note: Default C3 (60)
      if (reader->metadataValues.containsKey("RootNote")) {
        rootNote = reader->metadataValues["RootNote"].getIntValue();
      } else {
        rootNote = 60;
      }
    } else if (isPluck) {
      // Map Plucks: MIDI 36..96 (C2..C7)
      // start 36, num 61 (36+60=96) -> 61 semitones
      allNotes.setRange(36, 61, true);

      // Root Note: Default C4 (72)
      if (reader->metadataValues.containsKey("RootNote")) {
        rootNote = reader->metadataValues["RootNote"].getIntValue();
      } else {
        rootNote = 72;
      }

      // Force Looping OFF: Remove loop points from metadata
      reader->metadataValues.remove("NumSampleLoops");
      reader->metadataValues.remove("Loop0Start");
      reader->metadataValues.remove("Loop0End");
    } else if (isBass) {
      // Map Bass: MIDI 12..48 (C0..C3)
      // start 12, num 37 (12+36=48)
      allNotes.setRange(12, 37, true);

      // Root Note: Default C2 (36)
      if (reader->metadataValues.containsKey("RootNote")) {
        rootNote = reader->metadataValues["RootNote"].getIntValue();
      } else {
        rootNote = 36;
      }
    } else if (isFX) {
      // Map FX: Single Note 72 (C4)
      allNotes.setBit(72);

      // Root Note: 72 (No transposition)
      rootNote = 72;

      // Force Looping OFF
      reader->metadataValues.remove("NumSampleLoops");
      reader->metadataValues.remove("Loop0Start");
      reader->metadataValues.remove("Loop0End");

      isOneShot = true;
    } else if (isTexture) {
      // Map Textures
      if (reader->metadataValues.containsKey("RootNote")) {
        // Tonal Texture: Range 24..96, Pitch ON
        allNotes.setRange(24, 73, true);
        rootNote = reader->metadataValues["RootNote"].getIntValue();
        // Keep Looping (Default)
      } else {
        // Non-Tonal/Noise: Single Note 60
        allNotes.setBit(60);
        rootNote = 60;
        // Keep Looping (Default)
      }
    } else if (isSequence) {
      // Map Sequences: Single Note 60, Loop ON
      allNotes.setBit(60);
      rootNote = 60;

      // Force Looping ON: If no loop points, loop entire file
      if (!reader->metadataValues.containsKey("NumSampleLoops") ||
          reader->metadataValues["NumSampleLoops"].getIntValue() == 0) {
        reader->metadataValues.set("NumSampleLoops", "1");
        reader->metadataValues.set("Loop0Start", "0");
        reader->metadataValues.set("Loop0End",
                                   juce::String(reader->lengthInSamples));
      }
    } else if (isDrum) {
      // Map Drums (Individual): Single Note 60, One-Shot, Loop OFF
      allNotes.setBit(60);
      rootNote = 60;

      reader->metadataValues.remove("NumSampleLoops");
      reader->metadataValues.remove("Loop0Start");
      reader->metadataValues.remove("Loop0End");

      isOneShot = true;
    } else {
      // Standard mapping (Full Range)
      allNotes.setRange(0, 128, true);

      if (reader->metadataValues.containsKey("RootNote")) {
        rootNote = reader->metadataValues["RootNote"].getIntValue();
      }
    }

    auto *sound =
        new HowlingSound(file.getFileNameWithoutExtension(), *reader, allNotes,
                         rootNote, 0.0, 100.0, 60.0, isBass, isOneShot);

    synthEngine.addSound(sound);
  } else {
    DBG("Failed to load sample: " + file.getFullPathName());
  }

  sendChangeMessage();
}

void SampleManager::loadDrumKit(const juce::File &kitDirectory) {
  if (!kitDirectory.isDirectory())
    return;

  // Clear current sounds first so we don't play the old one if this load fails
  synthEngine.clearSounds();

  auto allowedExtensions = formatManager.getWildcardForAllFormats();
  int midiNote = 36; // Start at C1 (Standard Drum Map)
  int count = 0;

  // Iterate files in the directory
  for (const auto &file : kitDirectory.findChildFiles(
           juce::File::findFiles, false, allowedExtensions)) {

    if (count >= 16)
      break; // Limit to 16 pads

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader != nullptr) {
      // Map to SINGLE note
      juce::BigInteger noteMap;
      noteMap.setBit(midiNote);

      // Force Looping OFF
      reader->metadataValues.remove("NumSampleLoops");
      reader->metadataValues.remove("Loop0Start");
      reader->metadataValues.remove("Loop0End");

      auto *sound =
          new HowlingSound(file.getFileNameWithoutExtension(), *reader, noteMap,
                           midiNote,       // Root note = played note
                           0.0, 0.1, 60.0, // Fast attack
                           false, true);   // isBass=false, isOneShot=true

      synthEngine.addSound(sound);
      midiNote++;
      count++;
    }
  }
}

juce::String SampleManager::getCurrentSamplePath() const {
  return currentSamplePath;
}
