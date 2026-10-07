import fs from 'node:fs';
// Original fixture flashes white and sounds the same tone together every two
// media seconds. Compare window-rendered pixels with captured process audio.
// The optional playback clock helps diagnose buffering; it cannot prove sync.
const samples=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
function onsets(rows,isOn) {
  const result=[];let previous=false;
  for(const row of rows) {
    const active=isOn(row),time=(row.start+row.end)/2;
    if(active&&!previous&&time>0.5)result.push(time);
    previous=active;
  }
  return result;
}
const video=onsets(samples.videoObservations,r=>r.bright);
const audio=onsets(samples.audioObservations,r=>(r.tone??r.rms)>0.012);
const deltas=video.map(v=>audio.map(a=>a-v).sort((a,b)=>Math.abs(a)-Math.abs(b))[0]).filter(Number.isFinite);
const matched=deltas.filter(d=>Math.abs(d)<=0.15);
const sorted=deltas.map(Math.abs).sort((a,b)=>a-b);
const clock=samples.clockObservations??[];
function positionAt(time) {
  const after=clock.findIndex(s=>(s.start+s.end)/2>=time);
  if(after<=0)return null;
  const a=clock[after-1],b=clock[after],ta=(a.start+a.end)/2,tb=(b.start+b.end)/2;
  if(tb<=ta||a.rate<=0||a.rate!==b.rate||a.state!=='Playing'||b.state!=='Playing')return null;
  const position=a.position+(b.position-a.position)*(time-ta)/(tb-ta);
  return {time,position,nearestPulse:Math.round(position/2)*2,
    clockOffsetSeconds:(position-Math.round(position/2)*2)/a.rate,
    queryDuration:Math.max(a.end-a.start,b.end-b.start)};
}
console.log(JSON.stringify({videoOnsets:video,audioOnsets:audio,offsetsSeconds:deltas,
 videoClock:video.map(positionAt).filter(Boolean),
 matchedAudioClock:video.map(v=>[...audio].sort((a,b)=>Math.abs(a-v)-Math.abs(b-v))[0]).filter(Number.isFinite).map(positionAt).filter(Boolean),
 matched:matched.length,total:video.length,medianAbsoluteOffset:sorted[Math.floor(sorted.length/2)]??null,
 maxCaptureDuration:Math.max(...samples.videoObservations.map(s=>s.end-s.start)),
 passed:video.length>0&&matched.length/video.length>=0.8&&audio.length>0}));
