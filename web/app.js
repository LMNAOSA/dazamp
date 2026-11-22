/* DAZAMP standalone application control layer */
'use strict';
(() => {
  const $ = x => document.getElementById(x);
  const engine = new window.DazampAudio();
  const defaults = { tight:69, grind:72, body:61, space:14, inputTrim:0, outputTrim:-5, highCut:6800, gate:-48, cabMix:45, width:70 };
  const factory = [
    {name:'POST HUMAN / RHYTHM', settings:{tight:76,grind:74,body:65,space:12}},
    {name:'THE WALL / HEAVY', settings:{tight:81,grind:88,body:69,space:20}},
    {name:'AFTERLIGHT / LEAD', settings:{tight:55,grind:65,body:74,space:72}},
    {name:'DARK MATTER / AMBIENT', settings:{tight:43,grind:49,body:57,space:92}},
    {name:'COLD GLASS / CLEAN', settings:{tight:42,grind:8,body:47,space:49}},
  ];
  let saved = [];
  let monoCheck = false, tunerEnabled = false, lastPitchCheck = 0;
  try { saved = JSON.parse(localStorage.getItem('dazamp_presets')||'[]').filter(x=>x.name&&x.settings); } catch{}
  const state = {params:{...defaults,...factory[0].settings},snapshots:{A:null,B:null},snapshot:'A',running:false,source:'demo',recording:false};
  state.snapshots.A = {...state.params}; state.snapshots.B = {...state.params,space:72};
  function setStatus(message) { $('sourceStatus').textContent=message; }
  function setEngineStatus() {
    $('engineState').textContent=state.running?'ENGINE LIVE':'ENGINE OFFLINE';
    document.querySelector('.head-right .dot')?.classList.toggle('on',state.running);
    $('powerButton').classList.toggle('on',state.running);
    $('powerButton').setAttribute('aria-pressed',String(state.running));
    $('powerText').textContent=state.running?'POWER OFF':'POWER ON';
  }
  function updateView() {
    Object.entries(state.params).forEach(([name,val])=>{
      const knob=document.querySelector(`.knob[data-param="${name}"]`);
      if(knob) {knob.style.setProperty('--angle',`${-125+val*2.5}deg`);knob.setAttribute('aria-valuenow',String(val));$(name+'Value').textContent=String(Math.round(val)).padStart(2,'0');}
      const advanced=$(name); if(advanced && advanced.type==='range') advanced.value=val;
    });
    $('inputTrimOut').textContent=`${state.params.inputTrim>0?'+':''}${state.params.inputTrim} dB`;
    $('outputTrimOut').textContent=`${state.params.outputTrim>0?'+':''}${state.params.outputTrim} dB`;
    $('highCutOut').textContent=`${state.params.highCut} Hz`;
    $('gateOut').textContent=`${state.params.gate} dB`;
    $('cabOut').textContent=`${state.params.cabMix}% RIBBON`;
    $('widthOut').textContent=`${state.params.width}%`;
  }
  function updateParam(name,value) {
    const num=+value;
    state.params[name]=num;
    state.snapshots[state.snapshot]={...state.params};
    engine.setParams({[name]:num}).catch(err=>setStatus(err.message));
    updateView();
    $('presetSelect').value='custom';
  }
  function renderPresets() {
    const options=[...factory,...saved];
    $('presetSelect').replaceChildren(...options.map((p,i)=>new Option(p.name,String(i))),new Option('CUSTOM / UNSAVED','custom'));
    $('presetSelect').value='0';
  }
  function loadPreset(idx) {
    const selected=[...factory,...saved][idx]; if(!selected)return;
    state.params={...defaults,...selected.settings};
    state.snapshots[state.snapshot]={...state.params};
    engine.setParams(state.params).catch(err=>setStatus(err.message));
    updateView(); $('presetSelect').value=String(idx);
  }
  async function activateSource(which,file) {
    if(!state.running) {setStatus('Turn on the audio engine first.');return;}
    try {
      let message;
      if(which==='demo')message=await engine.setDemo();
      else if(which==='mic')message=await engine.setMic();
      else if(which==='file')message=await engine.setFile(file);
      state.source=which;setStatus(message);
      ['demo','mic'].forEach(s=>$(s+'Button').classList.toggle('active',s===which));
      document.querySelector('.file-label').classList.toggle('active',which==='file');
    } catch(err) {setStatus(err.message);}
  }
  $('powerButton').addEventListener('click',async()=>{
    if(state.running) {
      await engine.stopInput();await engine.context.suspend();state.running=false;setEngineStatus();setStatus('Engine suspended.');return;
    }
    try {
      await engine.init(); await engine.setParams(state.params);
      state.running=true;setEngineStatus();await activateSource('demo');
    } catch(err){setStatus('Could not start audio: '+err.message);}
  });
  $('demoButton').addEventListener('click',()=>activateSource('demo'));
  $('micButton').addEventListener('click',()=>activateSource('mic'));
  $('fileInput').addEventListener('change',e=>{const f=e.target.files[0];if(f)activateSource('file',f);e.target.value='';});
  $('irInput').addEventListener('change',async e=>{
    const f=e.target.files[0]; if(!f)return;
    if(!engine.ready){$('irStatus').textContent='Start the engine, then load a cabinet IR.';return;}
    try{$('irStatus').textContent=await engine.loadIR(f);$('cabMix').disabled=true;$('cabOut').textContent='CUSTOM IR ACTIVE';}catch(err){$('irStatus').textContent=err.message;}
    e.target.value='';
  });
  $('presetSelect').addEventListener('change',e=>{if(e.target.value!=='custom')loadPreset(+e.target.value);});
  for(const [id,direction] of [['prevPreset',-1],['nextPreset',1]]) {
    $(id).addEventListener('click',()=>{
      const total=factory.length+saved.length;const current=+$('presetSelect').value||0;
      loadPreset((current+direction+total)%total);
    });
  }
  document.querySelectorAll('.knob').forEach(el=>{
    const name=el.dataset.param;let drag=null;
    el.addEventListener('pointerdown',e=>{drag={y:e.clientY,value:state.params[name]};el.setPointerCapture(e.pointerId);});
    el.addEventListener('pointermove',e=>{if(!drag)return;updateParam(name,Math.max(0,Math.min(100,Math.round(drag.value+(drag.y-e.clientY)*.65))));});
    el.addEventListener('pointerup',()=>{drag=null;});
    el.addEventListener('pointercancel',()=>{drag=null;});
    el.addEventListener('wheel',e=>{e.preventDefault();updateParam(name,Math.max(0,Math.min(100,state.params[name]+(e.deltaY<0?2:-2))));},{passive:false});
    el.addEventListener('keydown',e=>{
      const delta=e.key==='ArrowUp'||e.key==='ArrowRight'?1:e.key==='ArrowDown'||e.key==='ArrowLeft'?-1:0;
      if(delta){e.preventDefault();updateParam(name,Math.max(0,Math.min(100,state.params[name]+delta)));}
      if(e.key==='Home')updateParam(name,0);
      if(e.key==='End')updateParam(name,100);
    });
  });
  ['inputTrim','outputTrim','highCut','gate','cabMix','width'].forEach(id=>$(id).addEventListener('input',e=>updateParam(id,e.target.value)));
  $('toggleAdvanced').addEventListener('click',()=>{
    $('advancedPanel').hidden=!$('advancedPanel').hidden;
    $('advancedChevron').textContent=$('advancedPanel').hidden?'+':'−';
  });
  function selectSnapshot(letter) {
    if(letter===state.snapshot)return;
    state.snapshots[state.snapshot]={...state.params};
    state.snapshot=letter;
    state.params={...state.snapshots[letter]};
    engine.setParams(state.params);updateView();
    $('aButton').classList.toggle('selected',letter==='A');$('bButton').classList.toggle('selected',letter==='B');
    $('presetSelect').value='custom';
  }
  $('aButton').addEventListener('click',()=>selectSnapshot('A'));
  $('bButton').addEventListener('click',()=>selectSnapshot('B'));
  $('copyAB').addEventListener('click',()=>{
    state.snapshots[state.snapshot==='A'?'B':'A']={...state.params};
    setStatus(`Copied snapshot ${state.snapshot} to ${state.snapshot==='A'?'B':'A'}.`);
  });
  $('monoButton').addEventListener('click', () => {
    monoCheck = !monoCheck;
    engine.setMonoCheck(monoCheck);
    $('monoButton').setAttribute('aria-pressed', String(monoCheck));
    setStatus(monoCheck ? 'MONO CHECK: summed L+R. Listen for lost weight or phase cancellation.' : 'Stereo monitoring restored.');
  });
  $('tunerButton').addEventListener('click', () => {
    tunerEnabled = !tunerEnabled;
    $('tunerPanel').hidden = !tunerEnabled;
    $('tunerButton').setAttribute('aria-pressed', String(tunerEnabled));
    if (tunerEnabled && !state.running) $('tunerHint').textContent = 'Start the engine and connect your guitar to tune.';
  });
  // Export and import user-authored settings only. No code, assets or remote requests.
  $('exportBank').addEventListener('click', () => {
    const doc = {schema:'dazamp-preset-bank-v1', app:'DAZAMP', exportedAt:new Date().toISOString(), presets:saved};
    const blob = new Blob([JSON.stringify(doc, null, 2)], {type:'application/json'});
    const url = URL.createObjectURL(blob), a = document.createElement('a');
    a.href=url; a.download='DAZAMP-preset-bank.json'; a.click();
    setTimeout(() => URL.revokeObjectURL(url), 30000);
    setStatus(`Exported ${saved.length} user presets. Factory presets remain built in.`);
  });
  $('importBankInput').addEventListener('change', async e => {
    const file = e.target.files?.[0]; e.target.value = ''; if (!file) return;
    try {
      if (file.size > 250000) throw Error('Bank is too large (250 KB maximum).');
      const doc = JSON.parse(await file.text());
      if (doc?.schema !== 'dazamp-preset-bank-v1' || !Array.isArray(doc.presets)) throw Error('Not a DAZAMP preset bank.');
      if (doc.presets.length > 100) throw Error('Bank contains too many presets.');
      const keys = Object.keys(defaults);
      const imported = doc.presets.map(p => {
        if (typeof p.name !== 'string' || !p.name.trim() || typeof p.settings !== 'object' || !p.settings) throw Error('Preset is malformed.');
        const settings = {...defaults};
        for (const key of keys) {
          if (Object.prototype.hasOwnProperty.call(p.settings,key)) {
            const val = p.settings[key];
            const slider = $(key);
            const low = slider && slider.type==='range' ? +slider.min : 0;
            const high = slider && slider.type==='range' ? +slider.max : 100;
            if (typeof val !== 'number' || !Number.isFinite(val) || val < low || val > high) throw Error('Preset value out of range: '+key);
            settings[key] = val;
          }
        }
        return {name:p.name.trim().slice(0,45).toUpperCase(),settings};
      });
      const existing = new Set(saved.map(p=>p.name));
      let added = 0;
      for (const preset of imported) {
        if (existing.has(preset.name) || saved.length >= 100) continue;
        saved.push(preset); existing.add(preset.name); added++;
      }
      localStorage.setItem('dazamp_presets', JSON.stringify(saved));
      renderPresets(); setStatus(`Imported ${added} new presets. ${imported.length-added} duplicates or excess skipped.`);
    } catch (err) {setStatus('Preset import failed: '+err.message);}
  });
  $('savePreset').addEventListener('click',()=>{
    const proposed='MY DAZAMP TONE '+String(saved.length+1).padStart(2,'0');
    const name=window.prompt('Name your preset:',proposed)?.trim();if(!name)return;
    saved.push({name:name.slice(0,45).toUpperCase(),settings:{...state.params}});
    try{localStorage.setItem('dazamp_presets',JSON.stringify(saved));}catch{}
    renderPresets();$('presetSelect').value=String(factory.length+saved.length-1);
    setStatus('Saved preset locally in this browser.');
  });
  let recorder=null,chunks=[];
  $('recordButton').addEventListener('click',()=>{
    if(!engine.ready||!state.running){setStatus('Start the engine before recording.');return;}
    if(recorder?.state==='recording'){recorder.stop();return;}
    if(!window.MediaRecorder){setStatus('Recording is unsupported in this browser.');return;}
    try {
      chunks=[];recorder=new MediaRecorder(engine.recordOut.stream);
      recorder.addEventListener('dataavailable',e=>{if(e.data.size)chunks.push(e.data);});
      recorder.addEventListener('stop',()=>{
        const blob=new Blob(chunks,{type:recorder.mimeType||'audio/webm'});
        const a=document.createElement('a');a.href=URL.createObjectURL(blob);
        a.download=`DAZAMP-${new Date().toISOString().slice(0,19).replaceAll(':','-')}.${blob.type.includes('mp4')?'m4a':'webm'}`;
        a.click();setTimeout(()=>URL.revokeObjectURL(a.href),30000);
        $('recordButton').textContent='RECORD OUTPUT';$('recordButton').classList.remove('active');
      });
      recorder.start();$('recordButton').textContent='STOP RECORDING';$('recordButton').classList.add('active');
      setStatus('Recording processed output. Press STOP RECORDING to save.');
    }catch(err){setStatus('Recorder error: '+err.message);}
  });
  const vu=$('vuNeedle'); let lastUpdate=0;
  function drawMeter(t) {
    if(t-lastUpdate>55){
      engine.updateGate();
      if (tunerEnabled && t-lastPitchCheck > 125) {
        lastPitchCheck = t;
        const pitch = engine.getPitch();
        if (pitch && pitch.confidence > .72) {
          $('tunerNote').textContent = pitch.note + pitch.octave;
          $('tunerCents').textContent = `${pitch.cents>=0?'+':''}${pitch.cents.toFixed(0)} CENTS`;
          $('tunerFreq').textContent = pitch.frequency.toFixed(1) + ' Hz';
          $('tunerNeedle').style.left = (50 + pitch.cents*.9) + '%';
          $('tunerNeedle').style.background = Math.abs(pitch.cents)<4 ? '#b9dba7' : '#eab069';
          $('tunerHint').textContent = Math.abs(pitch.cents)<4 ? 'IN TUNE' : pitch.cents<0 ? 'TUNE UP' : 'TUNE DOWN';
        } else {
          $('tunerNote').textContent = '—'; $('tunerCents').textContent = 'WAITING FOR NOTE';
          $('tunerFreq').textContent = '— Hz'; $('tunerNeedle').style.left = '50%';
          $('tunerHint').textContent = state.running ? 'Play one clear note' : 'Start the engine to tune';
        }
      }
      const db=engine.level();
      const angle=Math.max(-70,Math.min(70,(db+36)/36*140-70));
      vu.style.transform=`rotate(${angle}deg)`;
      $('peakLabel').textContent=db<=-85?'−∞ dB':`${db.toFixed(1)} dB`;
      lastUpdate=t;
    }
    requestAnimationFrame(drawMeter);
  }
  renderPresets();updateView();setEngineStatus();requestAnimationFrame(drawMeter);
})();
