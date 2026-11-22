// SPDX-License-Identifier: GPL-3.0-only
// The music oracle: what Sketchpad's core (the tested JavaScript) says, for every key, written as
// golden.json for fm1kit's C port to match (factory/oracle/check_music.c).
//   node factory/oracle/gen_golden.mjs /path/to/Sketchpad > factory/oracle/golden.json
import path from "node:path";
import { pathToFileURL } from "node:url";

const sketchpad = process.argv[2];
if (!sketchpad) { console.error("usage: gen_golden.mjs SKETCHPAD_DIR"); process.exit(2); }
const load = (m) => import(pathToFileURL(path.join(sketchpad, "core", m)).href);
const notes = await load("notes.mjs");
const scales = await load("scales.mjs");
const arp = await load("arpeggio.mjs");
const transport = await load("transport.mjs");
const harmony = await load("harmony.mjs");
const chords = await load("chords.mjs");
const melody = await load("melody.mjs");
const figures = await load("figures.mjs");

const ascii = (s) => s.replace(/♭/g, "b").replace(/♯/g, "#");
const out = { source: "Sketchpad core/notes.mjs, core/scales.mjs, core/arpeggio.mjs, core/transport.mjs, core/harmony.mjs, core/chords.mjs", names: [], solfege: [], plain: [], scales: [] };
for (const mode of ["major", "minor"]) {
  for (let tonic = 0; tonic < 12; tonic++) {
    out.names.push({ tonic, minor: mode === "minor" ? 1 : 0, names: notes.keyNames(tonic, mode).map(ascii) });
    const sp = notes.spelling("solfege", tonic, mode);
    out.solfege.push({ tonic, minor: mode === "minor" ? 1 : 0, names: [...Array(12).keys()].map((p) => ascii(notes.noteName(p, sp))) });
  }
}
out.plain = { letters: [...Array(12).keys()].map((p) => notes.noteName(p, "letters")),
              solfege: [...Array(12).keys()].map((p) => notes.noteName(p, "solfege")) };
const ids = ["major", "natural-minor", "major-pentatonic", "minor-pentatonic", "dorian", "harmonic-minor",
             "phrygian", "blues", "mixolydian", "lydian", "melodic-minor", "hirajoshi", "locrian", "major-blues", "lydian-dominant", "phrygian-dominant", "whole-tone"];
for (const id of ids) {
  for (let tonic = 0; tonic < 12; tonic++) out.scales.push({ id, tonic, pcs: scales.scalePcs(tonic, id) });
}
// run orders (core/arpeggio runOrder) over every scale in four keys, both endOn settings, random with seeds 1..4
out.runs = [];
for (const id of ids) {
  for (const tonic of [0, 2, 7, 9]) {
    const notes = scales.scalePcs(tonic, id).map((p) => 60 + ((p - tonic + 12) % 12) + tonic).concat([72 + tonic]);
    for (const pat of ["up", "down", "updown", "random"]) {
      for (const endOn of [false, true]) {
        for (const seed of pat === "random" ? [1, 2, 3, 4] : [1]) {
          out.runs.push({ id, tonic, pat, endOn: endOn ? 1 : 0, seed, notes, order: arp.runOrder(notes, pat, { seed, endOn }) });
        }
      }
    }
  }
}
out.tempo = [30, 40, 60, 61, 90, 133, 160, 200, 240, 250].map((b) => ({ bpm: b, clamp: transport.setTempo(b), beat_ms: Math.floor(transport.beatSeconds(transport.setTempo(b)) * 1000) }));
// chords and keys (D-030): core/harmony harmonize, keyFit, keysForNotes; core/chords inversions
const asciiR = (r) => r.replace(/♭/g, "b").replace(/♯/g, "#").replace(/°/g, "o");
const seven = ["major", "natural-minor", "dorian", "harmonic-minor", "phrygian", "mixolydian", "lydian", "melodic-minor", "locrian",
               "lydian-dominant", "phrygian-dominant"];
out.triads = [];
for (const id of seven) for (let tonic = 0; tonic < 12; tonic++)
  out.triads.push({ id, tonic, chords: harmony.harmonize(tonic, id, 3).map((c) => ({ rootPc: c.rootPc, sym: c.sym, roman: asciiR(c.roman),
    notes: c.notes, inv: chords.inversions(c).map((v) => v.notes) })) });
