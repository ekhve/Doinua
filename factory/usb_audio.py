#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: USB audio out (R-400, D-130): the firmware in the emulator with a USB cable to a "computer" that records the
FM-1's audio streaming interface as a recording program does (fm1-emu FM1_USB_WAV: SET_INTERFACE, SET_CUR 44.1 kHz,
one isochronous IN transaction a millisecond).

  factory/usb_audio.py [APP]

  the stream: stereo, 16-bit, 44.1 kHz; silent before a key; while A4 is held it sounds, at 440 Hz (within 20 cents),
  the same on both sides; few packets found empty once it flows (under 2 %)
(The computer's audio into the FM-1, R-401..R-403, is not modelled by the emulator: checked on the FM-1.)"""
import os
import re
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"
app = sys.argv[1] if len(sys.argv) > 1 else "hello"
work = ROOT / "work" / "usb_audio"
(work / "work").mkdir(parents=True, exist_ok=True)
fails = []


def gate(ok, what):
    print(f"  {'ok  ' if ok else 'FAIL'} {what}")
    if not ok:
        fails.append(what)


raw = work / "usb.raw"
if raw.exists():
    raw.unlink()
env = dict(os.environ, FM1_USB_WAV=str(raw), FM1_NO_PLAY="1", FM1_BENCH_KEYS="1500:A4@100,2600:A4^")
r = subprocess.run([str(EMU), "--fwsc", str(ROOT / "build" / app / f"{app}.fwsc"), "--usb", "--shot", "3000"],
                   env=env, capture_output=True, text=True, cwd=work)
log = r.stdout + r.stderr
m = re.search(r"usb audio: (\d+) ch at (\d+) Hz, (\d+) samples in (\d+) packets \((\d+) empty, (\d+) of them underruns\)", log)
gate(m is not None and m.group(1) == "2" and m.group(2) == "44100", "the FM-1 offers USB audio: stereo, 16-bit, 44.1 kHz"
     + (f" ({m.group(0)})" if m else f" (emulator said: {log.strip().splitlines()[-1] if log.strip() else 'nothing'})"))
if m:
    a = np.frombuffer(raw.read_bytes(), dtype="<i2").astype(np.float64).reshape(-1, 2)
    fs = 44100
    # the stream starts when the computer opens it: find the note by its level, then look a little after its start
    mono = a.mean(axis=1)[: len(a) // 441 * 441]
    rms = np.sqrt((mono.reshape(-1, 441) ** 2).mean(axis=1)) if len(mono) else np.zeros(1)
    loud = np.nonzero(rms > 300)[0]
    gate(len(rms) > 20 and rms[: max(1, (loud[0] if len(loud) else len(rms)) - 2)].max() < 30,
         "silent before the key (the first 10 ms blocks under rms 30)")
    gate(len(loud) >= 60, f"while A4 is held it sounds ({len(loud)} blocks of 10 ms over rms 300)")
    if len(loud) >= 60:
        s = a[(loud[0] + 20) * 441:(loud[0] + 60) * 441]
        x = s[:, 0] * np.hanning(len(s))
        spec = np.abs(np.fft.rfft(x, 8 * len(x)))
        f = np.fft.rfftfreq(8 * len(x), 1 / fs)
        band = (f > 200) & (f < 2000)
        peak = f[band][np.argmax(spec[band])]
        cents = 1200 * np.log2(peak / 440.0)
        gate(abs(cents) < 20, f"at 440 Hz: the strongest partial at {peak:.1f} Hz ({cents:+.1f} cents)")
        gate(np.abs(s[:, 0] - s[:, 1]).max() < 2, "left and right the same (the FM-1 is mono, sent as stereo)")
    packets, empty, under = int(m.group(4)), int(m.group(5)), int(m.group(6))
    gate(packets > 800 and under < packets * 0.02, f"packets flow: {packets}, {under} underruns (under 2 %)")
print(f"{'FAIL' if fails else 'PASS'}: usb audio, {len(fails)} failed")
sys.exit(1 if fails else 0)
