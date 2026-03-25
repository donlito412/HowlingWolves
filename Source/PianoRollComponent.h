#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
// Note Grid Component (The timeline view)
//==============================================================================
class NoteGridComponent : public juce::Component {
public:
  NoteGridComponent(HowlingWolvesAudioProcessor &p,
                    juce::MidiKeyboardState &state);
  ~NoteGridComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;

  void setZoom(float zoomX, float zoomY);
  void setVisibleRange(int startNote, int endNote);

private:
  HowlingWolvesAudioProcessor &audioProcessor;
  juce::MidiKeyboardState &keyboardState;

  float zoomX = 1.0f;
  float zoomY = 1.0f;

  // Grid config
  int numSteps = 16;
  int notesPerOctave = 12;
};

//==============================================================================
// Professional Piano Roll Container
//==============================================================================
class PianoRollComponent : public juce::Component {
public:
  PianoRollComponent(HowlingWolvesAudioProcessor &p);
  ~PianoRollComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // Components
  juce::MidiKeyboardState &keyboardState;
  juce::MidiKeyboardComponent keyboardComponent;
  NoteGridComponent noteGrid;

  // Layout Params
  float keyboardWidth = 20.0f; // Compact, professional size
};