out.keyfit = [];
const PATS = [[0, 3, 4, 0], [5, 3, 0, 4], [0, 5, 1, 4], [1, 4, 0], [0], [6, 2, 5]];
for (const m of ["major", "minor"]) for (let t = 0; t < 12; t++) {
  const c = harmony.harmonize(t, m === "minor" ? "natural-minor" : "major", 3);
  const progs = PATS.map((p) => p.map((i) => c[i]));
  if (m === "major") progs.push([c[0], c[3], harmony.harmonize(t, "natural-minor", 3)[5], c[0]]);
  for (const prog of progs) for (const [kt, km] of [[t, m], [(t + 7) % 12, m], [(t + 5) % 12, m === "major" ? "minor" : "major"]]) {
    const r = harmony.keyFit(prog, kt, km);
    out.keyfit.push({ tonic: kt, minor: km === "minor" ? 1 : 0, chords: prog.map((x) => ({ root: x.rootPc, notes: x.notes })),
      fits: r.fits, outside: r.outside, ok: r.ok ? 1 : 0, suggest: r.suggest ? [r.suggest.tonic, r.suggest.mode === "minor" ? 1 : 0, r.suggest.fits] : null });
  }
}
out.keyfit.push({ tonic: 0, minor: 0, chords: [], fits: 0, outside: [], ok: 1, suggest: null });
out.keys = [];
let a = 12345;
const rnd = () => { a = (a + 0x6D2B79F5) >>> 0; let x = Math.imul(a ^ (a >>> 15), 1 | a); x = (x + Math.imul(x ^ (x >>> 7), 61 | x)) ^ x; return ((x ^ (x >>> 14)) >>> 0) / 4294967296; };
const cases = [[[48, 1], [53, 1], [48, 1]], [[60, 1]], [[60, 1], [62, 1], [64, 1], [65, 1], [67, 1], [69, 1], [71, 1], [72, 1]],
  [[45, 1], [57, 1], [48, 1], [52, 1], [45, 1]], [[48, 4], [54, 1]], [[48, 1], [51, 1], [53, 1], [54, 1], [55, 1], [58, 1], [48, 1]], []];
for (let i = 0; i < 400; i++) {
  const n = 1 + Math.floor(rnd() * 12), base = 36 + Math.floor(rnd() * 24), tight = rnd() < 0.6;
  cases.push([...Array(n)].map(() => [tight ? base + [0, 2, 4, 5, 7, 9, 11, 12][Math.floor(rnd() * 8)] : 36 + Math.floor(rnd() * 48), 1 + Math.floor(rnd() * 4)]));
}
cases.forEach((ev, i) => {
  const limit = i % 8 === 0 ? 24 : 5;
  const r = harmony.keysForNotes(ev.map(([note, len]) => ({ note, len })), limit);
  out.keys.push({ limit, events: ev, ambiguous: r.ambiguous ? 1 : 0, heard: r.heard,
    candidates: r.candidates.map((c) => ({ tonic: c.tonic, minor: c.mode === "minor" ? 1 : 0, fit: c.fit, outside: c.outside, evidence: c.evidence })) });
});

// colour chords (D-033): core/chords DICTIONARY and identifyChord, core/voicing voicingsFor, core/harmony suggestNextChords
const voicing = await load("voicing.mjs");
const notesM = notes;
const SYMS = ["", "m", "dim", "aug", "sus2", "sus4", "7", "m7", "maj7", "mMaj7", "m7♭5", "dim7", "6", "m6", "add9",
              "m(add9)", "9", "m9", "9sus4", "m11", "13", "m13", "7sus4", "sus4(9)"];
out.colour = [];
for (const sym of SYMS) {
  const d = chords.DICTIONARY.find((x) => x.q === sym);
  for (let root = 0; root < 12; root++) for (const base of [36, 48, 60]) {
    const close = voicing.voice(root, d.iv, base);
    const v = voicing.voicingsFor({ rootPc: root, sym, notes: close, base });
    const inv = chords.inversions({ rootPc: root, notes: close });
    const id = chords.identifyChord(close, "letters");
    out.colour.push({ sym: ascii(sym), root, base, iv: d.iv, close, inv1: inv[1].notes, inv2: inv[2].notes,
      open: v.find((x) => x.id === "open").notes, wide: v.find((x) => x.id === "spread").notes,
      heard: id.length ? ascii(id[0].label) : null });
  }
}
out.labels = [];
for (const mode of ["major", "minor"]) for (let tonic = 0; tonic < 12; tonic++) {
  const sp = notesM.spelling("letters", tonic, mode);
  for (let root = 0; root < 12; root++) out.labels.push({ tonic, minor: mode === "minor" ? 1 : 0, root,
    m9: ascii(chords.chordLabel(root, "m9", sp)) });
}
// what played notes are (D-035): identifyChord, exact, labelled in the key
out.ident = [];
let b = 777;
const rnd2 = () => { b = (b * 1103515245 + 12345) % 2147483648; return b / 2147483648; };
const identCases = [];
for (const sym of SYMS) { const d = chords.DICTIONARY.find((x) => x.q === sym); for (const root of [0, 2, 5, 9, 11]) {
  const close = voicing.voice(root, d.iv, 48); identCases.push(close); identCases.push([...close.slice(1), close[0] + 12]); } }
for (let i = 0; i < 600; i++) { const n = 1 + Math.floor(rnd2() * 6), base = 48 + Math.floor(rnd2() * 12);
  identCases.push([...Array(n)].map(() => base + Math.floor(rnd2() * 19))); }
