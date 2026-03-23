#pragma once
#include "ObsidianLookAndFeel.h"
#include "MidiDragComponent.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class PerformTab : public juce::Component,
                   public juce::Button::Listener,
                   public juce::AudioProcessorValueTreeState::Listener {
public:
  PerformTab(HowlingWolvesAudioProcessor &p);
  ~PerformTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void buttonClicked(juce::Button *b) override {}

  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // --- Drawing Helpers ---
  // (Piano roll now handled by PianoRollComponent)

  // --- Layout Helpers ---
  void layoutVoicing();
  void layoutSpread();
  void layoutControls();

  // --- Component Setup ---
  void setupKnob(
      juce::Slider &s, const juce::String &n, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupSlider(
      juce::Slider &s, const juce::String &n, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupButton(juce::TextButton &b, const juce::String &t, juce::Colour c);
  void setupLabel(juce::Label &l, const juce::String &t);

  // --- Panels ---
  juce::Rectangle<int> transportPanel, voicingPanel, spreadPanel, controlsPanel;

  // Transport
  MidiDragComponent midiDrag{audioProcessor};

  // UI State for Transient Menus
  bool showChordMenu = false;
  bool showArpMenu = false;

  // Bottom Modules
  juce::Label voicingTitle, spreadTitle, controlsTitle;
  juce::Label densityLabel, complexityLabel, spreadLabel, octaveLabel;
  juce::Slider densityKnob, complexityKnob;
  juce::Slider spreadWidth, octaveRange;
  juce::TextButton chordHoldBtn, arpSyncBtn;

  // Selectors
  juce::ComboBox arpModeSelector, chordModeSelector;

  // --- Attachments ---
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      densityAtt, complexityAtt, spreadAtt, octaveAtt;
  // Button attachments removed - using manual parameter listeners instead
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      arpModeAtt, chordModeAtt;

  void setupComboBox(
      juce::ComboBox &c, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
          &att);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformTab)
};
