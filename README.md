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
train, for children). The concept: [docs/concepts/v0.5](docs/concepts/v0.5/CONCEPT_v0.5.md); how to use what is
built: [docs/MANUAL.md](docs/MANUAL.md).

It also runs on a PC and in the browser through an emulator. It is built as a *software factory*: agents write the
code, and automated gates decide whether it works.

> **Status (2026-11-27): the first build to flash is being finished.** Everything below runs in the PC simulator and
> on the real firmware in the emulator, and passes every gate (816 checks). **It has not been installed on a real
> FM-1 yet**: the owner's first hardware test comes once the sampler and MOTION are in (D-126). Then a first release.

## What it does today

| Part | What you get |
|---|---|
| **Four tracks** | CHORDS, BASS, MELODY, DRUMS, each with three pages: **PLAY** (you play, with as much help as you like: free, in the key, shown, or from one finger), **IDEAS** (it suggests a part to learn) and **AUTO** (hold a pad, a whole part plays). The key is worked out from what you play |
| **Recording** | Live, as played, on every track; the REC layer (GRID, FEEL with swing and MPC feels, CLICK, COUNT-IN, clear); erase with EDIT + a key; undo with SAVE held |
| **Sequencer** | **SONG** (sequences arranged on the keys), **SEQUENCE** (record with quantize and swing), **STEPS** (write notes and hits on a grid); the song is kept over power-off |
| **Sounds** | 178 sounds from seven engines: Basic, FMini (fmini-1), Juno-106 (FM-106), Bass Something 2, OberVave, CoralFlower VA, and four drum kits (Dusty, 808, Live, Latin) plus your own samples. The **Sound Browser** picks any of them |
| **Sound Edit and FX** | EDIT: OSC, FILTER (with a live spectrum), F.ENV, A.ENV, VOICE (poly, mono, legato, glide), LFO, MOD. FX: PERFORM (X/Y pairs), DELAY, REVERB, DIRT |
| **Drums** | Drum map v2 (the 12 sounds AUTO plays on F3–E4), each key's level, reverb and shape (attack, hold, decay, mute groups), the kit's sounds mixed into your own kit; the beat drawn as you recorded it |
| **ARP** | On every track |
| **Guided mode** | Lessons for beginners (lesson 1 built; more coming, and later lessons that teach the firmware itself) |
| **Games** | Unicorn Jump, Hiru Damatxo, Aldapeko Sagarra, the Night Train: play along, for children |
| **MIDI** | In and out over USB: notes, clock and controls; a MIDI keyboard plays the tracks |

**Coming in the first build** (D-121, D-126): the sampler inside DRUMS (sample from a phone over USB, TAKE and CHOP,
MY KIT 1–3), USB audio both ways (record the FM-1 on a phone or MPC), the kit's effects (PUNCH, COLOR, SPACE), ENV
(the notes and chords of a sample), note, key and tempo found in a sample, and MOTION (knob moves recorded into the
song). **Later**: the visualizer (the Kursaal at night, `docs/concepts/kursaal/`), more engines.

