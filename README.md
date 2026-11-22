# DAZAMP v0.3 — audible distortion audition + experimental native NAM path

DAZAMP is an independent guitar amp prototype built around four macro controls: **Tight / Grind / Body / Space**. Dark, engineered hardware-style UI. This release prioritises being able to **hear** the amp, not presenting a static mockup.

## Hear it right now (no setup)

- **`DAZAMP-listen.html`** — one-click, offline sound player. Press Play. Switch between `DISTORTED / STEREO`, `DISTORTED / MONO` and `BYPASSED / DRY` at the same place in the riff. Download the current MP3.
- **`DAZAMP-demo.html`** — *interactive amp*, self-contained HTML. Press **POWER ON**: a 7-second riff starts looping through live browser-based overdrive, amp waveshaping, cab processing and ambience. Adjust Tight/Grind/Body/Space. Use **AMP ACTIVE / AMP BYPASSED** to hear the same source directly or processed. Also includes tuner, user audio upload, input interface, cab WAV IR import, presets, A/B, recording, mono check.
- `audio/DAZAMP_DISTORTED_STEREO.mp3` — ready-made double-tracked processed render.
- `audio/DAZAMP_DISTORTED_MONO.mp3` — processed mono reference.
- `audio/DAZAMP_DRY_DI.mp3` — unprocessed DI-like reference.

**Disclosure:** The riff is an **original procedurally synthesised guitar-style DI**, not a real guitar performance, a NAM model, Neural DSP product, Lee Malia recording or transcription of a copyrighted song. The offline sound renders and the browser live DSP are separately implemented reference paths, so they are not sample-for-sample identical. The bundled cabinet tone is an original procedural approximation, not a real 4×12 recorded IR.

For a faithful comparison, record your own guitar DI through a quality high-impedance audio interface, then load a licenced, genuine amp-only NAM capture and licensed V30-style cabinet IR in a compiled Mac plugin.

## Try your guitar in the browser

For microphone/interface input, serve the `web` directory as localhost so browsers allow secure `getUserMedia`:

```bash
cd DAZAMP-v0.3/web
python3 -m http.server 8765
```

Visit `http://localhost:8765` on your Mac, press POWER ON, then GUITAR / INTERFACE. The demo riff is bundled inside the local `web/riff.js` asset, so the instrument-free demo needs no network. Use wired headphones, disable monitoring through the interface and avoid speakers to prevent feedback. For lowest latency or recording, ultimately use the native plugin.

## Native DAW plugin: experimental NAM support (not yet Mac-built)

There is a JUCE source project for AU, VST3 and standalone. Original DAZAMP waveshaping is the default. `--nam` enables an **optional**, experimental C++ model-loading path built using [AmpModeller](https://github.com/JonBellUK/AmpModeller) (which wraps the MIT-licensed [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore)). The wrapper supports processing/model loading with an audio-thread-safe model handoff according to its docs. This integration is source only; it has **not been compiled, signed, DAW-tested or latency-profiled** on Apple Silicon in this environment. It requires external Git dependencies and a **user-supplied `.nam` model** — no unlicensed captures are included.

On your internet-connected Mac with Xcode Command Line Tools, Git and CMake 3.22+ installed:

```bash
cd DAZAMP-v0.3
./build-mac.command --nam
```

Or for the original engine with NAM disabled:

```bash
./build-mac.command
```

The CMake build downloads JUCE (if not already installed). With `--nam` it also downloads the optional model wrapper and its upstream source. Expected build outputs, if successful: `native/build/DAZAMP_artefacts/Release/`. Insert the AU into Logic or VST3 into Reaper after installation/rescan. In the experimental NAM build, **LOAD NAM** loads a local amplifier-only `.nam` model. The project will continue using the original distortion if NAM is disabled or no model has been loaded. The `LOAD CAB IR` control can supply a cabinet impulse. For full-rig captures that already include a speaker, avoid adding another cabinet filter — the current native cab bypass flow is **not yet implemented**, so use amp-only captures for now.

C++ v0.3 NAM code is a *working integration candidate*, **not verified working binary**. Before any release, test parameter recall, sample rates, device latency, DSP CPU, model switching, failure handling, DAW automation, and actual sound versus a known NAM host. Verify the wrapper's licence and JUCE distribution terms before commercialising. Do not distribute third-party `.nam` captures or cab IRs without permission.

## Tests run here

```bash
python tests/test_browser.py
node tests/test_tuner.js
python tests/test_audio.py
```

- Chromium Web Audio sound starts and reports output; audio is decoded from bundled original riff; amp bypass changes output; GUI knobs, tuner/mono UI, IR importer, bank export/import and mobile overflow tested.
- The one-click A/B player decodes its MP3s, switches while playing, preserves playhead and has no browser exceptions in Chromium.
- PCM audio files checked for 48 kHz, sane peaks, stereo separation and changing spectral content after processing.
- **Mac build and NAM engine load have NOT been tested** due to lack of a Mac development runtime and unavailable third-party dependencies in this environment.

## Priority next

1. Mac compile + real native `.nam` capture audition; verify build API against selected upstream revision and fix any issues.
2. Replace the synthetic test riff with a **real clean guitar DI** and properly licensed JCM800 amp-only neural capture and V30-style speaker IR. Correct input calibration and playback gain.
3. Save capture/IR paths in native plugin state, implement proper full-rig cab bypass and model crossfade when changing captures.
4. Test 44.1/48/96 kHz sample rates, latency, silence, denormals, output clipping and performance on Apple Silicon. Finish metallic rendered UI skin.

### Rights

Original DAZAMP JavaScript/C++ prototype and test audio are made for this project. External JUCE, NAM libraries, captures and cabinet IRs carry their own terms, and are not bundled here. No artist or amp manufacturer endorsement is implied.
