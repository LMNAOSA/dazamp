/* DAZAMP v0.2 pitch estimation — offline-testable YIN-style difference function. */
'use strict';
(function(root) {
  function detectPitch(samples, sampleRate, minHz = 42, maxHz = 1020) {
    if (!samples || samples.length < 1024 || !Number.isFinite(sampleRate)) return null;
    const length = samples.length;
    let power = 0;
    for (let i = 0; i < length; i++) power += samples[i] * samples[i];
    const rms = Math.sqrt(power / length);
    if (rms < 0.008) return null;
    const minLag = Math.max(2, Math.floor(sampleRate / maxHz));
    const maxLag = Math.min(Math.floor(sampleRate / minHz), Math.floor(length / 2) - 1);
    if (maxLag <= minLag) return null;
    // Use a fixed comparison window so the average difference is independent of lag.
    const windowLength = Math.min(1250, length - maxLag);
    const differences = new Float32Array(maxLag + 2);
    for (let lag = 1; lag <= maxLag; lag++) {
      let d = 0;
      for (let i = 0; i < windowLength; i += 2) {
        const z = samples[i] - samples[i + lag];
        d += z * z;
      }
      differences[lag] = d;
    }
    let total = 0, candidate = -1, bestScore = 1;
    const normalized = new Float32Array(maxLag + 2);
    for (let lag = 1; lag <= maxLag; lag++) {
      total += differences[lag];
      normalized[lag] = total ? differences[lag] * lag / total : 1;
    }
    // First sufficiently deep local minimum beats a stronger harmonic an octave down.
    for (let lag = minLag; lag < maxLag; lag++) {
      const score = normalized[lag];
      if (score < 0.18 && score <= normalized[lag + 1]) {
        candidate = lag;
        while (candidate + 1 < maxLag && normalized[candidate + 1] < normalized[candidate]) candidate++;
        bestScore = normalized[candidate];
        break;
      }
    }
    if (candidate < 0) return null;
    const prev = normalized[candidate - 1], mid = normalized[candidate], next = normalized[candidate + 1];
    const denom = prev + next - 2 * mid;
    const refined = candidate + (Math.abs(denom) > 1e-7 ? Math.max(-1, Math.min(1, (prev - next) / (2 * denom))) : 0);
    const frequency = sampleRate / refined;
    if (!Number.isFinite(frequency) || frequency < minHz || frequency > maxHz || bestScore > .18) return null;
    const midi = 69 + 12 * Math.log2(frequency / 440);
    const closestMidi = Math.round(midi);
    const names = ['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','B'];
    return {frequency, midi, note: names[(closestMidi%12+12)%12], octave: Math.floor(closestMidi/12)-1,
      cents: (midi-closestMidi)*100, confidence: Math.max(0,1-bestScore), rms};
  }
  root.DazampTuner = { detectPitch };
  if (typeof module !== 'undefined' && module.exports) module.exports = {detectPitch};
})(typeof window !== 'undefined' ? window : globalThis);
