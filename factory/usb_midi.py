#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: MIDI over USB, both ways, with the firmware in the emulator and a USB cable to a "computer".

  factory/usb_midi.py [APP]     (APP sends its keys out and plays and shows notes that come in, as hello does)

  out: a key pressed at velocity 100 goes out as note-on 90 3C 64 within 20 ms, its release as 80 3C 00
  in:  a note-on sent from the computer reaches the app: the screen changes (it doesn't without it)
  clock (companion, D-044): PLAY sends Start, then 24 clocks a beat at the song's tempo, and stopping sends Stop
(The TRS jack's MIDI in is checked by the kit_midi_in scenario, with sound, on both targets.)"""
import os
import re
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"
app = sys.argv[1] if len(sys.argv) > 1 else "hello"
work = ROOT / "work" / "usb_midi"
(work / "work").mkdir(parents=True, exist_ok=True)
fails = []


def gate(ok, what):
    print(f"  {'ok  ' if ok else 'FAIL'} {what}")
    if not ok:
        fails.append(what)


def shot(ms, send="", keys=""):
    # FM1_NO_PLAY: without a key script the emulator's panel picture presses PLAY at 1.2 s, and PLAY starts hello's
    # playback (D-025), which would change the screen with or without the note from the computer
    env = dict(os.environ, FM1_USB_SEND=send, FM1_NO_PLAY="1")
    if keys:
        env["FM1_BENCH_KEYS"] = keys
    r = subprocess.run([str(EMU), "--fwsc", str(ROOT / "build" / app / f"{app}.fwsc"), "--usb", "--shot", str(ms)],
                       env=env, capture_output=True, text=True, cwd=work)
    lcd = np.asarray(Image.open(work / "work" / "panel.ppm").convert("RGB"), dtype=np.int16)[40:520, 370:850]
    return r.stdout + r.stderr, lcd


log, _ = shot(2500, keys="1500:C4@100,1800:C4^")
on = re.search(r"\[\s*(\d+) ms\] usb in:\s+90 3c 64", log)
off = re.search(r"\[\s*(\d+) ms\] usb in:\s+80 3c 00", log)
gate(on is not None and 1500 <= int(on.group(1)) <= 1520, "a key at velocity 100 goes out as note-on 90 3C 64 within 20 ms")
gate(off is not None and 1800 <= int(off.group(1)) <= 1820, "its release goes out as note-off 80 3C 00")
_, quiet = shot(1900)
_, played = shot(1900, send="1500:903C64")
diff = float((np.abs(played - quiet).max(axis=2) > 8).mean())
gate(diff > 0.02, f"a note-on from the computer reaches the app (screen {diff * 100:.1f}% changed)")
_, again = shot(1900)
gate(float((np.abs(again - quiet).max(axis=2) > 8).mean()) == 0.0, "without it, the screen is the same (the check above is the note's doing)")
if app == "companion":         # MIDI clock out (D-044): PLAY sends Start, a clock each tick (24 a beat), stopping sends Stop
    log, _ = shot(2000, keys="1000:PLAY,1050:PLAY^,1700:PLAY,1750:PLAY^")
    rt = [(int(m.group(1)), m.group(2)) for m in re.finditer(r"\[\s*(\d+) ms\] usb in:\s+(f[8abc])\s*$", log, re.M)]
    starts = [t for t, b in rt if b == "fa"]
    stops = [t for t, b in rt if b == "fc"]
    clocks = [t for t, b in rt if b == "f8" and 1000 <= t <= 1700]
    gate(len(starts) == 1 and 1000 <= starts[0] <= 1020, f"PLAY sends Start (FA) within 20 ms ({starts})")
    gate(len(stops) == 1 and 1700 <= stops[0] <= 1720, f"stopping sends Stop (FC) within 20 ms ({stops})")
    per = (clocks[-1] - clocks[0]) / (len(clocks) - 1) if len(clocks) > 1 else 0
    want = 60000 / 90 / 24
    gate(abs(per - want) <= want * 0.02 and len(clocks) >= 24, f"clocks (F8) at 24 a beat of 90 BPM: {len(clocks)}, {per:.2f} ms apart, want {want:.2f}")
print(f"{'PASS' if not fails else 'FAIL'}: usb midi, {len(fails)} failed")
sys.exit(1 if fails else 0)
