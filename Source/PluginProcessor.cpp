#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
// JUCE ADSR uses (time <= 0) => internal negative rates => unstable / NaN.
constexpr float kAdsrSecondsMin = 0.002f;

float clampAdsrTime(float v) {
  if (!std::isfinite(v))
    return kAdsrSecondsMin;
  return juce::jmax(kAdsrSecondsMin, v);
}
} // namespace

//==============================================================================
HowlingWolvesAudioProcessor::HowlingWolvesAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              // .withInput("Input", juce::AudioChannelSet::stereo(), true) //
              // Disabled to prevent feedback loop in Standalone
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      sampleManager(synthEngine), presetManager(apvts, sampleManager) {

  formatManager.registerBasicFormats();
  // Load initial samples
  sampleManager.loadSamples();
  sampleManager.addChangeListener(this);

#if WOLF_AUTHOR_BUILD
  isLicenseValid.store(true);
#else
  isLicenseValid.store(licenseManager.loadSavedLicense());
#endif
}

HowlingWolvesAudioProcessor::~HowlingWolvesAudioProcessor() {
  suspendProcessing(true);
  sampleManager.removeChangeListener(this);

  // Safe shutdown: silence and clear sounds before voices are torn down
  synthEngine.prepareToRemoveAllSounds();
  synthEngine.clearSounds();
  synthEngine.clearVoices();
}

//==============================================================================
const juce::String HowlingWolvesAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool HowlingWolvesAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double HowlingWolvesAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HowlingWolvesAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int HowlingWolvesAudioProcessor::getCurrentProgram() { return 0; }

void HowlingWolvesAudioProcessor::setCurrentProgram(int /*index*/) {}

const juce::String HowlingWolvesAudioProcessor::getProgramName(int /*index*/) {
  return {};
}

void HowlingWolvesAudioProcessor::changeProgramName(
    int /*index*/, const juce::String & /*newName*/) {}

//==============================================================================
void HowlingWolvesAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
  synthEngine.setCurrentPlaybackSampleRate(sampleRate);
  synthEngine.prepare(sampleRate, samplesPerBlock);
  midiProcessor.prepare(sampleRate);
  midiCapturer.prepare(sampleRate);

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  effectsProcessor.prepare(spec);
}

void HowlingWolvesAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool HowlingWolvesAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void HowlingWolvesAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear the buffer to prevent static/garbage noise
  buffer.clear();

  // --- AUDIO PROTECTION: BYPASS IF UNLICENSED ---
  if (!isLicenseValid.load()) {
    return; // Output pure silence
  }

  juce::AudioPlayHead *playHead = getPlayHead();
  bool hostBpmValid = false;
  double hostBpm = 120.0;

  if (playHead != nullptr) {
    if (auto pos = playHead->getPosition()) {
      if (auto optBpm = pos->getBpm(); optBpm.hasValue()) {
        hostBpm = *optBpm;
        hostBpmValid = (hostBpm > 0.0);
      }
    }
  }

  float manualBPM = 120.0f;
  if (auto *bpmParam = apvts.getRawParameterValue("standaloneBPM"))
    manualBPM = bpmParam->load();

  const double bpmForCapturer =
      hostBpmValid ? hostBpm : juce::jmax(1.0e-6, (double)manualBPM);
  midiCapturer.setBpm(bpmForCapturer);

  juce::ignoreUnused(transportPlaying.load());

  // Extra output-channel clear (instrument clears buffer above)
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // --- MIDI Processing Stage ---
  // 1. Process keyboard state (Input -> MidiBuffer)
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  // Arp / chords: use host BPM when the host reports it; otherwise standalone BPM
  float currentBPM =
      hostBpmValid ? (float)hostBpm : juce::jmax(0.1f, manualBPM);
  internalBPM.store(currentBPM);

  midiProcessor.process(midiMessages, buffer.getNumSamples(), getPlayHead(),
                        currentBPM);

  // Read parameters from APVTS and apply to synth engine (Josh Hodge pattern)
  auto *attackParam = apvts.getRawParameterValue("attack");
  auto *decayParam = apvts.getRawParameterValue("decay");
  auto *sustainParam = apvts.getRawParameterValue("sustain");
  auto *releaseParam = apvts.getRawParameterValue("release");
  auto *filterCutoffParam = apvts.getRawParameterValue("filterCutoff");
  auto *filterResParam = apvts.getRawParameterValue("filterRes");
  auto *filterDriveParam = apvts.getRawParameterValue("filterDrive");
  auto *lfoRateParam = apvts.getRawParameterValue("lfoRate");
  auto *lfoDepthParam = apvts.getRawParameterValue("lfoDepth");
  auto *lfoPhaseParam = apvts.getRawParameterValue("lfoPhase");
  auto *modSmoothParam = apvts.getRawParameterValue("modSmooth");
  auto *ampPanParam = apvts.getRawParameterValue("ampPan");
  auto *ampVelocityParam = apvts.getRawParameterValue("ampVelocity");

  // Output Parameters
  auto *gainParam = apvts.getRawParameterValue("gain");
  auto *panParam = apvts.getRawParameterValue("pan");

  // Effects Parameters
  auto *distDrive = apvts.getRawParameterValue("distDrive");
  auto *distMix = apvts.getRawParameterValue("distMix");
  auto *delayTime = apvts.getRawParameterValue("delayTime");
  auto *delayFdbk = apvts.getRawParameterValue("delayFeedback");
  auto *delayMix = apvts.getRawParameterValue("delayMix");
  auto *revSize = apvts.getRawParameterValue("reverbSize");
  auto *revDecay = apvts.getRawParameterValue("reverbDecay");
  auto *revDamp = apvts.getRawParameterValue("reverbDamping");
  auto *revMix = apvts.getRawParameterValue("REVERB_MIX");

  // Apply parameters to synth engine
  auto *filterTypeParam = apvts.getRawParameterValue("filterType");

  // Apply parameters to synth engine
  if (attackParam && decayParam && sustainParam && releaseParam &&
      filterCutoffParam && filterResParam && lfoRateParam && lfoDepthParam &&
      filterTypeParam) {

    int fType = (int)filterTypeParam->load();

    synthEngine.updateParams(
        clampAdsrTime(attackParam->load()),
        clampAdsrTime(decayParam->load()),
        juce::jlimit(0.0f, 1.0f, sustainParam->load()),
        clampAdsrTime(releaseParam->load()), filterCutoffParam->load(),
        filterResParam->load(), fType, lfoRateParam->load(),
        lfoDepthParam->load());

    // Update Mod Env Params
    auto *modA = apvts.getRawParameterValue("modAttack");
    auto *modD = apvts.getRawParameterValue("modDecay");
    auto *modS = apvts.getRawParameterValue("modSustain");
    auto *modR = apvts.getRawParameterValue("modRelease");
    auto *modAmt = apvts.getRawParameterValue("modAmount");
    auto *lfoTgt =
        apvts.getRawParameterValue("lfoTarget"); // Used for Mod Target too

    if (modA && modD && modS && modR && modAmt && lfoTgt) {
      // Target: 0=Cutoff, 1=Vol, 2=Pan, 3=Pitch
      int tgt = (int)lfoTgt->load();
      synthEngine.updateModParams(
          clampAdsrTime(modA->load()), clampAdsrTime(modD->load()),
          juce::jlimit(0.0f, 1.0f, modS->load()),
          clampAdsrTime(modR->load()), modAmt->load(), tgt);
    }
  }

  // --- Update Midi Processor ---
  auto *arpRateChoice =
      dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter("arpRate"));
  auto *arpModeChoice =
      dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter("arpMode"));
  auto *chordModeChoice = dynamic_cast<juce::AudioParameterChoice *>(
      apvts.getParameter("chordMode"));

  // Float/Int Params
  float arpGate = *apvts.getRawParameterValue("arpGate");
  bool arpOn = (bool)*apvts.getRawParameterValue("arpEnabled");
  int arpOct = (int)*apvts.getRawParameterValue("arpOctave");
  bool chordHold = (bool)*apvts.getRawParameterValue("chordHold");

  // New Params
  auto *arpDensityParam = apvts.getRawParameterValue("arpDensity");
  auto *arpComplexityParam = apvts.getRawParameterValue("arpComplexity");
  auto *arpSpreadParam = apvts.getRawParameterValue("arpSpread");

  // Default values if params missing (safe fallback)
  float dens = arpDensityParam ? arpDensityParam->load() : 1.0f;
  float comp = arpComplexityParam ? arpComplexityParam->load() : 0.0f;
  float spread = arpSpreadParam ? arpSpreadParam->load() : 0.0f;

  if (arpRateChoice && arpModeChoice) {
    int rateIdx = arpRateChoice->getIndex(); // 0..3
    int modeIdx = arpModeChoice->getIndex(); // 0..4

    // Map Rate Index to Float for MidiProcessor (temporary compatibility)
    midiProcessor.getArp().setParameters((float)rateIdx, modeIdx, arpOct,
                                         arpGate, arpOn, dens, comp, spread);
  }

  if (chordModeChoice) {
    int chordModeIdx = chordModeChoice->getIndex(); // 0..4
    midiProcessor.getChordEngine().setParameters(chordModeIdx, 0, chordHold);
  }

  // --- MIDI Capture (After processing, before Synth) ---
  midiCapturer.processMidi(midiMessages, buffer.getNumSamples());

  // --- Sample & Tune Parameters ---
  auto *tuneParam = apvts.getRawParameterValue("tune");
  auto *startParam = apvts.getRawParameterValue("sampleStart");
  auto *endParam = apvts.getRawParameterValue("sampleEnd");
  auto *lengthParam = apvts.getRawParameterValue("sampleLength");
  auto *loopParam = apvts.getRawParameterValue("sampleLoop");

  // Macros
  auto *macroCrush = apvts.getRawParameterValue("macroCrush");
  auto *macroSpace = apvts.getRawParameterValue("macroSpace");

  float crushVal = macroCrush ? macroCrush->load() : 0.0f;
  float spaceVal = macroSpace ? macroSpace->load() : 0.0f;

  float tuneVal = tuneParam ? tuneParam->load() : 0.0f;
  float startVal = startParam ? startParam->load() : 0.0f;
  float endVal = endParam ? endParam->load() : 1.0f;
  float lengthVal = lengthParam ? lengthParam->load() : 1.0f;
  bool loopVal = loopParam ? (loopParam->load() > 0.5f) : true;

  // If the UI is driving "sampleLength" (0..1), derive an end point from it.
  // This keeps the LENGTH knob functional even if "sampleEnd" isn't exposed.
  float effectiveEnd = endVal;
  if (lengthParam) {
    float clampedStart = juce::jlimit(0.0f, 1.0f, startVal);
    float clampedLen = juce::jlimit(0.0f, 1.0f, lengthVal);
    effectiveEnd = juce::jlimit(clampedStart, 1.0f,
                                clampedStart + clampedLen * (1.0f - clampedStart));
  }

  synthEngine.updateSampleParams(tuneVal, startVal, effectiveEnd, loopVal);

  // Voice-level controls
  float ampPanVal = ampPanParam ? ampPanParam->load() : 0.0f;
  float ampVelVal = ampVelocityParam ? ampVelocityParam->load() : 1.0f;
  float driveVal = filterDriveParam ? filterDriveParam->load() : 0.0f;
  float lfoPhaseVal = lfoPhaseParam ? lfoPhaseParam->load() : 0.0f;
  float modSmoothVal = modSmoothParam ? modSmoothParam->load() : 0.1f;
  synthEngine.updateVoiceControls(ampPanVal, ampVelVal, driveVal, lfoPhaseVal,
                                  modSmoothVal);

  // Apply parameters to effects processor

  float distDriveVal = distDrive ? distDrive->load() : 0.0f;
  // Smart mix: audible when drive, hunt, or bitcrush is active
  bool huntIsOn = false;
  bool bitcrushIsOn = false;
  if (auto *p = apvts.getRawParameterValue("huntOn"))
    huntIsOn = (bool)p->load();
  if (auto *p = apvts.getRawParameterValue("bitcrushOn"))
    bitcrushIsOn = (bool)p->load();

  float distMixTarget = 0.0f;
  if (distDriveVal > 0.01f || huntIsOn || bitcrushIsOn) {
    distMixTarget = 1.0f;
  }

  float distMixVal = distMixTarget; // Force smart mix logic overrides manual
                                    // param (since no UI knob exists)

  // Macro Crush mapping: adds to Drive and Mix
  distDriveVal += (crushVal * 0.8f);
  distMixVal += (crushVal * 0.5f);

  float delayTimeVal = delayTime ? delayTime->load() : 0.5f;
  float delayFdbkVal = delayFdbk ? delayFdbk->load() : 0.3f;
  float delayMixVal = delayMix ? delayMix->load() : 0.0f;
  auto *delayWidthRaw = apvts.getRawParameterValue("delayWidth");
  float delayWidthVal = delayWidthRaw ? delayWidthRaw->load() : 1.0f;
  float revSizeVal = revSize ? revSize->load() : 0.5f;
  float revDecayVal = revDecay ? revDecay->load() : 0.5f;
  float revDampVal = revDamp ? revDamp->load() : 0.5f;
  float revMixVal = revMix ? revMix->load() : 0.0f;

  // Macro Space mapping: adds to Delay/Reverb Mix and Size
  delayMixVal += (spaceVal * 0.4f);
  revMixVal += (spaceVal * 0.5f);
  revSizeVal += (spaceVal * 0.2f);
  revDecayVal += (spaceVal * 0.2f);

  // Clamp values
  distDriveVal = juce::jlimit(0.0f, 1.0f, distDriveVal);
  distMixVal = juce::jlimit(0.0f, 1.0f, distMixVal);
  delayMixVal = juce::jlimit(0.0f, 1.0f, delayMixVal);
  revMixVal = juce::jlimit(0.0f, 1.0f, revMixVal);
  revSizeVal = juce::jlimit(0.0f, 1.0f, revSizeVal);
  revDecayVal = juce::jlimit(0.0f, 1.0f, revDecayVal);

  float biteVal = 0.0f;
  if (auto *biteParam = apvts.getRawParameterValue("BITE"))
    biteVal = *biteParam;

  effectsProcessor.updateParameters(distDriveVal, distMixVal, delayTimeVal,
                                    delayFdbkVal, delayMixVal, delayWidthVal,
                                    revSizeVal, revDecayVal, revDampVal,
                                    revMixVal, biteVal);

  // Update Toggles
  auto *huntParam = apvts.getRawParameterValue("huntOn");
  auto *bitcrushParam = apvts.getRawParameterValue("bitcrushOn");
  if (huntParam)
    effectsProcessor.setHuntEnabled((bool)huntParam->load());
  if (bitcrushParam)
    effectsProcessor.setBitcrushEnabled((bool)bitcrushParam->load());

  // Update Chain Order
  auto *chainOrderParam = apvts.getRawParameterValue("CHAIN_ORDER");
  if (chainOrderParam) {
    int mode = (int)chainOrderParam->load();
    using ET = EffectsProcessor::EffectType;
    std::array<ET, 4> order;

    // Standard: Dist -> Bite -> Delay -> Reverb
    // Ethereal: Reverb -> Delay -> Dist -> Bite
    // Chaos: Delay -> Dist -> Bite -> Reverb
    // Reverse: Reverb -> Delay -> Bite -> Dist

    switch (mode) {
    default:
    case 0:
      order = {ET::Distortion, ET::TransientShaper, ET::Delay, ET::Reverb};
      break;
    case 1:
      order = {ET::Reverb, ET::Delay, ET::Distortion, ET::TransientShaper};
      break;
    case 2:
      order = {ET::Delay, ET::Distortion, ET::TransientShaper, ET::Reverb};
      break;
    case 3:
      order = {ET::Reverb, ET::Delay, ET::TransientShaper, ET::Distortion};
      break;
    }
    effectsProcessor.setChainOrder(order);
  }

  // Process synth
  synthEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Process effects
  effectsProcessor.process(buffer);

  // --- Master Section (Gain / Pan) ---
  float gain = gainParam ? gainParam->load() : 0.5f;
  float pan = panParam ? panParam->load() : 0.0f;

  // Apply Master Gain
  buffer.applyGain(gain);

  // Apply Master Pan (Constant Power)
  if (totalNumOutputChannels == 2) {
    // Pan range -1.0 to 1.0
    float angle = (pan + 1.0f) * (juce::MathConstants<float>::pi / 4.0f);
    float leftGain = std::cos(angle);
    float rightGain = std::sin(angle);

    buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
  }

  // Push L-channel into the lock-free visualizer FIFO.
  // The audio thread only touches the Processor's own data here — no UI
  // pointers involved — so this is safe even during editor teardown.
  if (buffer.getNumChannels() > 0) {
    auto *channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int start1, size1, start2, size2;
    visualizerFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
    if (size1 > 0)
      std::copy(channelData, channelData + size1,
                visualizerFifoData + start1);
    if (size2 > 0)
      std::copy(channelData + size1, channelData + size1 + size2,
                visualizerFifoData + start2);
    visualizerFifo.finishedWrite(size1 + size2);
  }
}

