#pragma once

#include <JuceHeader.h>
#include <functional>

class LicenseManager {
public:
  LicenseManager();
  ~LicenseManager();

  // True when license.key contains a prior Lemon Squeezy activation (JSON with
  // license_key + instance_id).
  bool loadSavedLicense();

  // Lemon Squeezy License API: validate existing instance and/or activate, then
  // save key + instance_id. Callback always runs on the message thread.
  void verifyLicense(
      const juce::String &licenseKey,
      std::function<void(bool success, const juce::String &message)> callback);

  void saveLicense(const juce::String &licenseKey,
                   const juce::String &instanceId);

  juce::String getSavedKey() const { return savedKey; }
  juce::String getSavedInstanceId() const { return savedInstanceId; }

private:
  juce::String savedKey;
  juce::String savedInstanceId;
};
