#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: fm1kit's music.c gives Sketchpad's answers for every key (golden.json, from gen_golden.mjs)."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
exe = ROOT / "work" / "oracle" / "dump_music"
exe.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([os.environ.get("CC", "cc"), "-O1", "-Wall", f"-I{ROOT / 'fm1kit' / 'include'}",
                str(HERE / "dump_music.c"), str(ROOT / "fm1kit" / "src" / "music.c"), "-o", str(exe)], check=True)
got = json.loads(subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout)
want = json.loads((HERE / "golden.json").read_text())
bad = []
for kind in ("names", "solfege"):
    for g, w in zip(got[kind], want[kind]):
        for p in range(12):
            if g["names"][p] != w["names"][p]:
                bad.append(f"{kind} tonic {w['tonic']} {'minor' if w['minor'] else 'major'} pc {p}: {g['names'][p]} not {w['names'][p]}")
for kind in ("letters", "solfege"):
    if got["plain"][kind] != want["plain"][kind]:
        bad.append(f"plain {kind}: {got['plain'][kind]} not {want['plain'][kind]}")
wscales = {(s["id"], s["tonic"]): s["pcs"] for s in want["scales"]}
for s in got["scales"]:
    w = wscales.get((s["id"], s["tonic"]))
    if w is None:
        bad.append(f"scale {s['id']}: not in Sketchpad")
        continue
    if s["pcs"] != w:
        bad.append(f"scale {s['id']} tonic {s['tonic']}: {s['pcs']} not {w}")
    if [p for p in range(12) if s["has"][p]] != sorted(w):
        bad.append(f"scale {s['id']} tonic {s['tonic']}: kit_scale_has disagrees")