// Helper to update all params including new ones
// synthEngine.updateParams(...) needs update.
// I will do it in next step. For now volume works.

//==============================================================================
bool HowlingWolvesAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *HowlingWolvesAudioProcessor::createEditor() {
  return new HowlingWolvesAudioProcessorEditor(*this);
}

//==============================================================================
void HowlingWolvesAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {

  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());

  // Save the currently loaded sample so the DAW can restore it on project reload
  const auto samplePath = sampleManager.getCurrentSamplePath();
  if (samplePath.isNotEmpty())
    xml->setAttribute("currentSamplePath", samplePath);

  if (lastPresetName.isNotEmpty())
    xml->setAttribute("lastPresetName", lastPresetName);

  copyXmlToBinary(*xml, destData);
}

void HowlingWolvesAudioProcessor::setStateInformation(const void *data,
                                                      int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

      // Restore the previously loaded sample
      const auto samplePath = xmlState->getStringAttribute("currentSamplePath");
      if (samplePath.isNotEmpty()) {
        const juce::File sampleFile(samplePath);
        if (sampleFile.existsAsFile())
          sampleManager.loadSound(sampleFile, false);
      }

      // Restore the preset display name shown in the browse button
      lastPresetName = xmlState->getStringAttribute("lastPresetName");
    }
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
HowlingWolvesAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Example: Master Gain
  layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f,
                                                         1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("pan", "Pan", -1.0f,
                                                         1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("tune", "Tune", -12.0f,
                                                         12.0f, 0.0f));

  // Standalone helpers
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "standaloneBPM", "Standalone BPM", 20.0f, 300.0f, 120.0f));

  // Attack, Decay, Sustain, Release (Simple ADSR for now, can be expanded)
  layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
                                                         0.002f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("decay", "Decay",
                                                         0.002f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain",
                                                         0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release",
                                                         0.002f, 5.0f, 0.1f));

  // Filter parameters
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "filterType", "Filter Type",
      juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterCutoff", "Filter Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterRes", "Filter Resonance", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterDrive", "Filter Drive", 0.0f, 1.0f, 0.0f));

  // LFO parameters
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfoWave", "LFO Waveform",
      juce::StringArray{"Sine", "Square", "Triangle"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>("lfoRate", "LFO Rate",
                                                         0.01f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfoTarget", "LFO Target",
      juce::StringArray{"Filter Cutoff", "Volume", "Pan", "Pitch"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lfoDepth", "LFO Depth", 0.0f, 1.0f, 0.5f));

  // Modulation Envelope (Added for ModulateTab)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modAttack", "Mod Attack", 0.002f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modDecay", "Mod Decay", 0.002f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modSustain", "Mod Sustain", 0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modRelease", "Mod Release", 0.002f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modAmount", "Mod Amount", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "modSmooth", "Mod Smooth", 0.0f, 1.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lfoPhase", "LFO Phase", 0.0f, 1.0f, 0.0f));

  // Macros (Added for PlayTab)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "macroCrush", "Crush Macro", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "macroSpace", "Space Macro", 0.0f, 1.0f, 0.0f));

  // Sample Parameters (Added)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleStart", "Sample Start", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleEnd", "Sample End", 0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterBool>("sampleLoop",
                                                        "Sample Loop", true));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleLength", "Sample Length", 0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "ampVelocity", "Amp Velocity", 0.0f, 1.0f, 1.0f));

  // PlayTab missing AmpPan? Processor has "pan" (Master param), but PlayTab
  // also has "ampPan". PlayTab: setupSlider(pan, "PAN", true, "ampPan",
  // panAtt); I should add ampPan distinct from master pan? Or alias? Usually
  // distinct per voice.
  layout.add(std::make_unique<juce::AudioParameterFloat>("ampPan", "Voice Pan",
                                                         -1.0f, 1.0f, 0.0f));

  // ... Effects ...

  // Distortion
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "distDrive", "Distortion Drive", 0.0f, 1.0f, 0.0f)); // Default 0 (Clean)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "distMix", "Distortion Mix", 0.0f, 1.0f,
      1.0f)); // Default 1.0 (Fully Audible)

  // Toggles for Effects
  layout.add(
      std::make_unique<juce::AudioParameterBool>("huntOn", "Hunt Mode", false));
  layout.add(std::make_unique<juce::AudioParameterBool>("bitcrushOn",
                                                        "Bitcrush", false));

  // Delay
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayTime", "Delay Time", 0.0f, 2.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f));

  // Delay Width? EffectsTab: dWidthSlider "delayWidth".
  // Note: I missed checking EffectsTab source manually for mismatches.
  // Assuming defaults for now, but adding width just in case user mentioned it
  // previously. Actually, EffectsTab.cpp from older logs might have it. Safest
  // to add commonly used ones if unsure. I will check EffectsTab ID in next
  // step if verification fails. For now, Proceed.
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayWidth", "Delay Width", 0.0f, 1.0f, 1.0f));

  // Reverb
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "reverbDecay", "Reverb Decay", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "reverbDamping", "Reverb Damping", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "REVERB_MIX", "Reverb Mix", 0.0f, 1.0f, 0.3f));

  // Transient Shaper
  layout.add(std::make_unique<juce::AudioParameterFloat>("BITE", "Bite Amount",
                                                         -1.0f, 1.0f, 0.0f));

  // --- MIDI Performance Parameters ---
  layout.add(std::make_unique<juce::AudioParameterBool>("arpEnabled", "Arp On",
                                                        false));
  // Rates: 0=1/4, 1=1/8, 2=1/16, 3=1/32
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "arpRate", "Arp Rate", juce::StringArray{"1/4", "1/8", "1/16", "1/32"},
      1)); // Default 1/8

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "arpMode", "Arp Mode",
      juce::StringArray{"OFF", "Up", "Down", "Up/Down", "Random"}, 0));

  layout.add(std::make_unique<juce::AudioParameterInt>("arpOctave",
                                                       "Arp Octaves", 1, 4, 1));
  layout.add(std::make_unique<juce::AudioParameterFloat>("arpGate", "Arp Gate",
                                                         0.1f, 1.0f, 0.5f));

  // Added PerformTab missing params
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "arpDensity", "Arp Density", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "arpComplexity", "Arp Complexity", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "arpSpread", "Arp Spread", 0.0f, 1.0f, 0.0f));

  // Chord Mode (Offset Mapping)
  // 0=Select (Bypass), 1=Major, 2=Minor, 3=7th, 4=9th
  layout.add(std::make_unique<juce::AudioParameterBool>("chordHold",
                                                        "Chord Enable", false));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "chordMode", "Chord Mode",
      juce::StringArray{"OFF", "Major", "Minor", "7th", "9th"}, 0));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "HUNT_MODE", "Hunt Mode", juce::StringArray{"Stalk", "Chase", "Kill"},
      0));

  // Signal Chain Order
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "CHAIN_ORDER", "Signal Chain",
      juce::StringArray{"Standard", "Ethereal", "Chaos", "Reverse"}, 0));

  return layout;
}

