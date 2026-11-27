#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: the sound fits the FM-1's time. Every sound, eight voices held, on the cycle-modelled emulator
(FM1_STRICT: each instruction its measured cycles, the flash cache's misses): the interrupts (audio render,
key scan, USB) must take less than BUDGET (80 %, D-091) of the CPU. Over 100 % the audio would glitch on a real FM-1; the
margin is for what an app does in its main loop and for what the model does not count.

  factory/cpu.py [APP]      (after tools/build.py APP; APP's PRESETS encoder must step through the sounds,
                             starting on Basic's second, as hello does)

Two passes: every sound with eight keys held; then four tracks at once (D-029): every sound on track 0 with four
keys held, and over MIDI the bass, two melody notes and a hi-hat (hello: channels 2, 3 and 10), eight voices in
four parts, so each part's per-block work is counted too."""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"
BUDGET = 80                            # the owner, 2026-11-22: "the 75% is our limit. We could raise it ... like 85" (D-091)
app = sys.argv[1] if len(sys.argv) > 1 else "hello"
out = ROOT / "build" / app
sounds = [ln.split("\t") for ln in subprocess.run([str(out / "app-host"), "--list-sounds"], capture_output=True,
                                                    text=True, check=True).stdout.splitlines()]
START = 1                              # hello starts on Basic: Piano (index 1)
CHORD = ",".join(f"{1200 + 10 * i}:{n}@100" for i, n in enumerate(["C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"]))
MIX = ",".join([f"{1200 + 10 * i}:{n}@100" for i, n in enumerate(["C4", "E4", "G4", "B4"])]
               + ["1250:m912D64", "1260:m924C64", "1270:m924F64", "1280:m992A64"])
# twelve voices (D-047): eight keys of the sound on track 0 and, over MIDI, the bass, two melody notes and a hi-hat
TWELVE = CHORD + ",1300:m912D64,1310:m924C64,1320:m924F64,1330:m992A64"
work = ROOT / "work" / "cpu"
(work / "work").mkdir(parents=True, exist_ok=True)


CLICK_MS = 8                           # an encoder click reaches the app every ~7 ms on the FM-1: the keys wait for them all


def load(i, held=CHORD):
    late = abs(i - START) * CLICK_MS    # the keys go down once the sound is chosen, the window as long after them (D-091)
    keys = ",".join(([f"900:e6:{i - START:+d}"] if i != START else [])
                    + [f"{int(t) + late}:{k}" for t, k in (e.split(":", 1) for e in held.split(","))])
    # --usb: a cable in, as when the FM-1 is played from a computer: USB's interrupts are counted, and the firmware does
    # not retry the USB once a second (a 25 ms wait the model skips, which made one window in 25 look busier: D-091);
    # and the computer records the FM-1's USB audio as it plays (D-130), so the audio streaming's work is counted too
    r = subprocess.run([str(EMU), "--usb", "--fwsc", str(out / f"{app}.fwsc"), "--shot", str(2000 + late)], cwd=work,
                       env=dict(os.environ, FM1_STRICT="1", FM1_BENCH_KEYS=keys, FM1_USB_WAV=str(work / f"usb{i}-{len(held)}.raw")),
                       capture_output=True, text=True)   # a computer recording the FM-1 over USB (D-130): the worst case
    (work / f"usb{i}-{len(held)}.raw").unlink(missing_ok=True)   # what the computer recorded: not kept
    m = re.search(r"(\d+)% of instructions in ISRs", r.stdout + r.stderr)
    return int(m.group(1)) if m else None


with ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
    loads = list(pool.map(load, range(len(sounds))))
    mixes = list(pool.map(lambda i: load(i, MIX), range(len(sounds))))
    twelves = list(pool.map(lambda i: load(i, TWELVE), range(len(sounds))))
fails = 0
for what, got in (("8 voices", loads), ("four tracks", mixes), ("twelve voices", twelves)):
    for (e, p, en, pn), pct in zip(sounds, got):
        ok = pct is not None and pct < BUDGET
        fails += not ok
        if not ok or pct >= BUDGET - 15:
            print(f"  {'ok  ' if ok else 'FAIL'} {en}: {pn}: {pct}% interrupt load, {what}")
worst = max((p for p in loads if p is not None), default=None)
i = loads.index(worst) if worst is not None else 0
mworst = max((p for p in mixes if p is not None), default=None)
j = mixes.index(mworst) if mworst is not None else 0
print(f"{'PASS' if not fails else 'FAIL'}: cpu, {len(sounds)} sounds under {BUDGET}%; heaviest {sounds[i][2]}: {sounds[i][3]} at {worst}%;"
      f" four tracks: heaviest with {sounds[j][2]}: {sounds[j][3]} on track 0 at {mworst}%")
sys.exit(1 if fails else 0)
