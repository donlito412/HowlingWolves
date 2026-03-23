#include "PianoRollComponent.h"

//==============================================================================
// NoteGridComponent
//==============================================================================
NoteGridComponent::NoteGridComponent(HowlingWolvesAudioProcessor &p,
                                     juce::MidiKeyboardState &state)
    : audioProcessor(p), keyboardState(state) {}

NoteGridComponent::~NoteGridComponent() {}

void NoteGridComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour::fromFloatRGBA(0.08f, 0.08f, 0.08f, 1.0f)); // Dark BG

  // Drawing Logic (Professional Grid)
  auto area = getLocalBounds();
  float stepWidth = area.getWidth() / 16.0f;            // 16 steps
  float noteHeight = area.getHeight() / (12.0f * 4.0f); // 4 Octaves visible?
  // Actually we need to sync zooming.

  // Grid Lines
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  for (int i = 0; i <= 16; ++i) {
    float x = i * stepWidth;
    g.drawVerticalLine((int)x, 0.0f, (float)area.getHeight());
    if (i % 4 == 0) {
      g.setColour(juce::Colours::white.withAlpha(0.15f)); // Beats
      g.drawVerticalLine((int)x, 0.0f, (float)area.getHeight());
      g.setColour(juce::Colours::white.withAlpha(0.05f));
    }
  }
}

void NoteGridComponent::resized() {}

void NoteGridComponent::mouseDown(const juce::MouseEvent &e) {
  // Implement Note creation/selection
}

void NoteGridComponent::setZoom(float zx, float zy) {
  zoomX = zx;
  zoomY = zy;
  repaint();
}

//==============================================================================
// PianoRollComponent
//==============================================================================
PianoRollComponent::PianoRollComponent(HowlingWolvesAudioProcessor &p)
    : audioProcessor(p), keyboardState(p.getKeyboardState()),
      keyboardComponent(p.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard),
      noteGrid(p, p.getKeyboardState()) {

  addAndMakeVisible(keyboardComponent);
  addAndMakeVisible(noteGrid);

  // Configure Keyboard for vertical orientation (COMPACT)
  keyboardComponent.setOrientation(
      juce::MidiKeyboardComponent::verticalKeyboardFacingRight);
  keyboardComponent.setAvailableRange(48, 72); // C2 to C4 (2 octaves - compact)
  keyboardComponent.setKeyWidth(20.0f);        // Compact width
  keyboardComponent.setBlackNoteWidthProportion(0.5f); // Narrower black keys
}

PianoRollComponent::~PianoRollComponent() {}

void PianoRollComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void PianoRollComponent::resized() {
  auto area = getLocalBounds();
  keyboardComponent.setBounds(area.removeFromLeft(20)); // Compact 20px width
  noteGrid.setBounds(area);
}