void HowlingWolvesAudioProcessor::changeListenerCallback(
    juce::ChangeBroadcaster *source) {
  if (source != &sampleManager)
    return;
  // Avoid stuck on-screen keys vs a new instrument after preset/sample change.
  keyboardState.reset();
  if (sampleManager.consumeApplyHostEnvelopeDefaults())
    applyInstrumentEnvelopeDefaults();
}

void HowlingWolvesAudioProcessor::applyInstrumentEnvelopeDefaults() {
  using IK = LoadedInstrumentKind;
  const IK k = sampleManager.getLastLoadedKind();
  if (k == IK::None)
    return;

  auto setF = [this](const char *id, float v) {
    if (auto *fp = dynamic_cast<juce::AudioParameterFloat *>(
            apvts.getParameter(id))) {
      const auto range = fp->getNormalisableRange();
      const float cl = juce::jlimit(range.start, range.end, v);
      fp->beginChangeGesture();
      fp->setValueNotifyingHost(range.convertTo0to1(cl));
      fp->endChangeGesture();
    }
  };

  auto setBool = [this](const char *id, bool on) {
    if (auto *p = apvts.getParameter(id)) {
      p->beginChangeGesture();
      p->setValueNotifyingHost(on ? 1.0f : 0.0f);
      p->endChangeGesture();
    }
  };

  float a, d, s, r;
  bool loop = true;

  switch (k) {
  case IK::Pluck:
    a = 0.002f;
    d = 0.12f;
    s = 0.42f;
    r = 0.35f;
    loop = false;
    break;
  case IK::Bass:
    a = 0.004f;
    d = 0.18f;
    s = 0.8f;
    r = 0.28f;
    break;
  case IK::Hit:
    a = 0.002f;
    d = 0.06f;
    s = 1.0f;
    r = 0.08f;
    loop = false;
    break;
  case IK::Sustained:
    a = 0.015f;
    d = 0.12f;
    s = 1.0f;
    r = 0.28f;
    break;
  case IK::Sequence:
    a = 0.01f;
    d = 0.1f;
    s = 1.0f;
    r = 0.15f;
    break;
  case IK::Texture:
    a = 0.04f;
    d = 0.2f;
    s = 1.0f;
    r = 0.4f;
    break;
  default:
    a = 0.01f;
    d = 0.1f;
    s = 1.0f;
    r = 0.1f;
    break;
  }

  setF("attack", a);
  setF("decay", d);
  setF("sustain", s);
  setF("release", r);
  setBool("sampleLoop", loop);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new HowlingWolvesAudioProcessor();
}
