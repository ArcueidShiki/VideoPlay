import {test} from 'node:test';
import assert from 'node:assert/strict';
import {analyze} from './Analyze-Sync.mjs';
function fixture(delay=0) {
 const rows=shift=>Array.from({length:13},(_,i)=>i+2).flatMap(id=>[0,.02].map(extra=>({start:id*.4+shift+extra,end:id*.4+shift+extra,pulseId:id})));
 const clock=Array.from({length:610},(_,i)=>({start:i*.01,end:i*.01,position:i*.05,rate:5,state:'Playing'}));
 return {videoObservations:rows(0),audioObservations:rows(delay),clockObservations:clock};
}
test('aligned identified pulses pass with sufficient expected coverage',()=>assert.equal(analyze(fixture()).passed,true));
test('5x audio delayed by an entire 400ms pulse period must fail',()=>{
 const result=analyze(fixture(.4));assert.equal(result.passed,false);assert.equal(result.matched,0);
 assert.ok(result.pairs.every(p=>Math.abs(p.offsetSeconds-.4)<1e-8));
});
test('missing most expected pulses cannot pass through a tiny matched subset',()=>{
 const data=fixture();data.videoObservations=data.videoObservations.slice(0,4);data.audioObservations=data.audioObservations.slice(0,4);
 assert.equal(analyze(data).passed,false);
});
test('periodic recordings without identity cannot pass',()=>{
 const data=fixture();for(const rows of [data.videoObservations,data.audioObservations])for(const r of rows)delete r.pulseId;
 assert.equal(analyze(data).passed,false);
});
test('audio pulse cannot be reused for another video identity',()=>{
 const data=fixture();data.audioObservations.forEach(r=>r.pulseId=2);assert.equal(analyze(data).passed,false);
});
test('clock correlation uses identity instead of rounding delayed pulses forward',()=>{
 const result=analyze(fixture(.4));assert.ok(result.matchedAudioClock.every(p=>Math.abs(p.clockOffsetSeconds-.4)<1e-8));
});
