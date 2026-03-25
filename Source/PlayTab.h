#pragma once
#include "ObsidianLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class PlayTab : public juce::Component, public juce::ChangeListener {
public:
  // Updated constructor to match user request V2
  PlayTab(HowlingWolvesAudioProcessor &p);
  ~PlayTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // --- Helpers ---
  void layoutSample();
  void layoutEnvelope();
  void layoutFilter();
  void layoutMod();
  void layoutMacros();

  void setupKnob(
      juce::Slider &s, const juce::String &n, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupSlider(
      juce::Slider &s, const juce::String &n, bool h,
      const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupButton(juce::TextButton &b,
                   const juce::String &t); // No attachment in V2 snippet? I
                                           // should check if I can add one.
  // User snippet just used logicless button. I will add OnClick or dummy
  // attachment logic if possible.
  void setupLabel(juce::Label &l, const juce::String &t);

  // --- Panels ---
  juce::Rectangle<int> samplePanel, envelopePanel, filterPanel, modPanel,
      macroPanel;

  // --- Controls ---
  // Sample
  // Sample
  juce::Slider sampleStart, sampleLength;
  juce::Slider volSlider; // Master Volume
  juce::TextButton revBtn, loopBtn;

  // Amp Envelope
  juce::Slider att, dec, sus, rel, velocity, pan;

  // Bottom Row
  juce::Slider cutoff, res, drive;
  juce::Slider lfoRate, lfoDepth;
  juce::Slider crushMacro, spaceMacro;

  // Labels
  juce::Label sampleTitle, envTitle, vcfTitle, lfoTitle, macroTitle;

  // Control Labels (V2 True Colors Style)
  juce::Label attLabel, decLabel, susLabel, relLabel, velLabel, panLabel;
  juce::Label cutLabel, resLabel, driveLabel;
  juce::Label rateLabel, depthLabel;
  juce::Label crushLabel, spaceLabel;
  juce::Label startLabel, lenLabel;
  juce::Label volLabel;

  // Visuals
  juce::AudioThumbnail thumbnail;

  // Attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      startAtt, lenAtt, attAtt, decAtt, susAtt, relAtt, velAtt, panAtt, cutAtt,
      resAtt, driveAtt, rateAtt, depthAtt, crushAtt, spaceAtt, volAtt;

  // Missing button attachments usually mapped to parameters, or just standard
  // buttons. User snippet uses TextButton for toggles.

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayTab)
};
