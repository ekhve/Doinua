#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate (build): the user kit (ST-35, D-056, R-137, R-138). A test kit, 01-kick1.wav and 05-snare1.wav (made here: a
falling sine and a noise burst):
  - without a LICENSE file the build stops and says why;
  - with one, the generated header puts kick1 on key 01 (F3) and snare1 on 05 (A3), named so;
  - the companion built with it (host) plays the file itself on F3 once PRESETS reaches USER (correlation >= 0.95).
Runs before the apps' own build in factory/check.sh (it builds the companion's host with the test kit; the build
after it is the real one)."""
import math
import os
import shutil
import struct
import subprocess
import sys
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
work = ROOT / "work" / "userkit"
kit = work / "kit"
fails = []


def wav(path, xs, rate=44100):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack("<%dh" % len(xs), *[int(max(-1, min(1, x)) * 30000) for x in xs]))


shutil.rmtree(work, ignore_errors=True)
kit.mkdir(parents=True)
ph, xs = 0.0, []
for i in range(int(0.3 * 44100)):                      # a kick: 120 Hz falling to 50, 300 ms
    f = 50 + 70 * math.exp(-i / 2000)
    ph += 2 * math.pi * f / 44100
    xs.append(math.sin(ph) * math.exp(-i / 6000))
wav(kit / "01-kick1.wav", xs)
seed = 1
ys = []
for i in range(int(0.2 * 22050)):                      # a snare: noise, 200 ms, at 22 050 Hz
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
    ys.append((seed / 0x3FFFFFFF - 1) * math.exp(-i / 1500))
wav(kit / "05-snare1.wav", ys, 22050)

env = dict(os.environ, FM1_USER_KIT=str(kit))
r = subprocess.run([sys.executable, ROOT / "tools" / "build.py", "companion", "--host-only"], env=env, capture_output=True, text=True)
said = r.stdout + r.stderr
if r.returncode == 0 or "no LICENSE file" not in said:
    fails.append("without a LICENSE file the build went on, or did not say why")
(kit / "LICENSE").write_text("Test sounds made by factory/userkit.py: public domain (CC0).\n")
r = subprocess.run([sys.executable, ROOT / "tools" / "build.py", "companion", "--host-only"], env=env, capture_output=True, text=True)
if r.returncode:
    fails.append("with a LICENSE file the build failed: " + (r.stdout + r.stderr)[-400:])
else:
    h = (ROOT / "build" / "companion" / "gen" / "app_assets.h").read_text()
    if '{ 0, "kick1", USER_KIT_0,' not in h or '{ 4, "snare1", USER_KIT_4,' not in h:
        fails.append("the header does not put kick1 on 01 (F3) and snare1 on 05 (A3)")
    out = work / "run"
    out.mkdir()
    # DRUMS, PRESETS to USER (the fifth kit), F3
    subprocess.run([ROOT / "build" / "companion" / "app-host", "--ms", "2000", "--keys",
                    "900:e1:+3,1000:e6:+1,1020:e6:+1,1040:e6:+1,1060:e6:+1,1300:F3@127,1400:F3^", "--out", out], check=True, capture_output=True)
    with wave.open(str(out / "audio.wav")) as w:
        raw = w.readframes(w.getnframes())
        ch = w.getnchannels()
    a = struct.unpack("<%dh" % (len(raw) // 2), raw)[::ch]
    seg = a[int(1.29 * 44100):int(1.7 * 44100)]
    peak = max(abs(v) for v in seg) if seg else 0
    on = next((i for i, v in enumerate(seg) if abs(v) > 50), None)
    if peak < 1000 or on is None:
        fails.append(f"F3 on the USER kit is silent (peak {peak})")
    else:                                               # the file's own wave: correlated with what it plays
        y = seg[max(0, on - 2):max(0, on - 2) + int(0.25 * 44100)]
        x = xs[:len(y)]
        best = 0.0
        for lag in range(0, 12):                        # where it starts, to a few samples
            xx, yy = x[:len(y) - lag], y[lag:]
            num = sum(p * q for p, q in zip(xx, yy))
            den = math.sqrt(sum(p * p for p in xx) * sum(q * q for q in yy)) or 1
            best = max(best, num / den)
        if best < 0.95:
            fails.append(f"F3 on the USER kit does not play the file (correlation {best:.2f})")
for f in fails:
    print("  FAIL", f)
print(f"{'PASS' if not fails else 'FAIL'}: user kit, {3 - len(fails) if len(fails) <= 3 else 0} of 3")
sys.exit(1 if fails else 0)
