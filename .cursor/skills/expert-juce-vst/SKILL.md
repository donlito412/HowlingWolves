---
name: expert-juce-vst
description: Guides design and implementation of JUCE audio plugins (VST3, AU, Standalone)—CMake setup, AudioProcessor/Editor, APVTS, real-time DSP, MIDI, and host quirks. Use when building, refactoring, or debugging JUCE plugins, VST3, synths/effects, or when the user mentions JUCE (often misspelled "Juice").
---

# Expert JUCE VST / plugin development

**Framework name:** JUCE (not "Juice"). Assume C++ unless the user specifies otherwise.

## This repository

- **Build:** Root `CMakeLists.txt`; JUCE via `FetchContent` (`juce_add_plugin`, `juce_generate_juce_header`).
- **Core types:** `*AudioProcessor` in `Source/PluginProcessor.*`, UI in `Source/PluginEditor.*`; feature code under `Source/`.
- **Match existing patterns** in this codebase (naming, APVTS usage, file layout) before introducing new abstractions.

## Architecture essentials

1. **AudioProcessor** owns DSP state, parameters, presets, and anything that must stay consistent with the audio thread.
2. **AudioProcessorEditor** owns GUI; reads parameter state via attachments/listeners; never performs heavy or blocking work on the audio thread.
3. **Parameters:** Prefer `juce::AudioProcessorValueTreeState` (APVTS) with `AudioProcessorValueTreeState::ParameterLayout`; wire UI with `AudioProcessorValueTreeState::SliderAttachment` / `ButtonAttachment` / `ComboBoxAttachment` as appropriate.
4. **State:** Implement `getStateInformation` / `setStateInformation` (often delegating to APVTS XML); keep serialization backward-compatible when changing parameter IDs.

## Real-time safety (non-negotiable)

In `processBlock` and anything called from it:

- No locks that can block; no allocation (`new`, `std::vector::push_back`, string ops, etc.).
- No file I/O, network, or plugin format APIs that may block.
- Use pre-allocated buffers, atomics or lock-free structures, or copy parameter values at block boundaries from APVTS (`getRawParameterValue`, smoothed values, or cached atomic copies updated from the message thread).

On the **message / GUI thread**: UI updates, preset browser, file dialogs, heavy work.

## CMake / targets

- Prefer **`juce_add_plugin`** with explicit `FORMATS` (e.g. VST3 AU Standalone), `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`, `COMPANY_NAME`, `PRODUCT_NAME`.
- Link JUCE modules with **`target_link_libraries`** on the plugin target; use **`target_sources`** for `Source/*.cpp` you add.
- After adding files or targets, the user rebuilds the same CMake preset they already use for this project.

## Plugin types (CMake flags)

- **Instrument:** `IS_SYNTH TRUE`, MIDI input as needed.
- **Effect:** `IS_SYNTH FALSE`; set bus layout in processor constructor / `isBusesLayoutSupported`.
- **MIDI effect:** `IS_MIDI_EFFECT TRUE` when applicable.

## Common host / format notes

- **VST3:** stable parameter IDs and sensible naming; respect bus layouts and sidechain when implemented.
- **AU:** same processor code; validate channel layouts and Cocoa editor lifetime on macOS.
- **Standalone:** useful for debugging; not a substitute for in-DAW testing.

## Debugging checklist

- Clicks/glitches: buffer size changes, denormals, uninitialized state, race between GUI and audio thread.
- Silence: bus disablement, wrong channel routing, MIDI not reaching processor.
- Crashes on close: editor/processor destruction order, timers not cancelled, listeners not detached.

## When suggesting code

- Prefer JUCE types (`AudioBuffer`, `MidiBuffer`, `dsp::` where it fits) and patterns already used in `Source/`.
- Keep new parameters in APVTS and expose them consistently in editor + state save/load.
- Mention if a change requires **rescan** in the DAW or **reinstall** of the built plugin bundle.