PAT = {"up": 0, "down": 1, "updown": 2, "random": 3}
runs = want.get("runs", [])
feed = "".join(f"{PAT[r['pat']]} {r['endOn']} {r['seed']} {len(r['notes'])} {' '.join(map(str, r['notes']))}\n" for r in runs)
lines = subprocess.run([str(exe), "runs"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for r, ln in zip(runs, lines):
    order = [int(x) for x in ln.split()]
    if order != r["order"]:
        bad.append(f"run {r['pat']} {r['id']} tonic {r['tonic']} endOn {r['endOn']} seed {r['seed']}: {order} not {r['order']}")
if len(lines) != len(runs):
    bad.append(f"runs: {len(lines)} answers for {len(runs)} questions")
tempo = want.get("tempo", [])
tl = subprocess.run([str(exe), "tempo"], input="".join(f"{t['bpm']}\n" for t in tempo), capture_output=True, text=True,
                    check=True).stdout.splitlines()
for t, ln in zip(tempo, tl):
    c, ms = map(int, ln.split())
    if (c, ms) != (t["clamp"], t["beat_ms"]):
        bad.append(f"tempo {t['bpm']}: {c} BPM {ms} ms, not {t['clamp']} BPM {t['beat_ms']} ms")
# chords and keys (D-030)
tri = {(t["id"], t["tonic"]): t["chords"] for t in want.get("triads", [])}
cl = subprocess.run([str(exe), "chords"], capture_output=True, text=True, check=True).stdout.splitlines()
seen = 0
for ln in cl:
    parts = ln.split(" | ")
    sid, tonic = parts[0].split()
    w = tri.get((sid, int(tonic)))
    if w is None:
        bad.append(f"triads {sid} {tonic}: not in Sketchpad")
        continue
    seen += 1
    for p, wc in zip(parts[1:], w):
        f = p.split()                       # root [sym] roman, then 9 notes (a major chord's sym is empty)
        root, roman, nums = int(f[0]), f[-10], [int(x) for x in f[-9:]]
        gsym = f[1] if len(f) == 12 else ""
        if root != wc["rootPc"] or gsym != wc["sym"] or roman != wc["roman"] or nums[:3] != wc["notes"] or [nums[0:3], nums[3:6], nums[6:9]] != wc["inv"]:
            bad.append(f"triad {sid} tonic {tonic}: {p.strip()} not {wc}")
if seen != len(tri):
    bad.append(f"triads: {seen} answered of {len(tri)}")
kf = want.get("keyfit", [])
feed = "".join(f"{c['tonic']} {c['minor']} {len(c['chords'])} " + " ".join(f"{x['root']} {len(x['notes'])} " + " ".join(map(str, x["notes"])) for x in c["chords"]) + "\n" for c in kf)
kl = subprocess.run([str(exe), "keyfit"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for c, ln in zip(kf, kl):
    fits, ok, outside, st, sm, sf = map(int, ln.split())
    gout = [i + 1 for i in range(32) if outside >> i & 1]
    gs = None if st < 0 else [st, sm, sf]
    if (fits, ok, gout, gs) != (c["fits"], c["ok"], c["outside"], c["suggest"]):
        bad.append(f"keyFit {c['tonic']} {c['minor']} {[x['root'] for x in c['chords']]}: {(fits, ok, gout, gs)} not {(c['fits'], c['ok'], c['outside'], c['suggest'])}")
if len(kl) != len(kf):
    bad.append(f"keyFit: {len(kl)} answers for {len(kf)}")
ks = want.get("keys", [])
feed = "".join(f"{c['limit']} {len(c['events'])} " + " ".join(f"{a} {b}" for a, b in c["events"]) + "\n" for c in ks)
kk = subprocess.run([str(exe), "keys"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for c, ln in zip(ks, kk):
    f = [int(x) for x in ln.split()]
    amb, heard, k, rest = f[0], f[1], f[2], f[3:]
    cands = [rest[i * 6:(i + 1) * 6] for i in range(k)]
    g = [{"tonic": t, "minor": m, "fit": h / tot, "outside": [p for p in range(12) if o >> p & 1], "evidence": e} for t, m, o, e, h, tot in cands]
    gh = [p for p in range(12) if heard >> p & 1]
    same = amb == c["ambiguous"] and gh == c["heard"] and len(g) == len(c["candidates"]) and all(
        a["tonic"] == b["tonic"] and a["minor"] == b["minor"] and abs(a["fit"] - b["fit"]) < 1e-12 and a["outside"] == b["outside"] and a["evidence"] == b["evidence"]
        for a, b in zip(g, c["candidates"]))
    if not same:
        bad.append(f"keysForNotes {c['events']}: {amb, gh, g[:2]} not {c['ambiguous'], c['heard'], c['candidates'][:2]}")
if len(kk) != len(ks):
    bad.append(f"keysForNotes: {len(kk)} answers for {len(ks)}")
# colour chords (D-033): the chords' notes in five voicings, what Sketchpad hears in them, labels, the next chord
col = {(c["sym"], c["root"], c["base"]): c for c in want.get("colour", [])}
VKEY = ["close", "inv1", "inv2", "open", "wide"]
used, ncol = set(), 0
for ln in subprocess.run([str(exe), "colour"], capture_output=True, text=True, check=True).stdout.splitlines():
    f = ln.split()
    if f[0] == "chord":
        sym, rest = f[1][1:-1], f[2:]          # "chord [sym] root base voicing notes...": a major chord's sym is empty
        root, base, v, notes = int(rest[0]), int(rest[1]), int(rest[2]), [int(x) for x in rest[3:]]
        w = col.get((sym, root, base))
        ncol += 1
        if w is None:
            bad.append(f"colour chord {sym!r}: not in Sketchpad's dictionary")
            continue
        used.add(sym)
        if notes != w[VKEY[v]]:
            bad.append(f"colour chord {sym} root {root} base {base} {VKEY[v]}: {notes} not {w[VKEY[v]]}")
        if v == 0 and w["heard"] is not None and w["heard"] != want["names"][0]["names"][root] + sym and root == 0:
            bad.append(f"colour chord {sym}: Sketchpad hears {w['heard']}")
    elif f[0] == "label":
        t, m, r, lab = int(f[1]), int(f[2]), int(f[3]), f[4]
        w = [x for x in want.get("labels", []) if (x["tonic"], x["minor"], x["root"]) == (t, m, r)]
        ncol += 1
        if not w or w[0]["m9"] != lab:
            bad.append(f"label tonic {t} minor {m} root {r}: {lab} not {w[0]['m9'] if w else None}")
    elif f[0] == "next":
        t, m, nx = int(f[1]), int(f[2]), [int(x) for x in f[3:]]
        w = [x for x in want.get("next", []) if (x["tonic"], x["minor"]) == (t, m)]
        ncol += 1
        if not w or w[0]["next"] != nx:
            bad.append(f"next chord tonic {t} minor {m}: {nx} not {w[0]['next'] if w else None}")
if want.get("colour") and used != {c["sym"] for c in want["colour"]}:
    bad.append(f"colour chords never built: {sorted({c['sym'] for c in want['colour']} - used)}")
idn = want.get("ident", [])
feed = "".join(f"{c['tonic']} {c['minor']} {len(c['notes'])} " + " ".join(map(str, c["notes"])) + "\n" for c in idn)
il = subprocess.run([str(exe), "ident"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for c, ln in zip(idn, il):
    root, lab = ln.split(" ", 1)
    if (int(root), lab[1:-1]) != (c["root"], c["label"]):
        bad.append(f"identifyChord {c['notes']} in {c['tonic']} {c['minor']}: {lab} (root {root}) not [{c['label']}] (root {c['root']})")
if len(il) != len(idn):
    bad.append(f"identifyChord: {len(il)} answers for {len(idn)}")
fits = want.get("fit", [])
feed = "".join(f"{c['tonic']} {sum(1 << p for p in c['pcs'])}\n" for c in fits)
fl = subprocess.run([str(exe), "fit"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for c, ln in zip(fits, fl):
    gfit = [[x.split(":")[0], int(x.split(":")[1])] for x in ln.split()[1:]]
    if gfit != c["top"]:
        bad.append(f"fitScales {c['pcs']} on {c['tonic']}: {gfit} not {c['top']}")
if len(fl) != len(fits):
    bad.append(f"fitScales: {len(fl)} answers for {len(fits)}")
mr = want.get("mrole", [])
mr2 = subprocess.run([str(exe), "mguide"], input="".join(f"{x['pc']} {x['chord']} {x['scale']}\n" for x in mr), capture_output=True, text=True, check=True).stdout.split()
for x, g2 in zip(mr, mr2):
    if int(g2) != x["role"]:
        bad.append(f"melody role {x['pc']} over {x['chord']:03x} in {x['scale']:03x}: {g2}, not {x['role']}")
if len(mr2) != len(mr):
    bad.append(f"melody roles: {len(mr2)} answers for {len(mr)}")
fg = want.get("figures", [])
feed = ""
for x in fg:
    feed += f"{len(x['steps'])} " + " ".join(f"{a} {b} {c2}" for a, b, c2 in x["steps"]) + f" {x['root']} {len(x['iv'])} " + " ".join(map(str, x["iv"])) + f" {x['next']} {x['scale']} {x['seed']} {x['center']}\n"
fg2 = subprocess.run([str(exe), "figs"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for x, ln in zip(fg, fg2):
    v = list(map(int, ln.split()))
    want_n = [q for nt in x["notes"] for q in nt]
    if v[0] != len(x["notes"]) or v[1:] != want_n:
        bad.append(f"figure seed {x['seed']} over {x['root']}: {v} not {[len(x['notes'])] + want_n}")
if len(fg2) != len(fg):
    bad.append(f"figures: {len(fg2)} answers for {len(fg)}")
cls = want.get("closest", [])
feed = "".join(f"{c['tonic']} {c['minor']} {len(c['notes'])} " + " ".join(map(str, c["notes"])) + "\n" for c in cls)
cl2 = subprocess.run([str(exe), "closest"], input=feed, capture_output=True, text=True, check=True).stdout.splitlines()
for c, ln in zip(cls, cl2):
    miss, lab = ln.split(" ", 1)
    if (int(miss), lab[1:-1]) != (c["missing"], c["label"]):
        bad.append(f"closest {c['notes']} in {c['tonic']} {c['minor']}: {lab} ({miss}) not [{c['label']}] ({c['missing']})")
if len(cl2) != len(cls):
    bad.append(f"closest: {len(cl2)} answers for {len(cls)}")
n = ncol + len(idn) + len(fits) + len(cls) + len(mr) + len(fg) + 2 * 24 * 12 + 24 + 2 * len(got["scales"]) + len(runs) + len(tempo) + 7 * len(tri) + len(kf) + len(ks)
for b in bad[:30]:
    print("  FAIL", b)
print(f"{'PASS' if not bad else 'FAIL'}: music oracle, {n} answers compared with Sketchpad, {len(bad)} differ")
sys.exit(1 if bad else 0)
