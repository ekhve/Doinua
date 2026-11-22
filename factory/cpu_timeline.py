#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A tool, not a gate (run it now and then): the CPU over time on the cycle-modelled emulator, from power-on through a
song that adds its tracks one by one. The owner asked for it on 2026-11-22: "start just the fm1, and see the cpu
profiling against the startup sequence. Then once it stabilizes, start many sounds ... a song with all the tracks one
by one adding" (D-091).

  factory/cpu_timeline.py [APP] [--step MS] [--until MS] [--no-usb] [--chords-clicks N]

APP defaults to companion (after tools/build.py APP). Each point is its own run of the emulator (FM1_STRICT) to that
time, reporting the interrupts' share of the last 40 ms, as factory/cpu.py does; the runs are the same up to their
end, so the points make one timeline. Writes work/cpu_timeline/APP.csv and APP.html (the chart, with the song's
phases and the gate's limit). --no-usb: no cable in (the firmware then retries the USB once a second).

The song (120 BPM, over MIDI as factory/cpu.py sends it: channel 1 the chords track, 2 the bass, 3 the melody,
10 the drums): power-on and idle; drums; + bass; + chords (a Cm7 a bar, held); + melody; all together. Before it,
PRESETS chooses the chords track's sound (--chords-clicks, companion: the Sound Browser; 4 = the Juno's E Piano Tre)."""
import argparse
import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"
BUDGET = 80                                           # factory/cpu.py's limit (D-091)
BEAT = 500                                            # 120 BPM
PHASES = [(0, "power-on"), (3000, "drums"), (6000, "+ bass"), (9000, "+ chords (Cm7)"), (12000, "+ melody"),
          (15000, "all together")]


def note(ch, n, at, length, vel=100):
    return [f"{at}:m{0x90 + ch - 1:02X}{n:02X}{vel:02X}", f"{at + length}:m{0x80 + ch - 1:02X}{n:02X}00"]


def song(until, clicks):
    ev = [f"900:e6:+{clicks}"] if clicks else []
    for bar in range(PHASES[1][0], until, 4 * BEAT):
        for b in range(4):
            t = bar + b * BEAT
            ev += note(10, 36 if b in (0, 2) else 38, t, 100)                 # kick, snare
            ev += note(10, 42, t, 80) + note(10, 42, t + BEAT // 2, 80, 70)   # hats in eighths
            if t >= PHASES[2][0]:
                ev += note(2, (36, 36, 43, 34)[b], t, BEAT - 40)              # bass: C C G Bb
            if t >= PHASES[4][0]:
                for k, m in enumerate((72, 75, 79, 77)):
                    ev += note(3, m - (b % 2) * 2 + k % 2, t + k * (BEAT // 4), BEAT // 4 - 20)   # melody
        if bar >= PHASES[3][0]:
            for n in (48, 51, 55, 58):                                        # Cm7: C Eb G Bb
                ev += note(1, n, bar, 4 * BEAT - 60, 90)
    keyed = sorted(ev, key=lambda e: int(e.split(":", 1)[0]))
    return ",".join(keyed)


def load(app, t, keys, usb):
    work = ROOT / "work" / "cpu_timeline" / "run"
    (work / "work").mkdir(parents=True, exist_ok=True)
    cmd = [str(EMU)] + (["--usb"] if usb else []) + ["--fwsc", str(ROOT / "build" / app / f"{app}.fwsc"), "--shot", str(t)]
    r = subprocess.run(cmd, cwd=work, env=dict(os.environ, FM1_STRICT="1", FM1_BENCH_KEYS=keys), capture_output=True, text=True)
    m = re.search(r"(\d+)% of instructions in ISRs", r.stdout + r.stderr)
    return int(m.group(1)) if m else None


def html(app, pts, usb):
    W, H, L, R, T, B = 960, 360, 48, 16, 40, 36
    tmax = pts[-1][0]
    x = lambda t: L + (W - L - R) * t / tmax
    y = lambda p: T + (H - T - B) * (1 - p / 100)
    path = " ".join(f"{'M' if i == 0 else 'L'}{x(t):.1f},{y(p):.1f}" for i, (t, p) in enumerate(pts) if p is not None)
    bands = "".join(f'<line x1="{x(t):.1f}" x2="{x(t):.1f}" y1="{T}" y2="{H - B}" class="ph"/>'
                    f'<text x="{x(t) + 4:.1f}" y="{T - 8}" class="lab">{name}</text>' for t, name in PHASES if t <= tmax)
    grid = "".join(f'<line x1="{L}" x2="{W - R}" y1="{y(v):.1f}" y2="{y(v):.1f}" class="g"/>'
                   f'<text x="{L - 6}" y="{y(v) + 4:.1f}" class="ax" text-anchor="end">{v}%</text>' for v in (0, 25, 50, 75, 100))
    ticks = "".join(f'<text x="{x(t):.1f}" y="{H - B + 18}" class="ax" text-anchor="middle">{t // 1000} s</text>'
                    for t in range(0, tmax + 1, 2000))
    dots = "".join(f'<circle cx="{x(t):.1f}" cy="{y(p):.1f}" r="7" class="hit"><title>{t} ms: {p}%</title></circle>'
                   for t, p in pts if p is not None)
    peak = max((p for _, p in pts if p is not None), default=0)
    return f"""<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CPU timeline</title><style>
