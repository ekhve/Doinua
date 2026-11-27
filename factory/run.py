#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The factory's gate runner: play scenarios on the host simulator and on the real firmware in the
emulator, and check what came out.

  factory/run.py                      every scenario in factory/scenarios/, both targets
  factory/run.py SCENARIO.json ...    just these
  --target host|emu|both              (default both)
  --no-build                          use what is in build/
  --approve                           write the host screens as the approved references (a person's call)

A scenario (JSON):
  { "name": "...", "app": "hello", "ms": 3000,
    "keys": "1000:C4@100,1800:C4^",        the emulator's script syntax (fm1-live FM1_BENCH_KEYS);
                                           times from boot; keep the first one >= 800 ms (the
                                           firmware is up by then)
    "checks": [
      {"type": "silence", "from": 600, "to": 950},
      {"type": "sound", "from": 1100, "to": 1700},            rms >= 300 (default): something plays
      {"type": "level_ratio", "a": [1400, 1700], "b": [2400, 2700], "ratio": 3.97, "tol": 0.1}   rms(b) / rms(a)
      {"type": "peak", "from": 1100, "to": 2500, "max_dbfs": -6.0}   the output's peak, dBFS of full 16-bit scale
      {"type": "pitch", "from": 1100, "to": 1700, "note": "C4", "cents": 20},   "offset": +50 wants it 50 cents above
                                           the note (a tuning), within the same tolerance
      {"type": "lights", "from": 1100, "to": 1700, "keys": {"C4": "on", "D4": "dim", "C#4": "off", "C5": "breath"}},
                                           levels: off, dim, half (API 28), on, breath
      {"type": "screen", "at": 1500}       the device frame equals the host frame, and the host frame
                                           equals the approved one in factory/approved/ (if any); with
                                           "dark": true, also that the screen is black (under 1% of pixels lit).
                                           "volatile_rect": [x, y, w, h] excludes a variable label from
                                           the approved reference only. The full host and device frames still match.
    ],
    "reboot": { "ms": ..., "keys": "...", "checks": [...] }   optional: a second run that starts with the store
                                           (host --store, emulator --flash) the first one left }
Exit status 0 only when every check on every target passed. Results: work/factory/<scenario>/."""
import argparse
import json
import math
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
FS = 44100
EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"
NOTE_BASE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
LEVELS = ["off", "dim", "on", "breath", "half"]
FRAME_MS = 20


def midi_of(name):
    n = name.strip()
    base = NOTE_BASE[n[0].upper()]
    rest = n[1:]
    acc = 0
    if rest.startswith("#"):
        acc, rest = 1, rest[1:]
    elif rest.startswith("b") and len(rest) > 1:
        acc, rest = -1, rest[1:]
    return (int(rest) + 1) * 12 + base + acc


def key_of(name):              # piano key 0..26 (0 = F3)
    k = midi_of(name) - 53
    if not 0 <= k < 27:
        raise SystemExit(f"{name}: not on the FM-1's keys (F3..G5)")
    return k


# ---- running ----

def run_host(sc, out, store=None):
    out.mkdir(parents=True, exist_ok=True)
    snaps = sorted({c["at"] for c in sc["checks"] if c["type"] == "screen"})
    lights = [c for c in sc["checks"] if c["type"] == "lights"]
    for c in lights:            # host lights are exact: one sample in the window is enough
        snaps.append((c["from"] + c["to"]) // 2)
    cmd = [ROOT / "build" / sc["app"] / "app-host", "--ms", str(sc["ms"]), "--keys", sc["keys"], "--out", out]
    if snaps:
        cmd += ["--snap", ",".join(str(s) for s in sorted(set(snaps)))]
    if store:
        cmd += ["--store", store]
    subprocess.run([str(c) for c in cmd], check=True)
    rate, audio = read_wav(out / "audio.wav")
    return {"audio": audio,
            "lcd": lambda ms: np.asarray(Image.open(out / f"lcd_{ms}.ppm").convert("RGB"), dtype=np.int16),
            "lights": lambda a, b: host_lights(out, (a + b) // 2)}


def host_lights(out, ms):
    d = json.loads((out / f"leds_{ms}.json").read_text())
    return [LEVELS[v] for v in d["keys"]]


def run_emu(sc, out, store=None):
    if not EMU.exists():
        raise SystemExit(f"no emulator at {EMU} (tools/setup.sh builds it)")
    out.mkdir(parents=True, exist_ok=True)
    for f in out.glob("*"):
        f.unlink()
    env = dict(os.environ, FM1_FACTORY="1", FM1_BENCH_KEYS=sc["keys"], FM1_BENCH_WAV=str(out / "bench.raw"),
               FM1_BENCH_FRAMES=f"{out},{FRAME_MS}")
    cmd = [str(EMU), "--fwsc", str(ROOT / "build" / sc["app"] / f"{sc['app']}.fwsc"), "--bench", str(sc["ms"])]
    if store:                   # the flash chip from this file and back to it: what was saved survives the reboot
        cmd += ["--flash", str(store)]
    r = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit("emulator failed")
    raw = np.frombuffer((out / "audio.raw").read_bytes(), dtype="<i2").reshape(-1, 2)[:, 0].astype(np.float64)
    leds = [json.loads(ln) for ln in (out / "leds.jsonl").read_text().splitlines()]

    def lcd(ms):
        t = min((int(p.stem.split("_")[1]) for p in out.glob("lcd_*.bmp") if int(p.stem.split("_")[1]) >= ms), default=None)
        if t is None:
            raise SystemExit(f"emulator: no frame at or after {ms} ms")
        return np.asarray(Image.open(out / f"lcd_{t}.bmp").convert("RGB"), dtype=np.int16)

    def lights(a, b):
        rows = [r["leds"][14:41] for r in leds if a <= r["ms"] <= b]
        if not rows:
            raise SystemExit(f"emulator: no LED data between {a} and {b} ms")
        m = np.array(rows)
        res = []
        for k in range(27):
            col = m[:, k]
            if col.max() < 0.001:
                res.append("off")
            elif col.max() - col.min() > 0.02:
                res.append("breath")
            elif col.mean() > 0.05:
                res.append("on")
            elif col.mean() > 0.008:     # KIT_LIGHT_HALF (API 28, D-075): ~15 % of on (~0.0135), dim ~0.003
                res.append("half")
            else:
                res.append("dim")
        return res

    return {"audio": raw, "lcd": lcd, "lights": lights}


def read_wav(p):
    b = p.read_bytes()
    assert b[:4] == b"RIFF" and b[8:12] == b"WAVE"
    rate = int.from_bytes(b[24:28], "little")
    i = b.index(b"data") + 8
    return rate, np.frombuffer(b[i:], dtype="<i2").astype(np.float64)


# ---- checks ----

def window(audio, a, b):
    return audio[int(a * FS / 1000):int(b * FS / 1000)]


def f0(x):
    x = x - x.mean()
    n = min(len(x), 8192)
    x = x[:n]
    c = np.correlate(x, x, "full")[n - 1:]
    lo, hi = int(FS / 2000), int(FS / 50)
    lag = lo + int(np.argmax(c[lo:hi]))
    if 1 <= lag < len(c) - 1:            # parabolic peak: sub-sample lag
        y0, y1, y2 = c[lag - 1], c[lag], c[lag + 1]
        d = (y0 - y2) / (2 * (y0 - 2 * y1 + y2)) if (y0 - 2 * y1 + y2) else 0
        return FS / (lag + d)
    return FS / lag


def check(c, res, target, sc, out):
    t = c["type"]
    if t == "silence":
        x = window(res["audio"], c["from"], c["to"])
        rms = float(np.sqrt((x ** 2).mean())) if len(x) else 0.0
        return rms < c.get("rms", 30), f"rms {rms:.0f}"
    if t == "sound":
        x = window(res["audio"], c["from"], c["to"])
        rms = float(np.sqrt((x ** 2).mean())) if len(x) else 0.0
        return rms >= c.get("rms", 300), f"rms {rms:.0f}"
    if t == "peak":
        x = window(res["audio"], c["from"], c["to"])
        peak = float(np.abs(x).max()) if len(x) else 0.0
        dbfs = 20 * math.log10(peak / 32768) if peak > 0 else -120.0
        return dbfs <= c["max_dbfs"], f"peak {peak:.0f} = {dbfs:.1f} dBFS, ceiling {c['max_dbfs']} dBFS"
    if t == "level_ratio":
        xa = window(res["audio"], *c["a"])
        xb = window(res["audio"], *c["b"])
        ra, rb = (float(np.sqrt((x ** 2).mean())) if len(x) else 0.0 for x in (xa, xb))
        if ra < 30:
            return False, f"window a is silent (rms {ra:.0f})"
        got = rb / ra
        return abs(got / c["ratio"] - 1) <= c.get("tol", 0.1), f"rms {rb:.0f} / {ra:.0f} = {got:.2f}, want {c['ratio']:.2f}"
    if t == "pitch":
        x = window(res["audio"], c["from"], c["to"])
        rms = float(np.sqrt((x ** 2).mean())) if len(x) else 0.0
        if rms < 100:
            return False, f"no sound (rms {rms:.0f})"
        want = 440 * 2 ** ((midi_of(c["note"]) - 69) / 12)
        got = f0(x)
        cents = 1200 * math.log2(got / want)
        off = c.get("offset", 0)
        what = f" (want {off:+d})" if off else ""
        return abs(cents - off) <= c.get("cents", 20), f"{got:.1f} Hz for {c['note']} {want:.1f} Hz ({cents:+.0f} cents{what})"
    if t == "lights":
        if "breath" in c["keys"].values() and c["to"] - c["from"] < 1200:
            raise SystemExit(f"{sc['name']}: a 'breath' check needs a window of at least 1200 ms (one breath is 1.13 s)")
        got = res["lights"](c["from"], c["to"])
        bad = [f"{n}: {got[key_of(n)]} not {want}" for n, want in c["keys"].items() if got[key_of(n)] != want]
        return not bad, "; ".join(bad) if bad else f"{len(c['keys'])} keys as expected"
    if t == "screen":
        return None, "compared across targets"
    raise SystemExit(f"unknown check type {t}")


def screen_checks(sc, results, out, approve):
    lines, ok_all = [], True
    for c in (c for c in sc["checks"] if c["type"] == "screen"):
        at = c["at"]
        host = results["host"]["lcd"](at) if "host" in results else None
        if host is not None:
            Image.fromarray(host.astype(np.uint8)).save(out / f"screen_{at}.png")
            ref = ROOT / "factory" / "approved" / f"{Path(sc['file']).stem}{sc.get('suffix', '')}_{at}.png"
            if approve:
                ref.parent.mkdir(parents=True, exist_ok=True)
                Image.fromarray(host.astype(np.uint8)).save(ref)
                lines.append(("ok", f"screen {at}: approved -> {ref.relative_to(ROOT)}"))
            elif ref.exists():
                want = np.asarray(Image.open(ref).convert("RGB"), dtype=np.int16)
                different = np.abs(want - host).max(axis=2) > 8
                if "volatile_rect" in c:
                    x, y, w, h = c["volatile_rect"]
                    mask = np.ones(different.shape, dtype=bool)
                    mask[y:y + h, x:x + w] = False
                    different = different[mask]
                diff = float(different.mean())
                ok = diff <= 0.001
                ok_all &= ok
                lines.append(("ok" if ok else "FAIL", f"screen {at}: host vs approved, {diff * 100:.2f}% pixels differ"))
            else:
                lines.append(("new", f"screen {at}: no approved reference yet (review {out / f'screen_{at}.png'}, then --approve)"))
        if c.get("dark"):                       # the screen is dark: on every target
            for tg in results:
                frame = results[tg]["lcd"](at)
                lit = float((frame.max(axis=2) > 8).mean())
                ok = lit < 0.01
                ok_all &= ok
                lines.append(("ok" if ok else "FAIL", f"[{tg}] screen {at} dark: {lit * 100:.2f}% pixels lit"))
        if host is not None and "emu" in results:
            dev = results["emu"]["lcd"](at)
            Image.fromarray(dev.astype(np.uint8)).save(out / f"screen_{at}_device.png")
            diff = float((np.abs(dev - host).max(axis=2) > 8).mean())
            ok = diff <= 0.005
            ok_all &= ok
            lines.append(("ok" if ok else "FAIL", f"screen {at}: device vs host, {diff * 100:.2f}% pixels differ"))
    return ok_all, lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scenarios", nargs="*")
    ap.add_argument("--target", default="both", choices=["host", "emu", "both"])
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--approve", action="store_true")
    a = ap.parse_args()
    files = [Path(s) for s in a.scenarios] or sorted((ROOT / "factory" / "scenarios").glob("*.json"))
    targets = ["host", "emu"] if a.target == "both" else [a.target]
    scenarios = []
    for f in files:
        sc = json.loads(f.read_text())
        sc["file"] = str(f)
        scenarios.append(sc)
    if not a.no_build:
        for app in sorted({sc["app"] for sc in scenarios}):
            cmd = [sys.executable, str(ROOT / "tools" / "build.py"), app] + (["--host-only"] if targets == ["host"] else [])
            subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    failed = 0
    summary = []
    for sc in scenarios:
        out = ROOT / "work" / "factory" / Path(sc["file"]).stem
        out.mkdir(parents=True, exist_ok=True)
        print(f"\n{sc['name']}  ({Path(sc['file']).name})")
        phases = [sc]
        if "reboot" in sc:      # a second run that starts with the store (the flash chip) the first left
            phases.append(dict(sc["reboot"], app=sc["app"], file=sc["file"], name=sc["name"], suffix="_reboot"))
        stores = {t: (out / f"store_{t}.bin" if len(phases) > 1 else None) for t in targets}
        for f in stores.values():
            if f is not None and f.exists():
                f.unlink()
        for k, ph in enumerate(phases):
            pout = out if k == 0 else out / "reboot"
            pout.mkdir(parents=True, exist_ok=True)
            if k:
                print("  -- after a reboot")
            results = {}
            for t in targets:
                results[t] = (run_host if t == "host" else run_emu)(ph, pout / t, stores[t])
            for t in targets:
                for c in ph["checks"]:
                    ok, msg = check(c, results[t], t, ph, pout)
                    if ok is None:
                        continue
                    print(f"  {'ok  ' if ok else 'FAIL'} [{t}] {c['type']} {c.get('from', c.get('at', ''))}-{c.get('to', '')}: {msg}")
                    failed += not ok
                    summary.append({"scenario": sc["name"], "phase": k, "target": t, "check": c, "ok": ok, "detail": msg})
            ok, lines = screen_checks(ph, results, pout, a.approve)
            for status, msg in lines:
                print(f"  {status:4} {msg}")
            failed += sum(1 for s_, _ in lines if s_ == "FAIL")
    (ROOT / "work" / "factory").mkdir(parents=True, exist_ok=True)
    (ROOT / "work" / "factory" / "summary.json").write_text(json.dumps(summary, indent=1))
    print(f"\n{'PASS' if not failed else 'FAIL'}: {len(summary)} checks, {failed} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
