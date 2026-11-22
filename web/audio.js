/* DAZAMP v0.2. Original Web Audio DSP; this is NOT a trained NAM model. */
'use strict';
class DazampAudio {
  constructor() {
    this.context = null;
    this.ready = false;
    this.mediaStream = null;
    this.sourceNode = null;
    this.bufferSource = null;
    this.demoBuffer = null;
    this.oscillators = [];
    this.params = {};
    this.customIR = false;
  }
  gain(db) { return Math.pow(10, db / 20); }
  filter(type, freq, q = 0.707, gain = 0) {
    const f = this.context.createBiquadFilter();
    f.type = type; f.frequency.value = freq; f.Q.value = q; f.gain.value = gain;
    return f;
  }
  makeShaper(amount, bias = 0.0) {
    const points = 16384; const curve = new Float32Array(points);
    const norm = Math.tanh(amount * (1 + Math.abs(bias)));
    for (let i = 0; i < points; i++) {
      const x = i * 2 / (points - 1) - 1;
      const y = Math.tanh(amount * (x + bias)) - Math.tanh(amount * bias);
      curve[i] = Math.max(-1, Math.min(1, y / Math.max(.01, norm)));
    }
    return curve;
  }
  impulseResponse(kind) {
    const c = this.context; const n = 4096; const rate = c.sampleRate;
    const b = c.createBuffer(2, n, rate);
    let seed = kind === 'close' ? 17 : 777;
    const random = () => { seed = (1664525 * seed + 1013904223) >>> 0; return seed / 4294967296; };
    for (let ch = 0; ch < 2; ch++) {
      const d = b.getChannelData(ch);
      d[0] = kind === 'close' ? .78 : .55;
      for (let i = 1; i < n; i++) {
        const t = i / rate;
        const shade = kind === 'close' ? 0.20 : 0.30;
        const early = (random() - .5) * shade * Math.exp(-t * (kind === 'close' ? 250 : 110));
        const ring = 0.06 * Math.sin(2 * Math.PI * (kind === 'close' ? 140 : 205) * t) * Math.exp(-t * 150);
        d[i] = early + ring;
      }
      d[Math.floor(.0067 * rate)] += kind === 'close' ? .15 : .26;
      d[Math.floor(.0142 * rate)] -= kind === 'close' ? .08 : .14;
    }
    return b;
  }
  reverbImpulse(seconds = 2.5) {
    const c = this.context; const n = Math.floor(seconds * c.sampleRate);
    const ir = c.createBuffer(2, n, c.sampleRate);
    let rng = 314159265;
    const rand = () => { rng = (rng * 1664525 + 1013904223) >>> 0; return rng / 4294967296; };
    for (let ch = 0; ch < 2; ch++) {
      const a = ir.getChannelData(ch); let lp = 0;
      for (let i = 0; i < n; i++) {
        const t = i / c.sampleRate;
        lp = lp * .84 + (rand() * 2 - 1) * .16;
        const rise = 1 - Math.exp(-t * 120);
        a[i] = lp * rise * Math.exp(-t * 3.1) * .95;
      }
    }
    return ir;
  }
  async init() {
    if (this.ready) { await this.context.resume(); return; }
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) throw new Error('Web Audio is unavailable in this browser.');
    this.context = new Ctx({ latencyHint: 'interactive' });
    await this.context.resume();
    const c = this.context;
    this.input = c.createGain();
    this.inputMeter = c.createAnalyser(); this.inputMeter.fftSize = 4096;
    this.gateGain = c.createGain(); this.gateEnv = 0; this.gateHold = 0;
    this.preHP = this.filter('highpass', 82);
    this.boostEQ = this.filter('peaking', 910, .8, 3);
    this.boostGain = c.createGain(); this.boostGain.gain.value = 1.5;
    this.boostShape = c.createWaveShaper(); this.boostShape.oversample = '4x'; this.boostShape.curve = this.makeShaper(1.25, .03);
    this.lowTrim = this.filter('lowshelf', 175, .7, -3);
    this.preMid = this.filter('peaking', 900, .9, 3);
    this.preGain = c.createGain();
    this.ampShape = c.createWaveShaper(); this.ampShape.oversample = '4x'; this.ampShape.curve = this.makeShaper(3.7, .045);
    this.ampOutput = c.createGain(); this.ampOutput.gain.value = .63;
    this.postHP = this.filter('highpass', 75);
    this.bodyEQ = this.filter('peaking', 990, .85, 2.3);
    this.presence = this.filter('peaking', 3600, .85, -1);
    this.highCut = this.filter('lowpass', 6800);
    this.cabClose = c.createConvolver(); this.cabRibbon = c.createConvolver();
    this.cabClose.normalize = false; this.cabRibbon.normalize = false;
    this.cabClose.buffer = this.impulseResponse('close');
    this.cabRibbon.buffer = this.impulseResponse('ribbon');
    this.closeGain = c.createGain(); this.ribbonGain = c.createGain();
    this.cabSum = c.createGain();
    this.dryGain = c.createGain();
    this.spaceGain = c.createGain();
    this.ambience = c.createGain();
    this.delayLeft = c.createDelay(2); this.delayRight = c.createDelay(2);
    this.delayLeft.delayTime.value = .215; this.delayRight.delayTime.value = .389;
    this.delayLP = this.filter('lowpass', 4100); this.delayRP = this.filter('lowpass', 3100);
    this.delayFeedbackL = c.createGain(); this.delayFeedbackR = c.createGain();
    this.delayFeedbackL.gain.value = .23; this.delayFeedbackR.gain.value = .18;
    this.delayGain = c.createGain(); this.delayGain.gain.value = .33;
    this.delayPanL = c.createStereoPanner(); this.delayPanR = c.createStereoPanner();
    this.delayPanL.pan.value = -.75; this.delayPanR.pan.value = .75;
    this.widenerLeft = c.createDelay(.1); this.widenerRight = c.createDelay(.1);
    this.widenerLeft.delayTime.value = .018; this.widenerRight.delayTime.value = .028;
    this.widthGainL = c.createGain(); this.widthGainR = c.createGain();
    this.widthPanL = c.createStereoPanner(); this.widthPanR = c.createStereoPanner();
    this.widthPanL.pan.value = -1; this.widthPanR.pan.value = 1;
    this.reverb = c.createConvolver(); this.reverb.normalize = true; this.reverb.buffer = this.reverbImpulse();
    this.revDark = this.filter('lowpass', 3300);
    this.revGain = c.createGain(); this.revGain.gain.value = .10;
    this.stereoMix = c.createGain();
    this.limiter = c.createDynamicsCompressor();
    this.limiter.threshold.value = -7; this.limiter.knee.value = 4; this.limiter.ratio.value = 15; this.limiter.attack.value = .003; this.limiter.release.value = .08;
    this.output = c.createGain();
    this.analyser = c.createAnalyser(); this.analyser.fftSize = 2048; this.analyser.smoothingTimeConstant = .7;
    this.recordOut = c.createMediaStreamDestination();
    this.input.connect(this.inputMeter); this.inputMeter.connect(this.gateGain);
    [this.gateGain, this.preHP, this.boostEQ, this.boostGain, this.boostShape, this.lowTrim,
     this.preMid, this.preGain, this.ampShape, this.ampOutput, this.postHP, this.bodyEQ,
     this.presence, this.highCut].forEach((node, idx, chain) => { if (idx < chain.length - 1) node.connect(chain[idx+1]); });
    this.highCut.connect(this.cabClose); this.highCut.connect(this.cabRibbon);
    this.cabClose.connect(this.closeGain); this.cabRibbon.connect(this.ribbonGain);
    this.closeGain.connect(this.cabSum); this.ribbonGain.connect(this.cabSum);
    this.cabSum.connect(this.dryGain); this.dryGain.connect(this.stereoMix);
    this.cabSum.connect(this.spaceGain);
    this.spaceGain.connect(this.delayLeft); this.spaceGain.connect(this.delayRight);
    this.delayLeft.connect(this.delayLP); this.delayLP.connect(this.delayPanL); this.delayPanL.connect(this.delayGain);
    this.delayLP.connect(this.delayFeedbackL); this.delayFeedbackL.connect(this.delayLeft);
    this.delayRight.connect(this.delayRP); this.delayRP.connect(this.delayPanR); this.delayPanR.connect(this.delayGain);
    this.delayRP.connect(this.delayFeedbackR); this.delayFeedbackR.connect(this.delayRight);
    this.delayGain.connect(this.ambience);
    this.spaceGain.connect(this.widenerLeft); this.spaceGain.connect(this.widenerRight);
    this.widenerLeft.connect(this.widthPanL); this.widenerRight.connect(this.widthPanR);
    this.widthPanL.connect(this.widthGainL); this.widthPanR.connect(this.widthGainR);
    this.widthGainL.connect(this.ambience); this.widthGainR.connect(this.ambience);
    this.spaceGain.connect(this.reverb); this.reverb.connect(this.revDark); this.revDark.connect(this.revGain); this.revGain.connect(this.ambience);
    this.ambience.connect(this.stereoMix);
    this.stereoMix.connect(this.limiter); this.limiter.connect(this.output);
    // Two parallel monitoring paths: stereo normal or equal-power mono sum.
    // Mono-check genuinely sums the two channels (not merely collapsing visual meters).
    this.stereoMonitor = c.createGain(); this.stereoMonitor.gain.value = 1;
    this.monoMonitor = c.createGain(); this.monoMonitor.gain.value = 0;
    this.monoSplit = c.createChannelSplitter(2);
    this.monoLeft = c.createGain(); this.monoLeft.gain.value = 0.5;
    this.monoRight = c.createGain(); this.monoRight.gain.value = 0.5;
    this.monoBus = c.createGain();
    this.monoMerge = c.createChannelMerger(2);
    this.output.connect(this.stereoMonitor); this.stereoMonitor.connect(this.analyser);
    this.output.connect(this.monoSplit);
    this.monoSplit.connect(this.monoLeft, 0); this.monoSplit.connect(this.monoRight, 1);
    this.monoLeft.connect(this.monoBus); this.monoRight.connect(this.monoBus);
    this.monoBus.connect(this.monoMerge, 0, 0); this.monoBus.connect(this.monoMerge, 0, 1);
    this.monoMerge.connect(this.monoMonitor); this.monoMonitor.connect(this.analyser);
    this.analyser.connect(c.destination); this.analyser.connect(this.recordOut);
    this.ready = true;
    await this.setParams({tight:69,grind:72,body:61,space:14,inputTrim:0,outputTrim:-5,highCut:6800,gate:-48,cabMix:45,width:70});
  }
  setMonoCheck(on) {
    if (!this.ready) return;
    const t = this.context.currentTime;
    this.stereoMonitor.gain.setTargetAtTime(on ? 0 : 1, t, .012);
    this.monoMonitor.gain.setTargetAtTime(on ? 1 : 0, t, .012);
  }
  getPitch() {
    if (!this.ready || this.context.state !== 'running' || !window.DazampTuner) return null;
    const samples = new Float32Array(this.inputMeter.fftSize);
    this.inputMeter.getFloatTimeDomainData(samples);
    return window.DazampTuner.detectPitch(samples, this.context.sampleRate);
  }
  async setParams(p) {
    this.params = {...this.params, ...p};
    if (!this.ready) return;
    const q = this.params; const t = this.context.currentTime;
    const smooth = (node, value) => node.setTargetAtTime(value, t, .026);
    const tight = (q.tight ?? 69)/100, grind = (q.grind ?? 72)/100;
    const body = (q.body ?? 61)/100, space = (q.space ?? 14)/100;
    smooth(this.input.gain, this.gain(q.inputTrim ?? 0));
    smooth(this.preHP.frequency, 58 + tight * 90);
    smooth(this.lowTrim.gain, -1 - tight * 6.5);
    smooth(this.boostEQ.gain, 2.2 + tight * 3.7);
    smooth(this.boostGain.gain, 1.15 + tight * .75);
    smooth(this.preMid.gain, 1.2 + grind * 3.8);
    smooth(this.preGain.gain, .60 + grind * 1.55);
    this.ampShape.curve = this.makeShaper(1.7 + 4.8 * grind, .025 + .035 * grind);
    smooth(this.ampOutput.gain, .68 - .18 * grind);
    smooth(this.postHP.frequency, 72 + tight * 24);
    smooth(this.bodyEQ.gain, -1.8 + body * 7.1);
    smooth(this.bodyEQ.frequency, 750 + body * 380);
    smooth(this.presence.gain, -.2 - body * 2.6);
    smooth(this.highCut.frequency, q.highCut ?? 6800);
    const cabMix = (q.cabMix ?? 45)/100;
    // Imported IR is a single microphone response: never double it through
    // both convolution lanes (which otherwise adds unintended 3 dB).
    smooth(this.closeGain.gain, this.customIR ? 1 : Math.cos(cabMix * Math.PI/2));
    smooth(this.ribbonGain.gain, this.customIR ? 0 : Math.sin(cabMix * Math.PI/2));
    smooth(this.spaceGain.gain, space * .72);
    smooth(this.dryGain.gain, 1);
    smooth(this.widthGainL.gain, (q.width ?? 70)/100 * .26);
    smooth(this.widthGainR.gain, (q.width ?? 70)/100 * .26);
    smooth(this.revGain.gain, .08 + space * .23);
    smooth(this.output.gain, this.gain(q.outputTrim ?? -5));
  }
  async setDemo() {
    await this.stopInput();
    if (!this.demoBuffer) this.demoBuffer = this.buildDemo();
    this.startBuffer(this.demoBuffer);
    return 'Synthetic low-tuned riff playing. Replace with your guitar or dry DI for proper tone testing.';
  }
  async setFile(file) {
    await this.stopInput();
    const data = await file.arrayBuffer();
    let buffer;
    try { buffer = await this.context.decodeAudioData(data); }
    catch { throw new Error('Audio file could not be decoded. Try WAV or MP3.'); }
    this.startBuffer(buffer);
    return `Looping ${file.name} (${buffer.duration.toFixed(1)} seconds).`;
  }
  async setMic() {
    if (!navigator.mediaDevices?.getUserMedia) throw new Error('Microphone access requires localhost or HTTPS. Run the included local server.');
    await this.stopInput();
    try {
      this.mediaStream = await navigator.mediaDevices.getUserMedia({audio:{echoCancellation:false,noiseSuppression:false,autoGainControl:false,channelCount:1},video:false});
    } catch(e) { throw new Error('Audio input permission failed: ' + e.message); }
    this.sourceNode = this.context.createMediaStreamSource(this.mediaStream);
    this.sourceNode.connect(this.input);
    return 'Guitar interface connected. Use headphones to avoid feedback. Disable interface direct monitoring.';
  }
  startBuffer(buffer) {
    const n = this.context.createBufferSource();
    n.buffer = buffer; n.loop = true; n.connect(this.input); n.start();
    this.bufferSource = n; this.sourceNode = n;
  }
  async stopInput() {
    if (this.sourceNode) { try { this.sourceNode.disconnect(); } catch{} this.sourceNode = null; }
    if (this.bufferSource) { try { this.bufferSource.stop(); } catch{} this.bufferSource.disconnect(); this.bufferSource = null; }
    if (this.mediaStream) { this.mediaStream.getTracks().forEach(track=>track.stop()); this.mediaStream=null; }
  }
  buildDemo() {
    const c = this.context, sampleRate = c.sampleRate, duration = 8;
    const b = c.createBuffer(1, Math.round(sampleRate*duration), sampleRate);
    const a = b.getChannelData(0);
    let rng = 4399;
    const rnd = () => { rng=(Math.imul(rng,1664525)+1013904223)>>>0; return rng/4294967296; };
    const notes = [61.74,61.74,61.74,82.41,61.74,61.74,92.5,82.41,61.74,61.74,61.74,73.42,61.74,82.41,92.5,55];
    notes.forEach((freq,index)=> {
      const start = Math.floor(index * .48 * sampleRate), len = Math.floor(.34*sampleRate);
      const delay = Math.max(2, Math.floor(sampleRate/freq));
      const line = new Float32Array(delay), palm = index%4===0?1:.75;
      for(let n=0;n<delay;n++)line[n]=(rnd()*2-1)*palm;
      let prev=0;
      for(let i=0;i<len && start+i<a.length;i++){
        const read=i%delay, v=line[read];
        line[read]=.992*(.49*v+.51*prev);prev=v;
        const env=Math.exp(-i/sampleRate*(index%4===3?11:6.2));
        const attack=(rnd()*2-1)*Math.exp(-i/sampleRate*130)*.12;
        a[start+i]+=v*env*.24+attack;
      }
    });
    return b;
  }
  async loadIR(file) {
    if (file.size > 5_000_000) throw new Error('IR too large. Select a WAV under 5 MB.');
    const buffer = await this.context.decodeAudioData(await file.arrayBuffer());
    if (buffer.duration > 4 || buffer.duration < .001)
      throw new Error('Cab IR duration must be between 1 ms and 4 seconds.');
    this.cabClose.buffer = buffer;
    this.customIR = true;
    await this.setParams(this.params);
    return `Loaded ${file.name} · ${(buffer.duration*1000).toFixed(0)} ms. Custom IR active; built-in mic blend bypassed.`;
  }
  updateGate() {
    if (!this.ready || this.context.state !== 'running') return;
    // Gentle, adaptive noise reduction. Ramping avoids abrupt discontinuities.
    const arr = new Float32Array(this.inputMeter.fftSize);
    this.inputMeter.getFloatTimeDomainData(arr);
    let sum = 0;
    for (const v of arr) sum += v * v;
    const rms = Math.sqrt(sum / arr.length);
    const threshold = this.gain(this.params.gate ?? -48);
    // Hold the gate after a transient, and never hard-mute the guitar's tail.
    if (rms > threshold * 1.2) this.gateHold = 9;
    else this.gateHold = Math.max(0, this.gateHold - 1);
    const target = this.gateHold > 0 ? 1 : (rms < threshold * .5 ? .18 : .65);
    this.gateGain.gain.setTargetAtTime(target, this.context.currentTime, target > this.gateGain.gain.value ? .005 : .035);
  }
  level() {
    if (!this.ready || this.context.state !== 'running') return -90;
    const arr = new Float32Array(this.analyser.fftSize);
    this.analyser.getFloatTimeDomainData(arr);
    let sum = 0; for(let i=0;i<arr.length;i++)sum+=arr[i]*arr[i];
    return 20*Math.log10(Math.max(1e-5,Math.sqrt(sum/arr.length)));
  }
}
window.DazampAudio = DazampAudio;
