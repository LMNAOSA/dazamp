# DAZAMP version history

## v0.2 — 22 November 2026

- Chromatic instrument tuner added, including extended low-metal range to F1.
- True stereo sum-to-mono listening check added in the Web Audio graph.
- Portable JSON preset-bank import and export with strict value and size validation.
- Corrected custom cabinet IR signal routing to avoid unintended dual-convolution gain boost.
- Web audio UX updates: tuner panel, mono-check, import/export controls.
- Native JUCE engine: reverb scratch buffer moved to `prepareToPlay` instead of being allocated every callback.
- Native JUCE engine: separate stereo-reflection delays, host parameters for WIDTH and MONO; editor controls added.
- Updated test suite and visual regression screenshots.

## v0.1 — initial prototype

- Working Web Audio prototype with four tone macros, synthetic demo riff, guitar/file input, cabinets, room ambience, A/B, presets and recorder.
- JUCE source skeleton for AU/VST3/standalone (uncompiled).
