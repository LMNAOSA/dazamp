'use strict';
const assert=require('node:assert/strict');
const {detectPitch}=require('../web/tuner.js');
const sr=48000;
const audio=(hz)=>Float32Array.from({length:4096},(_,i)=>.33*Math.sin(2*Math.PI*hz*i/sr)+.07*Math.sin(4*Math.PI*hz*i/sr));
for (const [hz,note,oct] of [[43.6535,'F',1],[49,'G',1],[55,'A',1],[61.7354,'B',1],[82.4069,'E',2],[110,'A',2],[146.8324,'D',3],[196,'G',3],[329.6276,'E',4],[440,'A',4],[659.255,'E',5]]) {
 const pitch=detectPitch(audio(hz),sr);
 assert.ok(pitch,'Missing note '+hz);
 assert.equal(pitch.note,note,'Note at '+hz);
 assert.equal(pitch.octave,oct);
 assert.ok(Math.abs(pitch.cents)<2,'Pitch error too large: '+JSON.stringify(pitch));
}
assert.equal(detectPitch(new Float32Array(4096),sr),null,'Silence must not produce notes');
assert.equal(detectPitch(audio(35),sr),null,'Outside pitch range must reject');
console.log('PASS: eleven guitar pitches, silence, out-of-range source');
