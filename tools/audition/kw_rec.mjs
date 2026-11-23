// K-WASUB's own WebAssembly build: plays events into it and writes a stereo s16 WAV and screen grabs.
// usage: node kw_rec.mjs <ksub.wasm> <events.json> <out.wav> <ms> <shots.json>
import fs from 'fs';
const [wasmPath, evPath, outPath, msStr, shotPath] = process.argv.slice(2);
const SR = 44100, ms = +msStr, BLK = 256;
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), new Proxy({}, { get: () => new Proxy({}, { get: () => () => 0 }) }));
const ex = instance.exports;
ex.ksub_init(); ex.ksub_master(4096);
const ev = JSON.parse(fs.readFileSync(evPath)).sort((a, b) => a[0] - b[0]);
const nblk = Math.ceil(ms * SR / 1000 / BLK), pcm = Buffer.alloc(nblk * BLK * 4), shots = {};
let k = 0;
for (let b = 0; b < nblk; b++) {
  const t = b * BLK * 1000 / SR;
  while (k < ev.length && ev[k][0] <= t) {
    const e = ev[k++];
    if (e[1] === 'midi') ex.ksub_midi(e[2], e[3], e[4]);
    else if (e[1] === 'enc') ex.ksub_turn(e[2], e[3]);
    else if (e[1] === 'button') ex.ksub_button(e[2], e[3]);
    else if (e[1] === 'shot') shots[e[2]] = Buffer.from(new Uint8Array(ex.memory.buffer, ex.ksub_screen(), 240 * 240 * 2)).toString('base64');
  }
  ex.ksub_step();
  const a = new Float32Array(ex.memory.buffer, ex.ksub_audio(), BLK * 2);
  for (let i = 0; i < BLK * 2; i++) pcm.writeInt16LE(Math.max(-32767, Math.min(32767, Math.round(a[i] * 32767))), (b * BLK * 2 + i) * 2);
}
const h = Buffer.alloc(44);
h.write('RIFF', 0); h.writeUInt32LE(36 + pcm.length, 4); h.write('WAVEfmt ', 8); h.writeUInt32LE(16, 16); h.writeUInt16LE(1, 20); h.writeUInt16LE(2, 22);
h.writeUInt32LE(SR, 24); h.writeUInt32LE(SR * 4, 28); h.writeUInt16LE(4, 32); h.writeUInt16LE(16, 34); h.write('data', 36); h.writeUInt32LE(pcm.length, 40);
fs.writeFileSync(outPath, Buffer.concat([h, pcm]));
if (shotPath) fs.writeFileSync(shotPath, JSON.stringify(shots));
