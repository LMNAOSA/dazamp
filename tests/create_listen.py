from pathlib import Path
from base64 import b64encode
p=Path(__file__).parents[1]
files=['DAZAMP_DISTORTED_STEREO.mp3','DAZAMP_DISTORTED_MONO.mp3','DAZAMP_DRY_DI.mp3']
assets=[b64encode((p/'audio'/name).read_bytes()).decode() for name in files]
content='''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><meta name="color-scheme" content="dark"><title>DAZAMP | Distortion Audition</title>
<style>
:root{color-scheme:dark;font-family:-apple-system,BlinkMacSystemFont,"Helvetica Neue",Arial,sans-serif}
*{box-sizing:border-box}body{margin:0;min-height:100dvh;background:radial-gradient(ellipse at 50% -15%,#39352d 0%,#151715 45%,#0b0d0b 100%);color:#e5dfd6;display:grid;place-items:center;padding:26px}
.rack{width:min(740px,100%);background:linear-gradient(122deg,#272923,#0d100e 53%,#23231f);border:1px solid #55534d;border-radius:14px;padding:28px 30px;box-shadow:0 15px 38px #000a,inset 0 1px 1px #8886;position:relative}
.top{display:flex;align-items:center;justify-content:space-between;gap:16px;border-bottom:1px solid #67605970;padding-bottom:25px}
h1{margin:0;font-weight:750;letter-spacing:.12em;font-size:clamp(30px,5vw,52px);text-shadow:0 1px 2px #000b;color:#e1ddce}.eyebrow{font-size:10px;letter-spacing:.18em;color:#9a9b92}.led{height:10px;width:10px;border-radius:50%;background:#c48b43;box-shadow:0 0 16px #fdac41}
.sub{margin:24px 0 18px;font-weight:600;letter-spacing:.06em;font-size:17px}.small{color:#a9a69b;font-size:13px;line-height:1.65}.options{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:20px 0}
button{padding:15px 10px;background:linear-gradient(#30332f,#1c201d);border:1px solid #5e625a;color:#c7c7be;border-radius:7px;font-size:11px;letter-spacing:.04em;cursor:pointer;font-weight:700;transition:.12s}
button.active{border-color:#d5a062;background:linear-gradient(#634624,#291f16);color:#ffd8a0;box-shadow:inset 0 0 9px #d99b5430,0 0 10px #9f69392b}
button:focus-visible{outline:3px solid #e4a763;outline-offset:3px}audio{display:block;width:100%;margin:16px 0;accent-color:#d5a062;min-height:56px}
.foot{border-top:1px solid #69625670;margin-top:22px;padding-top:18px;display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}.foot a{color:#f0be80;font-size:12px} .status{color:#d9aa70;font-size:12px;letter-spacing:.08em}.fine{max-width:720px;color:#8b9089;font-size:12px;line-height:1.6;margin:19px auto 0}
@media(max-width:520px){body{padding:12px}.rack{padding:18px 17px}.options{grid-template-columns:1fr;gap:7px}.top{padding-bottom:16px}.sub{margin:18px 0 10px}button{padding:13px}}
</style></head><body><main><section class="rack" aria-label="DAZAMP amp audition"><div class="top"><div><div class="eyebrow">DAZAMP // AUDIO LABORATORY / 003</div><h1>DAZAMP</h1><span class="eyebrow">ENGINEERED FOR IMPACT</span></div><div><span class="led" title="Audio ready"></span></div></div>
<div class="sub">HEAR THE DIFFERENCE.</div><div class="small">One original low-tuned riff. Switch between the dry input, a single amplified track and the double-tracked stereo distortion. Switching retains your place in the riff.</div>
<div class="options"><button id="stereo" class="active" type="button">DISTORTED / STEREO</button><button id="mono" type="button">DISTORTED / MONO</button><button id="dry" type="button">BYPASSED / DRY</button></div>
<div id="status" class="status" aria-live="polite">DISTORTED / STEREO — PRESS PLAY</div>
<audio id="player" controls preload="metadata" aria-label="DAZAMP tone audio player"></audio>
<div class="foot"><span class="eyebrow">REFERENCE RENDER · 48 kHz</span><a id="download" href="#" download="DAZAMP_DISTORTED_STEREO.mp3">SAVE CURRENT AUDIO</a></div></section>
<div class="fine">Original, procedurally synthesised guitar-style DI, processed with DAZAMP's original distortion and procedural cab filter. Not a recorded guitar, Neural DSP preset, Lee Malia recording or NAM capture. The live adjustable DAZAMP demo is included separately.</div></main>
<script>
const tracks={stereo:{name:'DISTORTED / STEREO',file:'DAZAMP_DISTORTED_STEREO.mp3',src:'data:audio/mpeg;base64,%%%STEREO%%%'},mono:{name:'DISTORTED / MONO',file:'DAZAMP_DISTORTED_MONO.mp3',src:'data:audio/mpeg;base64,%%%MONO%%%'},dry:{name:'BYPASSED / DRY',file:'DAZAMP_DRY_DI.mp3',src:'data:audio/mpeg;base64,%%%DRY%%%'}};
const player=document.getElementById('player'), status=document.getElementById('status'), dl=document.getElementById('download');let selected='stereo';
function setTrack(key, keepTime=true){
 const time=keepTime&&Number.isFinite(player.currentTime)?player.currentTime:0;
 const playing=!player.paused;
 selected=key; const track=tracks[key];
 player.pause();player.src=track.src;player.load();
 player.addEventListener('loadedmetadata',function onLoaded(){player.removeEventListener('loadedmetadata',onLoaded);if(time<player.duration)player.currentTime=time;if(playing)player.play().catch(()=>{});},{once:true});
 for(const button of document.querySelectorAll('button')){button.classList.toggle('active',button.id===key);button.setAttribute('aria-pressed',String(button.id===key));}
 dl.href=track.src;dl.download=track.file;
 status.textContent=track.name+(playing?' — PLAYING':' — PRESS PLAY');
}
for(const name of Object.keys(tracks))document.getElementById(name).addEventListener('click',()=>setTrack(name));
player.addEventListener('play',()=>{status.textContent=tracks[selected].name+' — PLAYING'});
player.addEventListener('pause',()=>{status.textContent=tracks[selected].name+' — PAUSED'});
setTrack('stereo',false);
</script></body></html>'''
for tag,audio in zip(['STEREO','MONO','DRY'],assets): content=content.replace('%%%'+tag+'%%%',audio)
(p/'DAZAMP-listen.html').write_text(content)
print('Wrote',p/'DAZAMP-listen.html',len(content),'bytes')
