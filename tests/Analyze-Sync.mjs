import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
import path from 'node:path';

// Identity comes from captured RGB/tone content, never nearest timestamps.
export function analyze(samples, useWindow=false) {
 const videoRows=(useWindow?samples.windowObservations:samples.videoObservations)??[];
 const audioRows=samples.audioObservations??[], clock=samples.clockObservations??[];
 function pulses(rows) {
  const found=new Map();let candidate=null;
  for(const row of rows) {
   const id=row.pulseId,time=(row.start+row.end)/2;
   if(!Number.isInteger(id)||id<1||id>44){candidate=null;continue;}
   if(candidate?.id===id&&!found.has(id))found.set(id,candidate.time);
   candidate={id,time};
  }
  return found;
 }
 const video=pulses(videoRows),audio=pulses(audioRows);
 const first=clock[0],last=clock.at(-1);
 const stable=clock.length>=2&&clock.every((r,i)=>r.state==='Playing'&&r.rate>0&&r.rate===first.rate&&(!i||r.position>=clock[i-1].position));
 const expected=stable?Array.from({length:44},(_,i)=>i+1).filter(id=>2*id>=first.position+0.5*first.rate&&2*id<=last.position-0.15*first.rate):[];
 const pairs=expected.filter(id=>video.has(id)&&audio.has(id)).map(id=>({id,video:video.get(id),audio:audio.get(id),offsetSeconds:audio.get(id)-video.get(id)}));
 const matched=pairs.filter(p=>Math.abs(p.offsetSeconds)<=0.15);
 const sorted=pairs.map(p=>Math.abs(p.offsetSeconds)).sort((a,b)=>a-b);
 function positionAt(time,id) {
  const after=clock.findIndex(s=>(s.start+s.end)/2>=time);
  if(after<=0)return null;
  const a=clock[after-1],b=clock[after],ta=(a.start+a.end)/2,tb=(b.start+b.end)/2;
  if(tb<=ta||a.rate<=0||a.rate!==b.rate||a.state!=='Playing'||b.state!=='Playing')return null;
  const position=a.position+(b.position-a.position)*(time-ta)/(tb-ta);
  return {time,position,pulseId:id,clockOffsetSeconds:(position-2*id)/a.rate,queryDuration:Math.max(a.end-a.start,b.end-b.start)};
 }
 const reason=!video.size||!audio.size?'Unique pulse identities unavailable':!stable?'Stable playback clock coverage unavailable':expected.length<3?'Fewer than three complete expected pulses':null;
 return {method:useWindow?'window-capture-qpc':'print-window',reason,expectedPulseIds:expected,pairs,
  offsetsSeconds:pairs.map(p=>p.offsetSeconds),videoClock:pairs.map(p=>positionAt(p.video,p.id)).filter(Boolean),
  matchedAudioClock:pairs.map(p=>positionAt(p.audio,p.id)).filter(Boolean),
  matched:matched.length,total:expected.length,paired:pairs.length,
  medianAbsoluteOffset:sorted[Math.floor(sorted.length/2)]??null,
  maxCaptureDuration:videoRows.length?Math.max(...videoRows.map(s=>s.end-s.start)):null,
  passed:!reason&&matched.length>=Math.ceil(expected.length*0.8)};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url))
 console.log(JSON.stringify(analyze(JSON.parse(fs.readFileSync(process.argv[2],'utf8')),process.argv[3]==='window')));
