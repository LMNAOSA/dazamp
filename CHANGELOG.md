# DAZAMP release changes

## v0.3
- Created an original 7.1-second riff and reproducible offline distortion reference renderer (`audio/render_demo.py`). Exported PCM WAV and MP3 in dry, processed mono and processed double-tracked stereo versions.
- Added offline, one-click `DAZAMP-listen.html` with native sound playback and switching between pre-rendered references while preserving play position.
- Embedded the original DI-like riff in `DAZAMP-demo.html`; POWER ON routes it through the browser's actual live distortion/cab/ambience processing, rather than relying on the original unbundled procedural note generator.
- Added genuine dry/wet monitoring bypass so users can toggle the same riff between unprocessed and DAZAMP-processed audio without switching sound sources.
- Updated C++ JUCE source with optional *experimental* `.nam` model-loading path using the `AmpModeller` wrapper (NeuralAmpModelerCore under the hood). `./build-mac.command --nam` enables this path when building on Mac. Not compiled here.
- Added audible asset and browser tests, improved accurate project documentation.

## v0.2
- Browser tuner, mono check, preset import/export; native realtime scratch allocation improvements.
