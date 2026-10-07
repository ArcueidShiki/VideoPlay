import fs from 'node:fs';
// Original fixture flashes white and sounds the same tone together every two
// media seconds. Compare observed presentation, not the player's UI clock.
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
console.log(JSON.stringify({videoOnsets:video,audioOnsets:audio,offsetsSeconds:deltas,
 matched:matched.length,total:video.length,medianAbsoluteOffset:sorted[Math.floor(sorted.length/2)]??null,
 maxCaptureDuration:Math.max(...samples.videoObservations.map(s=>s.end-s.start)),
 passed:video.length>0&&matched.length/video.length>=0.8&&audio.length>0}));
