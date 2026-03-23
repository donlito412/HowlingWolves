#pragma once

#include <JuceHeader.h>
#include <functional>

class LicenseManager {
public:
    LicenseManager();
    ~LicenseManager();

    // Check if there is already a saved valid key in the Application Data folder
    bool loadSavedLicense();

    // Make the HTTP request to Gumroad to verify the user-entered key
    // Returns true if verification succeeded, false otherwise.
    void verifyLicense(const juce::String& licenseKey, std::function<void(bool success, const juce::String& message)> callback);

    // Save the key if valid
    void saveLicense(const juce::String& licenseKey);

    juce::String getSavedKey() const { return savedKey; }

private:
    juce::String savedKey;
    juce::String productPermalink = "howlingwolvesvst"; // Hardcoded
};
