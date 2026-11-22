# DAZAMP v0.2 — verification report

**Build date:** 22 November 2026

## Automated passes

- JavaScript parsed without syntax errors (`node --check` on all three browser sources).
- Chromatic tuner tests: **11 pitches** from F1 (43.65 Hz) to E5 (659.26 Hz), with less than 2 cents error on the synthesized test tones. Silence and an out-of-range 35 Hz test tone correctly rejected.
- Chromium headless browser smoke test: 4 operable macro knobs and 5 factory presets; Web Audio context started, demo signal produced non-silent output; mono-check toggled; tuner panel opened; advanced high-cut changed; A/B snapshots switched; a short actual WAV-format test cabinet impulse decoded, loaded and disabled the synthetic mic blend; a JSON preset bank imported and exported successfully; no uncaught page errors.
- Mobile 390px viewport: four controls rendered and no horizontal document overflow.
- Desktop and mobile screenshots reproduced in `preview-desktop.png` and `preview-mobile.png`.
- Native code review: `wetBuffer` is preallocated in `prepareToPlay`, no `juce::AudioBuffer` created inside `processBlock`. Width lines also allocated in `prepareToPlay`. New host parameters `WIDTH` and `MONO` exposed in JUCE processor and editor source.

## Important limitations

- The Chromium test uses the entire HTML/CSS/JS inlined via `page.set_content` because the environment restricts browser navigation; **there was no Mac GUI, audio interface, DAW host or listening test**.
- Native JUCE source was **not built**. A source review cannot validate compile success, real-time safety of JUCE internals, DSP quality, delay compensation, AU validation, Logic compatibility or plugin state persistence.
- The tuner tests are generated periodic signals; noisy, heavily picked and dynamically changing strings will have lower accuracy than the unit tests.
- An audio level on a synthetic DI demo is **not** evidence that the sound is indistinguishable from a JCM800, Lee Malia tone or Neural DSP.
- The custom-IR test uses an original short generated impulse, not a commercial/licensed 4×12 recording.