identCases.forEach((notes, i) => {
  const tonic = i % 12, mode = i % 5 === 0 ? "minor" : "major";
  const r = chords.identifyChord(notes, notesM.spelling("letters", tonic, mode));
  out.ident.push({ tonic, minor: mode === "minor" ? 1 : 0, notes, label: r.length ? ascii(r[0].label) : "", root: r.length ? r[0].rootPc : -1 });
});
// which scales fit (D-041): core/scales fitScales (its top five) for pitch-class sets and tonics
out.fit = [];
let fr = 4242;
const rnd3 = () => { fr = (fr * 1103515245 + 12345) % 2147483648; return fr / 2147483648; };
for (let i = 0; i < 300; i++) {
  const k = 1 + Math.floor(rnd3() * 7), pcsSet = [...new Set([...Array(k)].map(() => Math.floor(rnd3() * 12)))], tonic = Math.floor(rnd3() * 12);
  const r = scales.fitScales([{ notes: pcsSet.map((p) => 60 + p) }], tonic);
  out.fit.push({ tonic, pcs: pcsSet, top: r.map((x) => [x.scale.id, Math.round(x.fit * pcsSet.length)]) });
}
// the closest chord (D-042): identifyChord with missing, its first reading
// the melody guide and figures (D-049): Sketchpad core/melody melodyRole and core/figures renderFigure
out.mrole = [];
for (let c = 0; c < 4096; c += 37) for (let sc = 1; sc < 4096; sc += 523) for (let p = 0; p < 12; p++) {
  const cp = [...Array(12).keys()].filter((q) => c >> q & 1), sp = [...Array(12).keys()].filter((q) => sc >> q & 1);
  out.mrole.push({ pc: p, chord: c, scale: sc, role: ["stable", "movement", "tension"].indexOf(melody.melodyRole(p, cp, sp)) });
}
const ROLES = ["root", "octave", "fifth", "third", "top", "chordTone", "step", "stepUp", "stepDown", "approach"];
const CHORD_IV = [[0, 4, 7], [0, 3, 7], [0, 3, 6], [0, 4, 8], [0, 4, 7, 10], [0, 4, 7, 11], [0, 3, 7, 10], [0, 3, 6, 10], [0, 5, 7], [0, 4, 7, 10, 14], [0, 3, 7, 10, 14]];
out.figures = [];
let figSeed = 4242;
const rnd5 = () => { figSeed = (figSeed * 1103515245 + 12345) % 2147483648; return figSeed / 2147483648; };
for (const pat of figures.PATTERNS.filter((p) => p.kind === "melody")) for (let i = 0; i < 40; i++) {
  const root = Math.floor(rnd5() * 12), ivs = CHORD_IV[Math.floor(rnd5() * CHORD_IV.length)], next = Math.floor(rnd5() * 12);
  const tonic = Math.floor(rnd5() * 12), scale = scales.SCALES[Math.floor(rnd5() * scales.SCALES.length)];
  const set = scale.iv.map((x) => (tonic + x) % 12), seed = Math.floor(rnd5() * 100000), center = 55 + Math.floor(rnd5() * 20);
  const chord = { rootPc: root, notes: ivs.map((x) => 48 + root + x) }, nchord = { rootPc: next, notes: [48 + next] };
  const fig = figures.renderFigure(pat, chord, nchord, set, seed, center);
  out.figures.push({ steps: pat.steps.map((s) => [s.p, s.d, ROLES.indexOf(s.r)]), root, iv: ivs, next, scale: set.reduce((m, p) => m | (1 << p), 0), seed, center,
                     notes: fig.map((x) => [x.midi, x.pos, x.dur]) });
}
out.closest = [];
let cr = 9090;
const rnd4 = () => { cr = (cr * 1103515245 + 12345) % 2147483648; return cr / 2147483648; };
for (let i = 0; i < 400; i++) {
  const n = 3 + Math.floor(rnd4() * 4), base = 48 + Math.floor(rnd4() * 12);
  const notes = [...Array(n)].map(() => base + Math.floor(rnd4() * 20)), tonic = i % 12, mode = i % 3 ? "major" : "minor";
  const r = chords.identifyChord(notes, notesM.spelling("letters", tonic, mode), { missing: true });
  out.closest.push({ tonic, minor: mode === "minor" ? 1 : 0, notes, label: r.length ? ascii(r[0].label) : "", missing: r.length ? r[0].missing : -1 });
}
out.next = [];
for (const mode of ["major", "minor"]) for (let tonic = 0; tonic < 12; tonic++) {
  const pal = harmony.harmonize(tonic, mode === "minor" ? "natural-minor" : "major", 3);
  out.next.push({ tonic, minor: mode === "minor" ? 1 : 0,
    next: pal.map((c) => pal.findIndex((x) => x.rootPc === harmony.suggestNextChords([c], tonic, mode)[0].chord.rootPc)) });
}

console.log(JSON.stringify(out, null, 0));
