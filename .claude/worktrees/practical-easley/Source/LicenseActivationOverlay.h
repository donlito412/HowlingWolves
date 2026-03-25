#pragma once

#include "LicenseManager.h"
#include <JuceHeader.h>

//==============================================================================
/*
    Full-screen overlay: Lemon Squeezy license key entry until activation succeeds.
*/
class LicenseActivationOverlay : public juce::Component {
public:
  LicenseActivationOverlay(LicenseManager &lm,
                           std::function<void()> onActivationSuccess);
  ~LicenseActivationOverlay() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  LicenseManager &licenseManager;
  std::function<void()> onSuccessCallback;

  juce::TextEditor keyInput;
  juce::TextButton activateBtn{"Activate Plugin"};
  juce::Label titleLabel;
  juce::Label descLabel;
  juce::Label statusLabel;

  void activateClicked();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseActivationOverlay)
};