:root{{--surface:#fcfcfb;--ink:#0b0b0b;--ink2:#52514e;--grid:#e4e3df;--series:#2a78d6;--limit:#b4372f}}
@media (prefers-color-scheme:dark){{:root{{--surface:#1a1a19;--ink:#f0efec;--ink2:#c3c2b7;--grid:#34332f;--series:#3987e5;--limit:#e5675d}}}}
body{{margin:0;padding:16px;background:var(--surface);color:var(--ink);font:14px system-ui,sans-serif}}
svg{{width:100%;max-width:{W}px;height:auto}} .g{{stroke:var(--grid)}} .ph{{stroke:var(--ink2);stroke-dasharray:2 4}}
.ax,.lab{{fill:var(--ink2);font-size:12px}} .ln{{fill:none;stroke:var(--series);stroke-width:2}}
.lim{{stroke:var(--limit);stroke-width:1.5;stroke-dasharray:6 4}} .limt{{fill:var(--ink2);font-size:12px}}
.hit{{fill:transparent}} .hit:hover{{fill:var(--series);fill-opacity:.25}} p{{color:var(--ink2);max-width:{W}px}}</style></head>
<body><h2>CPU over time: {app}</h2>
<p>The interrupts' share of the CPU (audio, keys, USB, screen transfers) in each 40 ms, on the cycle-modelled emulator,
from power-on through a song that adds its tracks one by one. Peak {peak}%. USB cable {'in' if usb else 'out'}. The dashed red line is
the CPU gate's limit, {BUDGET}%. Hover a point for its time and value.</p>
<svg viewBox="0 0 {W} {H}" role="img" aria-label="CPU load over time, peak {peak} percent">{grid}{bands}{ticks}
<line x1="{L}" x2="{W - R}" y1="{y(BUDGET):.1f}" y2="{y(BUDGET):.1f}" class="lim"/><text x="{W - R}" y="{y(BUDGET) - 6:.1f}" class="limt" text-anchor="end">limit {BUDGET}%</text>
<path d="{path}" class="ln"/>{dots}</svg>
<table><caption>The points</caption><tr><th>ms</th><th>%</th></tr>{''.join(f'<tr><td>{t}</td><td>{p}</td></tr>' for t, p in pts)}</table>
</body></html>"""


def main():
    a = argparse.ArgumentParser()
    a.add_argument("app", nargs="?", default="companion")
    a.add_argument("--step", type=int, default=200)
    a.add_argument("--until", type=int, default=17000)
    a.add_argument("--no-usb", action="store_true")
    a.add_argument("--chords-clicks", type=int, default=4)
    o = a.parse_args()
    keys = song(o.until, o.chords_clicks)
    times = list(range(o.step, o.until + 1, o.step))
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
        pcts = list(pool.map(lambda t: load(o.app, t, keys, not o.no_usb), times))
    pts = list(zip(times, pcts))
    out = ROOT / "work" / "cpu_timeline"
    (out / f"{o.app}.csv").write_text("ms,isr_pct\n" + "".join(f"{t},{p}\n" for t, p in pts))
    (out / f"{o.app}.html").write_text(html(o.app, pts, not o.no_usb))
    for (t0, name), (t1, _) in zip(PHASES, PHASES[1:] + [(o.until + 1, "")]):
        seg = [p for t, p in pts if t0 < t <= t1 and p is not None]
        if seg:
            print(f"  {name:16} {t0 / 1000:5.1f}..{min(t1, o.until) / 1000:5.1f} s: {min(seg)}..{max(seg)} %, mean {sum(seg) / len(seg):.0f} %")
    print(f"cpu timeline: {out / (o.app + '.html')}")


if __name__ == "__main__":
    main()
