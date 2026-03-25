#include "PresetManager.h"
#include "SampleManager.h"

const juce::File PresetManager::defaultDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Presets")};

const juce::File PresetManager::factoryDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Factory Presets")};

const juce::File PresetManager::expansionsDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Expansions")};

// ".wav" legacy: preset browser can load a sample file as a one-shot "preset"
const juce::String PresetManager::presetExtension{".wav"};

#if JUCE_MAC
const juce::File PresetManager::sharedDirectory{
    juce::File("/Users/Shared/Wolf Instruments")};
#elif JUCE_WINDOWS
const juce::File PresetManager::sharedDirectory{
    juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
        .getChildFile("Wolf Instruments")};
#else
const juce::File PresetManager::sharedDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Shared")};
#endif

juce::File PresetManager::bundledFactoryDirectory() noexcept {
  const auto exe =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile);
  auto dir = exe.getParentDirectory();
  for (int depth = 0; depth < 10 && dir.exists(); ++depth) {
    const auto factory =
        dir.getChildFile("Resources").getChildFile("Factory");
    if (factory.isDirectory())
      return factory;
    dir = dir.getParentDirectory();
  }
  return {};
}

namespace {
juce::String firstPathSegment(const juce::String &rel) {
  const auto norm = rel.replaceCharacter('\\', '/');
  const int slash = norm.indexOfChar('/');
  if (slash >= 0)
    return norm.substring(0, slash);
  return norm.isEmpty() ? juce::String() : norm;
}

juce::File findSampleUnderRoot(const juce::File &root,
                               const juce::String &samplePath) {
  if (!root.isDirectory() || samplePath.isEmpty())
    return {};

  const juce::File direct = root.getChildFile(samplePath);
  if (direct.existsAsFile())
    return direct;

  const juce::String baseName = juce::File(samplePath).getFileName();
  if (baseName.isEmpty())
    return {};

  const auto found = root.findChildFiles(juce::File::findFiles, true, "*");
  for (const auto &f : found) {
    if (f.getFileName().equalsIgnoreCase(baseName))
      return f;
  }
  return {};
}
} // namespace

juce::String PresetManager::getDisplayNameForFile(const juce::File &file) {
  if (!file.existsAsFile())
    return {};

  if (file.isAChildOf(expansionsDirectory)) {
    const auto rel =
        file.getParentDirectory().getRelativePathFrom(expansionsDirectory);
    const auto pack = firstPathSegment(rel);
    if (pack.isNotEmpty())
      return pack + juce::String(" · ") + file.getFileNameWithoutExtension();
  }

  return file.getFileNameWithoutExtension();
}

juce::String
PresetManager::getCategoryForPresetBrowser(const juce::File &file) {
  if (!file.existsAsFile())
    return "All";

  if (file.isAChildOf(expansionsDirectory)) {
    const auto rel =
        file.getParentDirectory().getRelativePathFrom(expansionsDirectory);
    const auto pack = firstPathSegment(rel);
    return pack.isNotEmpty() ? pack : juce::String("Expansions");
  }

  const auto parent = file.getParentDirectory();
  const auto bundledRoot = bundledFactoryDirectory();
  if (parent != defaultDirectory && parent != factoryDirectory &&
      parent != bundledRoot)
    return parent.getFileName();

  return "All";
}

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts,
                             SampleManager &sm)
    : valueTreeState(apvts), sampleManager(sm) {
  // Do NOT create directories automatically.
  // Rely on user having folders where they want them.
}

// We no longer enforce category subfolders.
// User can organize files as they wish.

void PresetManager::savePreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  auto file = defaultDirectory.getChildFile(presetName + ".xml");
  if (!defaultDirectory.exists())
    defaultDirectory.createDirectory();

  auto state = valueTreeState.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());

  // Store the sample path relative to defaultDirectory so presets are
  // portable across machines with different usernames or drive names.
  auto samplePath = sampleManager.getCurrentSamplePath();
  if (samplePath.isNotEmpty()) {
    juce::File sampleFile(samplePath);
    auto relativePath = sampleFile.getRelativePathFrom(defaultDirectory);
    xml->setAttribute("SamplePath", relativePath);
  }

  xml->writeTo(file);
  currentPresetName = presetName;
  DBG("Saved preset: " + file.getFullPathName());
}

void PresetManager::deletePreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  // Search in all folders
  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  if (presetFile.isAChildOf(factoryDirectory) ||
      presetFile.isAChildOf(bundledFactoryDirectory()) ||
      presetFile.isAChildOf(expansionsDirectory)) {
    DBG("Cannot delete protected preset");
    return;
  }

  if (!presetFile.deleteFile()) {
    DBG("Preset file " + presetFile.getFullPathName() +
        " could not be deleted");
    return;
  }

  currentPresetName = "";
}