How to use all of it: [docs/MANUAL.md](docs/MANUAL.md). What is being built now, by whom, and what needs the owner:
the **Factory status** issue (#1).

## What's here

| Path | What it is |
|---|---|
| `fm1kit/include/fm1kit.h` | **The platform API** (screen, text, lights, keys with velocity, the sound engines and drum kits, FX, music theory, the sequencer, a store that survives power-off, MIDI). Apps use this header and nothing else ([docs/PLATFORM.md](docs/PLATFORM.md), also the decision log D-001 onwards) |
| `fm1kit/src/` | `device.c` (the FM-1, on felucca-bsp), `host.c` (PC simulator), and the portable rest: `synth.c`, `drums.c`, `engines.c`, the ported engines (`fmini/`, `juno/`, `bs2/`, `obervave/`, `coralflower/`), `music.c`, `seq.c`, `store.c`, `midi.c`, `gfx.c`, `image.c` |
| `apps/companion/` | **The product firmware**: the four tracks, recording, the sequencer, Sound Edit and FX, the drums, the guided mode and the games |
| `apps/hello/` | The first test app: plays the keys, names the note, lights the scale |
| `factory/` | The gates: `check.sh` runs all of them; `run.py` (scenarios), `safety.py`, `cpu.py`, `usb_midi.py`, `unit/`, `oracle/` (music theory vs Sketchpad), `scenarios/`, `approved/` (screens the owner signed off) |
| `docs/` | The manual; requirements (`R-nnn`), stories (`ST-nn`), use cases (`UC-nn`), hazards, architecture; `labs/` (the browser labs each screen is built from, D-084); `concepts/` (the product concepts) |
| `features/` | Behaviour scenarios in Gherkin |
| `tools/` | `setup.sh` (toolchain, SDK files, emulator), `build.py`, generators (fonts, tables, images, engines) |
| `third_party/` | Pinned submodules: [felucca-bsp](https://github.com/hugelton/felucca-bsp) (MPL-2.0), [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) (MIT), [fm1-emu](https://github.com/joakimeriksson/fm1-emu) (MIT, with the factory patch in `factory/patches/`) |

## Quick start (Linux x86-64)

```sh
git clone --recurse-submodules https://github.com/ekhve/Doinua && cd Doinua
tools/setup.sh                 # JieLi toolchain, 3 SDK files, Python packages, the emulator (~2 min)
factory/check.sh               # build + every gate (about 45 min: 816 checks on the PC and in the emulator)
```

Outputs:
- `build/companion/companion.fwsc`: the package an FM-1 installs (the product).
- `build/companion/app-host`: the PC simulator.
- `work/factory/`: screens, audio and results.

Try the app on the PC:

```sh
mkdir -p work/try && build/companion/app-host --ms 3000 --keys '1000:C4@100,1800:C4^' --out work/try --snap 1500
```

Try the real firmware in the emulator: open https://joakimeriksson.github.io/fm1-emu/ and drop
`build/companion/companion.fwsc` onto it.

## Play it in the browser

No FM-1 needed: open [fm1-emu's page](https://joakimeriksson.github.io/fm1-emu/), click **Open .fwsc…** (or drop the file
on the synth) and pick `build/companion/companion.fwsc`. It is the real firmware in the emulator's WebAssembly build, with sound,
screen, lights and keys (checked on 2026-11-07 with fm1-emu's own `web/smoke.mjs`: it boots, sounds, and a MIDI note
plays). The browser keeps the file for next time. A page of our own, with Doinua already loaded, needs the
emulator pin moved to fm1-emu's current version (an owner's decision).

## Installing on a real FM-1 (not done yet: read this first)

Nobody has run this on hardware. Before the first install:

1. Back up your FM-1, and have a way back to stock firmware: [Felucca's web installer](https://hugelton.github.io/Felucca/)
   has "Return to official V15", and there are rescue tools if an install fails.
2. Install with felucca-bsp's installer: `python3 third_party/felucca-bsp/tools/fm1_install.py build/companion/companion.fwsc`
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
`factory/cpu.py` keeps every sound under 80 % of the FM-1's CPU, and the build stays under 90 % of the app's flash.
The owner does three things: approves the spec, approves the screens (in the conversation; the agent runs the
**Approve screens** workflow, D-123), and tries it on the FM-1.

## Licence

The selected OberVave factory programs in `fm1kit/src/obervave/bank.h` come from [OberVave 3.5.2](https://github.com/zvenson/obervave/tree/a1f660c), GPL-3.0-only.
The CoralFlower VA engine and nine selected factory patches in `fm1kit/src/coralflower/` come from [CoralFlower](https://github.com/FicusFicusov/CoralFlower/tree/46790b9), GPL-3.0-only.

GPL-3.0-only (`LICENSE`). `fm1kit/src/device.c` and `fm1kit/app.ld` are derived from felucca-bsp, so
they stay MPL-2.0. The Inter Tight font is SIL OFL 1.1 (`assets/fonts/OFL.txt`). The FMini engine (`fm1kit/src/fmini/`) is fmini-1's (GPL-3.0-only, https://github.com/ruppert/fmini-1), ported (D-086). The Juno-106 engine (`fm1kit/src/juno/`) is FM-106's (GPL-3.0-only, https://github.com/PatttF/fm-106), ported (D-090). The selected Bass Something 2 patch data (`fm1kit/src/bs2/`) comes from John Brown’s MPL-2.0 factory bank (https://github.com/imijes/bass-something-2, 4c8a82f). Each package carries
three unmodified JieLi SDK files (Apache-2.0, `LICENSES/`). This is an independent project, not made
or endorsed by M-VAVE or Hügelton Instruments.

## Published

This repository starts from one commit: the work up to 2026-11-22, without its earlier history (which held the
owner's private requirements method, removed in D-058). The decisions behind everything are in `docs/PLATFORM.md`
(D-001 onwards); the licences of what it includes are under "Licence" above and in `LICENSES/`.
