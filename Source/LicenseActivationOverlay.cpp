#include "LicenseActivationOverlay.h"

//==============================================================================
LicenseActivationOverlay::LicenseActivationOverlay(
    LicenseManager &lm, std::function<void()> onActivationSuccess)
    : licenseManager(lm), onSuccessCallback(onActivationSuccess) {
  // Setup background styling
  setOpaque(false);

  // Title Label
  titleLabel.setText("HOWLING WOLVES VST", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  titleLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(titleLabel);

  // Description Label
  descLabel.setText(
      "Please enter your Gumroad License Key to activate this plugin.",
      juce::dontSendNotification);
  descLabel.setFont(juce::Font(16.0f));
  descLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  descLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(descLabel);

  // Status Label (for errors)
  statusLabel.setText("", juce::dontSendNotification);
  statusLabel.setFont(juce::Font(14.0f));
  statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
  statusLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(statusLabel);

  // Key Input
  keyInput.setMultiLine(false);
  keyInput.setReturnKeyStartsNewLine(false);
  keyInput.setReadOnly(false);
  keyInput.setScrollbarsShown(false);
  keyInput.setCaretVisible(true);
  keyInput.setPopupMenuEnabled(true);
  keyInput.setTextToShowWhenEmpty("e.g. A1B2C3D4-E5F6G7H8-I9J0K1L2-M3N4O5P6",
                                  juce::Colours::grey);
  keyInput.setFont(juce::Font(16.0f, juce::Font::plain));
  keyInput.setJustification(juce::Justification::centred);
  addAndMakeVisible(keyInput);

  // Activate Button
  activateBtn.onClick = [this] { activateClicked(); };
  addAndMakeVisible(activateBtn);

  // Pressing enter in text editor will trigger activation
  keyInput.onReturnKey = [this] { activateClicked(); };
}

LicenseActivationOverlay::~LicenseActivationOverlay() {}

void LicenseActivationOverlay::paint(juce::Graphics &g) {
  // Dark semitransparent overlay or full dark background
  g.fillAll(juce::Colour(0xff121212)); // Solid dark background

  // Center box container background
  auto bounds = getLocalBounds().withSizeKeepingCentre(500, 300).toFloat();
  g.setColour(juce::Colour(0xff1e1e1e));
  g.fillRoundedRectangle(bounds, 12.0f);

  // Outline
  g.setColour(juce::Colour(0xff444444));
  g.drawRoundedRectangle(bounds, 12.0f, 1.0f);
}

void LicenseActivationOverlay::resized() {
  auto bounds = getLocalBounds().withSizeKeepingCentre(400, 250);

  bounds.removeFromTop(20);
  titleLabel.setBounds(bounds.removeFromTop(40));

  bounds.removeFromTop(10);
  descLabel.setBounds(bounds.removeFromTop(30));

  bounds.removeFromTop(30);
  keyInput.setBounds(bounds.removeFromTop(40).withSizeKeepingCentre(350, 40));

  bounds.removeFromTop(20);
  activateBtn.setBounds(
      bounds.removeFromTop(40).withSizeKeepingCentre(200, 40));

  bounds.removeFromTop(10);
  statusLabel.setBounds(bounds.removeFromTop(30));
}

void LicenseActivationOverlay::activateClicked() {
  juce::String key = keyInput.getText().trim();
  if (key.isEmpty()) {
    statusLabel.setText("Please enter a valid license key.",
                        juce::dontSendNotification);
    return;
  }

  statusLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
  statusLabel.setText("Verifying with Gumroad...", juce::dontSendNotification);
  activateBtn.setEnabled(false);
  keyInput.setEnabled(false);

  licenseManager.verifyLicense(key, [this, key](bool success,
                                                const juce::String &message) {
    // Must be called on message thread!
    if (success) {
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
      statusLabel.setText(message, juce::dontSendNotification);

      // Save it
      licenseManager.saveLicense(key);

      // Trigger the success callback (which will hide the overlay)
      if (onSuccessCallback != nullptr)
        onSuccessCallback();
    } else {
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
      statusLabel.setText(message, juce::dontSendNotification);
      activateBtn.setEnabled(true);
      keyInput.setEnabled(true);
    }
  });
}