void PresetManager::loadPresetFile(const juce::File &presetFile) {
  if (!presetFile.existsAsFile())
    return;

  currentPresetFile = presetFile;
  currentPresetName = getDisplayNameForFile(presetFile);

  if (presetFile.getFileExtension().equalsIgnoreCase(".xml")) {
    auto xml = juce::parseXML(presetFile);
    if (xml != nullptr) {
      valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));

      auto samplePath = xml->getStringAttribute("SamplePath");
      if (samplePath.isNotEmpty()) {
        juce::File sampleFile = defaultDirectory.getChildFile(samplePath);
        if (!sampleFile.existsAsFile())
          sampleFile =
              presetFile.getParentDirectory().getChildFile(samplePath);
        if (!sampleFile.existsAsFile())
          sampleFile = bundledFactoryDirectory().getChildFile(samplePath);
        if (!sampleFile.existsAsFile())
          sampleFile =
              findSampleUnderRoot(expansionsDirectory, samplePath);
        if (!sampleFile.existsAsFile())
          sampleFile = juce::File(samplePath);
        if (sampleFile.existsAsFile()) {
          sampleManager.loadSound(sampleFile, false);
        } else {
          DBG("Sample file not found: " + samplePath);
        }
      }
    }
  } else {
    sampleManager.loadSound(presetFile);
  }
}

void PresetManager::loadPreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  loadPresetFile(presetFile);
}

// Helper to find file across subfolders (User + Factory)
// Helper to find file across subfolders (User + Factory)
juce::File PresetManager::getPresetFile(const juce::String &presetName) const {
  // Helper to search a root directory recursively
  auto findInRoot = [&](const juce::File &root) -> juce::File {
    auto options = juce::File::TypesOfFileToFind::findFiles;
    // Recursive search
    auto allFiles = root.findChildFiles(options, true, "*");
    for (const auto &f : allFiles) {
      if (f.getFileNameWithoutExtension() == presetName) {
        // Check extension (support both .xml and .wav)
        if (f.getFileExtension().equalsIgnoreCase(".xml") ||
            f.getFileExtension().equalsIgnoreCase(presetExtension))
          return f;
      }
    }
    return juce::File();
  };

  auto f = findInRoot(defaultDirectory);
  if (f.existsAsFile())
    return f;

  f = findInRoot(factoryDirectory);
  if (f.existsAsFile())
    return f;

  f = findInRoot(sharedDirectory);
  if (f.existsAsFile())
    return f;

  f = findInRoot(bundledFactoryDirectory());
  if (f.existsAsFile())
    return f;

  f = findInRoot(expansionsDirectory);
  if (f.existsAsFile())
    return f;

  return juce::File();
}

int PresetManager::loadNextPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  int currentIndex = -1;
  // Find current index manually
  for (int i = 0; i < allPresets.size(); ++i) {
    if (allPresets[i].getFullPathName() == currentPresetFile.getFullPathName()) {
      currentIndex = i;
      break;
    }
  }

  const auto nextIndex =
      currentIndex + 1 > allPresets.size() - 1 ? 0 : currentIndex + 1;
  loadPresetFile(allPresets[nextIndex]);
  return nextIndex;
}

int PresetManager::loadPreviousPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  int currentIndex = -1;
  for (int i = 0; i < allPresets.size(); ++i) {
    if (allPresets[i].getFullPathName() == currentPresetFile.getFullPathName()) {
      currentIndex = i;
      break;
    }
  }

  const auto prevIndex =
      currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
  loadPresetFile(allPresets[prevIndex]);
  return prevIndex;
}

juce::File PresetManager::getPresetFolder() const { return defaultDirectory; }

juce::Array<juce::File> PresetManager::getAllPresets() const {
  juce::Array<juce::File> presets;
  std::set<juce::String> loadedNames; // Track unique names

  auto options = juce::File::TypesOfFileToFind::findFiles;

  auto scanRoot = [&](const juce::File &root) {
    DBG("Scanning root: " + root.getFullPathName());
    if (!root.isDirectory()) {
      DBG("Root is not a directory: " + root.getFullPathName());
      return;
    }
    // Recursively find all files
    auto allFiles = root.findChildFiles(options, true, "*"); // Scan everything

    for (const auto &file : allFiles) {
      DBG("Scanning file: " + file.getFileName());
      if (file.getFileExtension().equalsIgnoreCase(presetExtension) ||
          file.getFileExtension().equalsIgnoreCase(".xml")) {

        // Deduplication: Only add if name not yet seen
        auto name = file.getFileName();
        if (loadedNames.find(name) == loadedNames.end()) {
          DBG("Found preset: " + file.getFullPathName());
          presets.add(file);
          loadedNames.insert(name);
        } else {
          DBG("Skipping duplicate preset: " + name);
        }
      } else {
        DBG("Skipping file (wrong extension): " + file.getFileName());
      }
    }
  };

  // Order matters: First scan wins.
  // We prioritize Installed/User locations over Development.
  scanRoot(defaultDirectory);
  scanRoot(sharedDirectory);
  scanRoot(factoryDirectory);
  scanRoot(bundledFactoryDirectory());
  scanRoot(expansionsDirectory);

  return presets;
}

juce::String PresetManager::getCurrentPreset() const {
  return currentPresetName;
}
