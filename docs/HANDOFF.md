# Handoff: getting started on Doinua (for a new agent)

Written 2026-11-23 by the agent that built Doinua so far, for whichever agent takes over. Read this first, then
`CLAUDE.md` (the rules: binding), then `docs/PLATFORM.md` (the API and the decision log D-001..D-094).

## What this is

**Doinua** ("melody" in Basque, made in Donostia) is custom firmware for the **M-VAVE FM-1** (a small synth, JieLi
AC79 chip, 240x240 screen, 27 keys, 4 knobs + 3 encoders). The product is the **companion** app (`apps/companion/`): a
small studio that teaches music to children and beginners. Four tracks (CHORDS, BASS, MELODY, DRUMS), help levels
(play freely, in key, shown, played for you), a sequencer, songs kept over power-off, games (unicorn, train, Hiru
damatxo), guided lessons. `apps/hello/` is a small test app. Everything sits on **fm1kit** (`fm1kit/`), our platform
layer with two backends: the real FM-1 (`device.c`) and a PC simulator (`host.c`).

The project began as `fm1-scales` (a private repo, now the archive with the full history). Doinua (public) starts
from one commit of 2026-11-22. Code and docs still say "fm1-scales" in places.

**Owner**: `ekhve` (product owner, decides what the child sees/hears/learns, safety, licences, publishing). The agent is
the "factory lead": it writes code, keeps the backlog (issue #1 "Factory status"), merges its own PRs once
`factory/check.sh` and CI pass. Owner decisions are asked in the conversation and recorded as D-nnn.

## How to build and test

```sh
tools/setup.sh                 # JieLi toolchain + fm1-emu emulator (once; CI caches it)
python3 tools/build.py companion   # also: hello. -> build/companion/companion.fwsc (the package an FM-1 installs)
factory/check.sh               # every gate: reqlint, trace, build, unit tests, music oracle, safety, USB MIDI,
                               # CPU (cycle-modelled, < 80 %), and ~670 scenario checks on host AND emulated firmware
```

- Scenarios: `factory/scenarios/*.json` (format: docstring of `factory/run.py`). Key scripts: `T:C4@100`, `T:C4^`,
  `T:e6:+3` (encoder 6 = PRESETS), `T:mHEX` (MIDI into the DIN jack).
- Screens are compared with `factory/approved/*.png`. **Only the owner approves screens** (Actions > "Approve screens").
- `factory/cpu_timeline.py`: a tool (not a gate) charting CPU from power-on through a song.
- The firmware has **never run on a real FM-1**. Install notes: README "Installing on a real FM-1".
- The browser emulator: https://joakimeriksson.github.io/fm1-emu/ (drop a .fwsc on it).

## Rules that bite (from CLAUDE.md and experience)

- **Never weaken a gate**; never approve screens; never touch the safety parts (loader, update/UBOOT paths, boot
  guard in `device.c`, `factory/safety.py`, `tools/build.py` checks) without the owner.
- **No 64-bit multiplies in device code** (the CPU gets them wrong). Use 32-bit Q15 tricks (see `fxbus.c` `mulq15`).
- **Rounding**: `>> 15` floors negatives, so feedback loops keep a -1 for ever (a DC hum). Round to nearest or towards
  zero in loops (`q15z` in `synth.c`, `mulq15` in `fxbus.c`).
- Every `fm1kit.h` change bumps `FM1KIT_API` (now **36**) and gets a D-nnn.
- CI's compiler is newer than a typical local one: an implicit declaration is an **error** there. `engines.c` does
  not include `kit_internal.h`; declare what it calls next to its other prototypes.
- Flash: the companion is **~432 KB of 523 KB** allowed (90 % of the app area, `safety.py`). RAM pool ~51 KB free.
- CPU: heaviest sound **79 %** of the 80 % gate (`factory/cpu.py`). Any new engine/effect must be measured there.
- The emulator's emulated device starts notes ~25 ms after the host: pitch checks right at a note's start are
  fragile for slow-attack sounds.

## What is built (newest first; details in docs/PLATFORM.md)

| D | What |
|---|---|
| D-094 | Every engine's sound plays its preset's effects (owner's call). Reverb tails die out (q15z). |
| D-093 | **Effects bus** `fm1kit/src/fxbus.c` (API 36): per-track DRIVE, sends to CHORUS / DELAY (0.74 s) / REVERB. `kit_track_sends`, `kit_fx_bus`. FMini patches set the bus from their 12 FX values. |
| D-092 | Big fonts run-length coded: 64 KB of flash back. |
| D-091 | CPU gate 80 %; voice budget knows engine costs (FMini 9.2 %, Juno 9.8 % + 15 % a track); `KIT_CPU_LIMIT` 75. Juno: 5 voices. |
| D-090 | **Juno-106** engine (FM-106 port, GPL-3), 28 owner picks, `fm1kit/src/juno.c` (float, built for the AC79 FPU). |
| D-089 | Images LZ coded. |
| D-088 | **ARP** on every track (`apps/companion/arp.c`). |
| D-087 | **Sound Browser** (PRESETS opens a grouped list; `tools/gen_browser.py` from `assets/sounds/browser.json`). |
| D-086 | **FMini** engine (fmini-1 port, GPL-3), 66 owner picks. |
| D-085 | Song kept over power-off (`bank.c`, one song). |

Engines today: Basic, Analog, Phase, Lo-fi, Trio, Organ wheel, Noise, Drums (Felucca-Engines), FMini, Juno-106.
The owner picked **208 sounds** in an audition (`assets/sounds/browser.json`); 99 are playable now (the rest wait for
their engines).

## Backlog (owner's priority: the music side first)

Issue **#1 "Factory status"** is the live list. In order:

1. **K-WASUB audition (in progress).** K-WASUB (https://github.com/Guizmoh/K-WASUB_Firmware_for_FM1, GPL-3.0) is
   a bass firmware: 121 presets (97 basses, 24 synths), 9 oscillator engines. Plan: record all presets, build an
   audition page, the owner picks, then port the engine part + the picks (~20-40 KB). Tools in `tools/audition/`:
   - build its WebAssembly: in its repo `source/`, run the `clang --target=wasm32 ...` line of
     `web/make_emulator.sh` (output `build/web/ksub.wasm`). (fm1-emu plays its .fwsc but gives **no audio**.)
   - `python3 tools/audition/plan_kw.py kw_all.json 0 121 <path to ksub_data.c>` writes the events
     (PRESETS +1 a slot, 13.5 s a slot, a bass phrase over MIDI); then
     `node tools/audition/kw_rec.mjs ksub.wasm kw_all.json kw_all.wav <ms from kw_all.json.slots.json> kw_all.shots.json`.
     A probe of 3 presets worked (sound in every slot). Next: record all 121, cut a clip per slot, check the names on
     the screen grabs, make the page (earlier rounds: one page, a clip per sound, the owner ticks favourites).
2. **More engines** for the owner's picks: Prophet (27, from Melodee), OberVave (19), BS2 (11), VA (9), the rest.
   Port like FMini/Juno: copy the engine with its licence, generate a bank of the picks, wire `engines.c`, add a
   unit test + hello scenario, measure CPU and flash, docs D-nnn, Draft requirements.
3. **FMini arpeggio patches** play through our ARP (each patch's arp settings, `fmini_x_t.arp[7]`).
4. **Sound editing + FX pages** after the Sound Edit Lab v3 (pages OSC FILTER F.ENV A.ENV VOICE LFO MOD; FX:
   PERFORM DELAY REVERB DIRT). Screens need the owner's approval; build them as the lab draws them (rule 10).
5. **Several songs** (save, load, name); today one song is kept (D-085, `kit_bank_*`, 16 KB).
6. **Drum kits from the owner's samples** (pipeline exists: `kits/user/`, `factory/userkit.py`, D-056). Waits for the
   samples (WAV mono 44.1 kHz, one hit a file, `NN-name.wav`, with a LICENSE).
7. Softer game sounds (unicorn, train were too strident for the owner; Hiru damatxo is fine).
8. Later: start-up logo (the owner checks it), the visual "just play" mode (Donostia bay; the Kursaal seen from the
   María Cristina bridge: the owner will send a sketch), sampler (Sampler Lab questions open), sampled piano (40-80 KB,
   owner to decide).

## Waiting on the owner

- Draft requirements R-307..R-315 to confirm (`docs/REQUIREMENTS.md`, Status "Draft").
- First test on a real FM-1 (play test: effects, Juno chords, CPU in use).
- K-WASUB picks (after the audition page).
- Drum samples; sampler answers; sampled piano yes/no; logo; Kursaal sketch.

## The labs (screen designs)

`docs/labs/` holds the interactive HTML mock-ups the owner shaped (index: `docs/labs/README.md`). **A lab is the
contract for its screen** (D-084). The next one to build is `sound-edit-lab.html`. The owner's earlier published
pages on claude.ai are private to the owner; everything an agent needs is in the repo.

## Where things are

`fm1kit/include/fm1kit.h` (the API) · `fm1kit/src/` (synth.c voices/mix/reverb, engines.c engine glue, fxbus.c,
juno.c, fmini/, juno/, bank.c, store.c, device.c, host.c) · `apps/companion/` (the product) · `factory/` (gates,
scenarios, approved screens) · `docs/` (PLATFORM decision log, REQUIREMENTS EARS, REQUIREMENTS-PLAYBOOK, USE_CASES,
STORIES, MANUAL, ARCHITECTURE, HAZARDS, TRACEABILITY generated) · `features/fm1-scales.feature` (Gherkin) ·
`tools/` (build, generators, audition).
