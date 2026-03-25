#include "LicenseConfig.h"
#include "LicenseManager.h"

namespace {

juce::String instanceNameForActivation() {
  juce::String host = juce::SystemStats::getComputerName().trim();
  if (host.isEmpty())
    return "Howling Wolves";
  return "Howling Wolves - " + host;
}

juce::var postLicenseForm(const juce::String &endpoint,
                          std::initializer_list<std::pair<juce::String, juce::String>>
                              fields) {
  juce::URL url(endpoint);
  for (const auto &kv : fields)
    url = url.withParameter(kv.first, kv.second);

  juce::StringPairArray responseHeaders;
  int statusCode = 0;
  auto stream = url.createInputStream(
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
          .withConnectionTimeoutMs(15000)
          .withNumRedirectsToFollow(5)
          .withExtraHeaders(
              "Accept: application/json\r\n"
              "Content-Type: application/x-www-form-urlencoded\r\n")
          .withResponseHeaders(&responseHeaders)
          .withStatusCode(&statusCode));

  if (stream == nullptr)
    return {};

  return juce::JSON::parse(stream->readEntireStreamAsString());
}

juce::String errorStringFromApi(const juce::var &rootVar) {
  auto *o = rootVar.getDynamicObject();
  if (o == nullptr)
    return "Invalid server response.";
  const auto err = o->getProperty("error");
  if (err.isString())
    return err.toString();
  return {};
}

bool metaProductMatches(const juce::DynamicObject *root) {
  if (kLemonSqueezyProductId <= 0) {
    juce::ignoreUnused(root);
    return true;
  }
  if (root == nullptr)
    return false;
  const auto metaVar = root->getProperty("meta");
  auto *meta = metaVar.getDynamicObject();
  if (meta == nullptr)
    return false;
  const int pid = static_cast<int>(meta->getProperty("product_id"));
  return pid == kLemonSqueezyProductId;
}

bool licenseKeyStatusOk(const juce::DynamicObject *lk) {
  if (lk == nullptr)
    return false;
  const auto s = lk->getProperty("status").toString();
  return s != "expired" && s != "disabled";
}

bool handleValidateResponse(const juce::var &json, juce::String &outError) {
  auto *root = json.getDynamicObject();
  if (root == nullptr) {
    outError = "Could not read license response.";
    return false;
  }

  const bool valid = (bool)root->getProperty("valid");
  if (!valid) {
    outError = errorStringFromApi(json);
    if (outError.isEmpty())
      outError = "License is not valid.";
    return false;
  }

  auto *lk = root->getProperty("license_key").getDynamicObject();
  if (!licenseKeyStatusOk(lk)) {
    outError = "This license is expired or disabled.";
    return false;
  }

  if (!metaProductMatches(root)) {
    outError = "This license is not for Howling Wolves.";
    return false;
  }

  return true;
}

bool handleActivateResponse(const juce::var &json, juce::String &instanceId,
                            juce::String &outError) {
  auto *root = json.getDynamicObject();
  if (root == nullptr) {
    outError = "Could not read license response.";
    return false;
  }

  const bool activated = (bool)root->getProperty("activated");
  if (!activated) {
    outError = errorStringFromApi(json);
    if (outError.isEmpty())
      outError = "Could not activate this license key.";
    return false;
  }

  auto *lk = root->getProperty("license_key").getDynamicObject();
  if (!licenseKeyStatusOk(lk)) {
    outError = "This license is expired or disabled.";
    return false;
  }

  if (!metaProductMatches(root)) {
    outError = "This license is not for Howling Wolves.";
    return false;
  }

  auto *inst = root->getProperty("instance").getDynamicObject();
  if (inst == nullptr) {
    outError = "Activation did not return an instance id.";
    return false;
  }

  instanceId = inst->getProperty("id").toString();
  if (instanceId.isEmpty()) {
    outError = "Activation did not return an instance id.";
    return false;
  }

  return true;
}

} // namespace

//==============================================================================
LicenseManager::LicenseManager() {}

LicenseManager::~LicenseManager() {}

bool LicenseManager::loadSavedLicense() {
  juce::File appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("WolfPaccAudio")
          .getChildFile("HowlingWolves");

  if (!appDataDir.exists())
    return false;

  juce::File licenseFile = appDataDir.getChildFile("license.key");
  if (!licenseFile.existsAsFile())
    return false;

  const juce::String txt = licenseFile.loadFileAsString().trim();
  if (txt.isEmpty())
    return false;

  const auto json = juce::JSON::parse(txt);
  auto *o = json.getDynamicObject();
  if (o == nullptr)
    return false;

  savedKey = o->getProperty("license_key").toString().trim();
  savedInstanceId = o->getProperty("instance_id").toString().trim();
  return savedKey.isNotEmpty() && savedInstanceId.isNotEmpty();
}

void LicenseManager::saveLicense(const juce::String &licenseKey,
                                 const juce::String &instanceId) {
  juce::File appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("WolfPaccAudio")
          .getChildFile("HowlingWolves");

  if (!appDataDir.exists())
    appDataDir.createDirectory();

  juce::DynamicObject::Ptr obj = new juce::DynamicObject();
  obj->setProperty("license_key", licenseKey);
  obj->setProperty("instance_id", instanceId);

  juce::File licenseFile = appDataDir.getChildFile("license.key");
  licenseFile.replaceWithText(juce::JSON::toString(juce::var(obj.get()), false));

  savedKey = licenseKey;
  savedInstanceId = instanceId;
}

void LicenseManager::verifyLicense(
    const juce::String &licenseKey,
    std::function<void(bool, const juce::String &)> callback) {

  const juce::String keyTrim = licenseKey.trim();
  const juce::String savedKeyCopy = savedKey;
  const juce::String savedInstCopy = savedInstanceId;

  juce::Thread::launch([this, keyTrim, savedKeyCopy, savedInstCopy,
                        callback]() {
    juce::String err;

    if (keyTrim == savedKeyCopy && savedInstCopy.isNotEmpty()) {
      const auto validateJson = postLicenseForm(
          "https://api.lemonsqueezy.com/v1/licenses/validate",
          {{"license_key", keyTrim}, {"instance_id", savedInstCopy}});

      if (handleValidateResponse(validateJson, err)) {
        saveLicense(keyTrim, savedInstCopy);
        juce::MessageManager::callAsync([callback] {
          callback(true, "License verified successfully!");
        });
        return;
      }
      // fall through to activate (e.g. new machine / instance replaced)
    }

    const auto activateJson = postLicenseForm(
        "https://api.lemonsqueezy.com/v1/licenses/activate",
        {{"license_key", keyTrim},
         {"instance_name", instanceNameForActivation()}});

    juce::String newInstanceId;
    if (handleActivateResponse(activateJson, newInstanceId, err)) {
      saveLicense(keyTrim, newInstanceId);
      juce::MessageManager::callAsync([callback] {
        callback(true, "License activated successfully!");
      });
      return;
    }

    if (err.isEmpty())
      err = "Network error. Please check your internet connection.";

    juce::MessageManager::callAsync([callback, err] { callback(false, err); });
  });
}
