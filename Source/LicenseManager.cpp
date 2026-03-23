#include "LicenseManager.h"

LicenseManager::LicenseManager() {}

LicenseManager::~LicenseManager() {}

bool LicenseManager::loadSavedLicense() {
  juce::File appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("WolfPaccAudio")
          .getChildFile("HowlingWolves");

  if (!appDataDir.exists()) {
    return false;
  }

  juce::File licenseFile = appDataDir.getChildFile("license.key");
  if (licenseFile.existsAsFile()) {
    savedKey = licenseFile.loadFileAsString().trim();
    return savedKey.isNotEmpty();
  }

  return false;
}

void LicenseManager::saveLicense(const juce::String &licenseKey) {
  juce::File appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("WolfPaccAudio")
          .getChildFile("HowlingWolves");

  if (!appDataDir.exists()) {
    appDataDir.createDirectory();
  }

  juce::File licenseFile = appDataDir.getChildFile("license.key");
  licenseFile.replaceWithText(licenseKey);
  savedKey = licenseKey;
}

void LicenseManager::verifyLicense(
    const juce::String &licenseKey,
    std::function<void(bool, const juce::String &)> callback) {
  // We launch a thread to avoid blocking the GUI
  juce::Thread::launch([this, licenseKey, callback]() {
    juce::URL gumroadUrl("https://api.gumroad.com/v2/licenses/verify");
    gumroadUrl = gumroadUrl.withParameter("product_permalink", productPermalink)
                     .withParameter("license_key", licenseKey);

    juce::StringPairArray responseHeaders;
    int statusCode = 0;

    // POST request
    std::unique_ptr<juce::InputStream> stream(gumroadUrl.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withConnectionTimeoutMs(10000)
            .withNumRedirectsToFollow(5)
            .withResponseHeaders(&responseHeaders)
            .withStatusCode(&statusCode)));

    if (stream != nullptr) {
      juce::String responseStr = stream->readEntireStreamAsString();
      auto jsonResult = juce::JSON::parse(responseStr);

      if (jsonResult.isObject()) {
        auto *jsonObject = jsonResult.getDynamicObject();

        if (jsonObject->hasProperty("success") &&
            jsonObject->getProperty("success").toString() == "true") {

          if (jsonObject->hasProperty("purchase")) {
            auto *purchaseObj =
                jsonObject->getProperty("purchase").getDynamicObject();
            if (purchaseObj != nullptr &&
                purchaseObj->hasProperty("refunded") &&
                purchaseObj->getProperty("refunded").toString() == "false") {

              // Valid and not refunded!
              juce::MessageManager::callAsync([callback] {
                callback(true, "License verified successfully!");
              });
              return;

            } else {
              juce::MessageManager::callAsync([callback] {
                callback(false, "License was refunded or disabled.");
              });
              return;
            }
          }
        }
      }

      juce::MessageManager::callAsync([callback, responseStr] {
        // Return generic invalid if parsing failed or success=false
        callback(false, "Invalid license key.");
      });

    } else {
      juce::MessageManager::callAsync([callback] {
        callback(false,
                 "Network error. Please check your internet connection.");
      });
    }
  });
}
