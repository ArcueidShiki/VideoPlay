import http from 'node:http';
import fs from 'node:fs';
const sample = process.argv[2];
const portFile = process.argv[3];
const size = fs.statSync(sample).size;
const server = http.createServer((req, res) => {
  if (req.url === '/stall') { res.writeHead(200, {'Content-Type':'video/mp4'}); res.flushHeaders(); return; }
  if (req.url !== '/sample.mp4') { res.writeHead(404); res.end(); return; }
  const range = /bytes=(\d+)-(\d*)/.exec(req.headers.range || '');
  const start = range ? Number(range[1]) : 0;
  const end = range?.[2] ? Math.min(Number(range[2]), size - 1) : size - 1;
  if (start > end || start >= size) { res.writeHead(416); res.end(); return; }
  const headers = {'Content-Type':'video/mp4', 'Content-Length':end-start+1, 'Accept-Ranges':'bytes'};
  if (range) headers['Content-Range'] = `bytes ${start}-${end}/${size}`;
  res.writeHead(range ? 206 : 200, headers);
  fs.createReadStream(sample, {start, end}).pipe(res);
});
server.listen(0, '127.0.0.1', () => {
  fs.writeFileSync(portFile, String(server.address().port));
  console.log(`Fixture server: http://127.0.0.1:${server.address().port}/sample.mp4`);
});
