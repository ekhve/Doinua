# Doinua

*Doinua* is Basque for "melody". Made in Donostia. (The project began as `fm1-scales`; the code and docs still use that
name in places.)

**The FM-1 Music Companion**: firmware that turns the M-VAVE FM-1 into a small studio that teaches while you make
music. You build a short song, a loop of up to eight bars, out of four tracks (chords, bass, melody and drums), and
you can **start with any of them**: a groove on the drums, a melody, or a few chords. The FM-1 works out the key from
what you play, then helps with the next part. On every track you choose how much help you get: play freely, play in
the key (every key sounds right), let the FM-1 show you what to play, or let it play the part from one finger (the
chord pad). What you record is kept exactly as you played it. Three ways to use it: **Producer** (everything open, in
any order), **Guided** (step by step, for beginners and older children) and **Game** (play along with a unicorn or a
dinosaur, for children). The concept: [docs/concepts/v0.5](docs/concepts/v0.5/CONCEPT_v0.5.md); how to use what is
built: [docs/MANUAL.md](docs/MANUAL.md).

It also runs on a PC and in the browser through an emulator. It is built as a *software factory*: agents write the
code, and automated gates decide whether it works.

> Status: **Phase 0, the platform**. The `fm1kit` API (screen, text, lights, keys with velocity, 41 sounds
> from 7 engines, music theory, a store that survives power-off, MIDI in and out) runs on the FM-1 firmware
> and in a PC simulator. A first app, `hello`, plays notes and lights the scale. Every gate passes in the
> emulator. **It has not been installed on a real FM-1 yet.**

## What's here

| Path | What it is |
|---|---|
| `fm1kit/include/fm1kit.h` | **The platform API.** Apps use this header and nothing else ([docs/PLATFORM.md](docs/PLATFORM.md)) |
| `fm1kit/src/` | `device.c` (the FM-1, on felucca-bsp), `host.c` (PC simulator), and the portable `core.c`, `gfx.c`, `synth.c`, `engines.c` (Felucca-Engines host), `music.c`, `store.c`, `midi.c`, `player.c`, `image.c`, `seq.c` |
| `apps/hello/` | First app: plays the keys (and a MIDI keyboard), names the note, lights the scale, remembers its settings. Turn SELECT to change the key, ALGORITHM the scale, PRESETS the sound |
| `apps/companion/` | The product firmware: one song of four tracks, real-time recording, the chord pad and manual chords (SCL switches them on the CHORDS track). How to use it: `docs/MANUAL.md` |
| `factory/` | The gates: `check.sh` runs all of them; `run.py` (scenarios), `safety.py`, `cpu.py`, `usb_midi.py`, `unit/`, `oracle/` (music theory vs Sketchpad), `scenarios/`, `approved/` (screens a person signed off) |
| `docs/`, `features/` | Requirements (`R-nnn`), use cases (`UC-nn`), decisions (`D-nnn`, in `PLATFORM.md`), behaviour scenarios in Gherkin |
| `docs/concepts/` | **The product concepts, versioned**: how it is used, controls, screens; the agreed one becomes use cases and requirements ([index](docs/concepts/README.md)) |
| `tools/` | `setup.sh` (toolchain, SDK files, emulator), `build.py`, font and table generators |
| `third_party/` | Pinned submodules: [felucca-bsp](https://github.com/hugelton/felucca-bsp) (MPL-2.0), [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) (MIT), [fm1-emu](https://github.com/joakimeriksson/fm1-emu) (MIT, with the factory patch in `factory/patches/`) |

## Quick start (Linux x86-64)

```sh
git clone --recurse-submodules https://github.com/ekhve/fm1-scales && cd fm1-scales
tools/setup.sh                 # JieLi toolchain, 3 SDK files, Python packages, the emulator (~2 min)
factory/check.sh               # build + every gate (~1 min)
```

Outputs:
- `build/hello/hello.fwsc`: the package an FM-1 installs.
- `build/hello/app-host`: the PC simulator.
- `work/factory/`: screens, audio and results.

Try the app on the PC:

```sh
mkdir -p work/try && build/hello/app-host --ms 3000 --keys '1000:C4@100,1800:C4^' --out work/try --snap 1500
```

Try the real firmware in the emulator: open https://joakimeriksson.github.io/fm1-emu/ and drop
`build/hello/hello.fwsc` onto it.

## Play it in the browser

No FM-1 needed: open [fm1-emu's page](https://joakimeriksson.github.io/fm1-emu/), click **Open .fwsc…** (or drop the file
on the synth) and pick `build/hello/hello.fwsc`. It is the real firmware in the emulator's WebAssembly build, with sound,
screen, lights and keys (checked on 2026-11-07 with fm1-emu's own `web/smoke.mjs`: it boots, sounds, and a MIDI note
plays). The browser keeps the file for next time. A page of our own, with fm1-scales already loaded, needs the
emulator pin moved to fm1-emu's current version (an owner's decision).

## Installing on a real FM-1 (not done yet: read this first)

Nobody has run this on hardware. Before the first install:

1. Back up your FM-1, and have a way back to stock firmware: [Felucca's web installer](https://hugelton.github.io/Felucca/)
   has "Return to official V15", and there are rescue tools if an install fails.
2. Install with felucca-bsp's installer: `python3 third_party/felucca-bsp/tools/fm1_install.py build/hello/hello.fwsc`
   (needs `pip install mido python-rtmidi`).
3. If it doesn't start, it drops into update mode after two failed boots. Holding **OCT- and OCT+ for 5 s**
   also enters update mode at any time.

## The factory, in one paragraph

A behaviour is written down as a **scenario** (`factory/scenarios/*.json`): key presses with times,
then checks on what comes out (pitch in cents, silence, which key lights are off, dim, on or breathing, and screens).
`factory/run.py` plays each scenario twice: on the **PC simulator**, which takes milliseconds, and on
the **real firmware in the emulator**, at about 8× real time. The two screens must match pixel for pixel.
`factory/safety.py` checks that every build can still be updated over USB. `factory/oracle/` checks the music
theory against [Sketchpad](https://github.com/ekhve/sketchpad)'s tested JavaScript.
People do three things: approve the spec, approve the screens (`run.py --approve`), and try it on the FM-1.

## Licence

GPL-3.0-only (`LICENSE`). `fm1kit/src/device.c` and `fm1kit/app.ld` are derived from felucca-bsp, so
they stay MPL-2.0. The Inter Tight font is SIL OFL 1.1 (`assets/fonts/OFL.txt`). The FMini engine (`fm1kit/src/fmini/`) is fmini-1's (GPL-3.0-only, https://github.com/ruppert/fmini-1), ported (D-086). The Juno-106 engine (`fm1kit/src/juno/`) is FM-106's (GPL-3.0-only, https://github.com/PatttF/fm-106), ported (D-090). Each package carries
three unmodified JieLi SDK files (Apache-2.0, `LICENSES/`). This is an independent project, not made
or endorsed by M-VAVE or Hügelton Instruments.

## Published

This repository starts from one commit: the work up to 2026-11-22, without its earlier history (which held the
owner's private requirements method, removed in D-058). The decisions behind everything are in `docs/PLATFORM.md`
(D-001 onwards); the licences of what it includes are under "Licence" above and in `LICENSES/`.
