# DAZAMP / D-800 — v0.2 development release

A standalone/browser guitar amp prototype and a JUCE-based AU, VST3 and standalone **native-source project**. The design is an original dark, machined-metal amplifier UI: four interlinked controls, **Tight / Grind / Body / Space**, intentionally avoiding chains of dozens of plugins.

## Current build status

| Component | Status |
|---|---|
| Browser GUI and sound engine | **Playable and automated-test verified in Chromium** |
| Offline/demo DI riff | **Functional**, procedurally generated, not a real instrument recording |
| Guitar interface input via browser | Implemented; requires localhost/HTTPS and a supported interface; **real hardware untested** |
| Custom WAV cabinet IR | Implemented; tested with a generated WAV impulse |
| Chromatic tuner | Functional, algorithm-tested 43.65–659 Hz, including low drop tunings |
| Mono-check, factory presets, A/B, user bank JSON import/export | Browser tested |
| Browser recording | MediaRecorder/WebM or M4A depending on browser; **not lossless WAV yet** |
| macOS AU, VST3, standalone | **JUCE C++ source only; Mac compilation and Logic/Reaper scan unverified** |
| Neural Amp Modeler / NAM A2 capture playback | **Not integrated** |
| Genuine speaker cab/mic captures | User may import legally usable WAV IR; bundled defaults are **synthetic** |

## Play now

1. Unzip `DAZAMP-v0.2.zip`.
2. Open `DAZAMP-demo.html` in Chrome or Safari.
3. Press **POWER ON**. The procedural DI demo riff should begin looping automatically. Change Tight, Grind, Body, Space to hear the processing alter the sound.
4. For your own recorded dry guitar, use **LOAD DRY AUDIO** (WAV or MP3). For real guitar input, see the localhost instructions below.
5. **TUNER** analyses the raw input, not the distorted output. Low tunings down to approximately F1 are supported when the source is clean and stable.
6. **MONO CHECK** sums left and right equally to check your sound for phase/width problems.
7. **ADVANCED** reveals input/output trim, high cut, gate, synthetic cab blend and width. Loading a custom WAV cabinet IR **bypasses** the synthetic cab-mic blend, so two copies of the same custom IR cannot add an unintended gain bump.
8. **SAVE PRESET** stores a preset in this browser's local storage; **EXPORT PRESETS**/**IMPORT PRESETS** produce or read portable JSON preset banks.

### Live guitar on Mac

Browser microphone capture requires a secure context, so serve locally instead of opening the file directly:

```bash
cd DAZAMP-v0.2/web
python3 -m http.server 8765
```

Open `http://localhost:8765` in Chrome/Safari, choose **GUITAR / INTERFACE**, permit audio input, and select your instrument interface in macOS/browser input settings. Configure the interface for Hi-Z input, disable browser/system echo cancellation when possible, use wired headphones and disable hardware direct monitoring if you don't want to hear the dry guitar over the processed signal. Bluetooth can substantially increase latency.

This is **not yet a calibrated low-latency live-performance rig**: browser input/output routing and device latency vary. Do not infer a production-grade delay or CPU budget from the demonstration.

### DAW plugin on Mac — source build only

You'll need Xcode Command Line Tools, CMake 3.22+ (`brew install cmake`), Git and an internet connection for the JUCE dependency if it isn't installed locally.

```bash
cd DAZAMP-v0.2
./build-mac.command
```

Or manually:

```bash
cd native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
```

If the build succeeds, JUCE normally places AU/VST3/Standalone artefacts under `native/build/DAZAMP_artefacts/Release/`. Install AU in `~/Library/Audio/Plug-Ins/Components/`, or VST3 in `~/Library/Audio/Plug-Ins/VST3/` and rescan the host. **This native build has not been compiled against macOS in this environment and may need fixes.** No signed installer or notarised binary is included.

## Under the bonnet

Browser signal path: **DI/guitar → input trim → envelope noise reduction → high-pass / low-trim → boost / multi-stage 4× oversampled waveshaping → post EQ → cabinet convolution → parallel dry + stereo reflections/delay + reverb → compressor/output**. Stereo listening and a true summed-mono listening path are selectable without changing core tone controls. Tuner listens to pre-distortion input.

Native path: **DI → dynamic gate → pre-EQ/boost → 4× oversampled nonlinear amp stage → body/top-cut EQ → cabinet convolution → parallel stereo delay, reflections and reverb → output**. New width and mono-check parameters are host-automatable. Reverb scratch space and widening delay lines are allocated in `prepareToPlay`, not inside the realtime processing callback. The native and browser engines are not numerically identical yet.

## Where it isn't finished

The sound engine is **original waveshaping DSP**, not Neural DSP's proprietary modelling and not the Neural Amp Modeler inference engine. The included synthetic cabinet response is a stand-in, not a real Marshall JCM800 or V30 cabinet recording. The patch names are descriptive; DAZAMP is not endorsed by, affiliated with or a reproduction of any commercial artist, amp manufacturer, Neural DSP, or STL Tones product.

The high-impact, fully photorealistic metal mockup inspired this UI. The current browser and native GUIs are functional custom-rendered implementations, not that final photographed artwork with animated hardware controls. A subsequent presentation/skin pass can align the exact industrial visual with working knobs, without jeopardising audio behaviour.

### Next engineering priorities

1. Compile and test AU, VST3 and standalone on Apple Silicon, including crash/overload testing, host recall and automation.
2. Integrate a suitably licensed NAM engine and real `.nam` model import, preserving the four-knob macro experience.
3. A/B the DSP against legally usable modern British amp captures and real cabinet IRs using dry guitar DI recordings; measure latency, oversampling aliasing, output headroom and mono compatibility.
4. Add real-time WAV capture/offline bounce, reliable input device/channel selection and processor CPU metering.
5. Persist imported native IR references and snapshot state in host sessions; pursue a polished photoreal hardware skin and proper installer.

## Code, rights and dependencies

Original DAZAMP development source is provided in this archive. JUCE carries separate GPL/commercial terms; check them before distributing a proprietary plugin. Any future NAM, capture or speaker-IR components must be individually checked for licensing and the right to redistribute them. The names Marshall, Neural DSP, STL Tones and Bring Me The Horizon are used only to describe the inspiration/context of tone research.

Run the included test suite for the present browser build:

```bash
node tests/test_tuner.js
python tests/test_browser.py
```

The browser test uses an installed local Chromium at `/usr/bin/chromium` and inlines the demo due to navigation restrictions in the build environment; on your Mac, the demo itself is the thing to audition. For results see `TEST-RESULTS.md`.
