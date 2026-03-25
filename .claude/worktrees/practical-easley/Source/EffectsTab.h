#pragma once
#include "ObsidianLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class EffectsTab : public juce::Component, public juce::Timer {
public:
  EffectsTab(HowlingWolvesAudioProcessor &p);
  ~EffectsTab() override;

  void timerCallback() override;
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // --- Helpers ---
  void setupSlider(
      juce::Slider &s, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupLabel(juce::Label &l, const juce::String &t);
  void setupButton(juce::TextButton &b, const juce::String &t, juce::Colour c);
  void drawEQBars(juce::Graphics &g);
  void layoutSliderGroup(juce::Rectangle<int> bounds,
                         std::vector<juce::Slider *> sliders,
                         juce::Label &title);

  // --- Components ---
  juce::Rectangle<int> delayPanel, reverbPanel, bitePanel,
      eqPanel; // Layout rects only
  // Note: User code used Panels as rects in resized/paint but also
  // addAndMakeVisible(panel)? Wait, user code: "juce::Component
  // distortionPanel..." and "addAndMakeVisible(distortionPanel)". AND
  // "lnf->drawGlassPanel(g, distortionPanel.getBounds())". I will stick to
  // Component for panels to handle z-order and bounds consistently. Actually,
  // user code in Resize used "delayPanel = area.removeFromLeft..." assigning
  // Rect to Component? No. User code was mixed. "juce::Component
  // distortionPanel" declared, but inside resized it assigns result of
  // removeFromLeft. "distortionPanel = area.removeFromLeft..." implies
  // distortionPanel is a Rectangle<int>, NOT a Component. BUT
  // "addAndMakeVisible(distortionPanel)" implies it is a Component. I will use
  // juce::Component for the panels to be safe and cleaner.

  juce::Component delayComp, reverbComp, biteComp, eqComp;

  // Sliders
  juce::Slider delayTime, delayFeedback, delayWidth, delayMix;
  juce::Slider revSize, revDecay, revDamp, revMix;
  juce::Slider biteDial;

  // Buttons
  juce::TextButton huntBtn, bitcrushBtn;

  // Labels
  juce::Label delayTitle, reverbTitle, biteTitle, eqTitle;
  juce::Label dTimeLabel, dFdbkLabel, dWidthLabel, dMixLabel;
  juce::Label rSizeLabel, rDecayLabel, rDampLabel, rMixLabel;

  // Attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      dTimeAtt, dFdbkAtt, dMixAtt, // dWidthAtt? (Missing)
      rSizeAtt, rDampAtt, rMixAtt, // rDecayAtt? (Missing)
      biteAtt;

  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> huntAtt,
      bitcrushAtt;

  // Missing param connections will be left null or unconnected visually

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsTab)
};
