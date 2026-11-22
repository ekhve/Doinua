# DRUMS: a proposal (concept v0.5, 2026-11-12, for the owner)

**Status:** tried and reviewed (2026-11-12 and 13); the agreed result is in `docs/STORIES.md` (ST-30 to ST-35, ST-38 to ST-50), `docs/REQUIREMENTS-PLANNED.md` and `features/planned.feature`, which win where this concept differs (one sound a key, no repeated octaves; per-pad settings; GRID and FEEL in the REC layer). Lab: `drum-lab.html` (D-U1 to D-U10). The lab makes every
sound with the synth; the firmware's sampled kits come from open-licence recordings.

## The owner's answers (2026-11-12)

1. Four kits, part synthesized and part sampled: yes.
2. Samples: open source (as Felucca: CC0, IMA ADPCM in flash).
3. Pockets: explore more; Dilla, laid back, pushed and the swings are good.
4. A lab first.
5. New: a **user kit**, the owner's own samples (with their licence) mapped to the keys.
6. New: **FX** like an MPC: a compressor and a soft clipper; lo-fi.

## The kits

| Kit | Made with | For | Pieces |
|---|---|---|---|
| Dusty | samples | boom bap, Dilla | kick, snare, rim, hats, shaker, tambourine |
| 808 | synth (Felucca's drum voices, GPL as ours) | G-funk, trap, reggaeton | 808 kick, clap, hats, cowbell, toms |
| Live | samples | funk, D'Angelo, disco | kick, snare with ghost notes, cross stick, hats, toms, ride bell |
| Latin | synth and samples | dembow, shatta | congas, bongo, clave, guiro, timbal |

The pads are the same in every octave: C kick, C# rim, D snare, D# clap, E ghost snare, F G A toms, F# hat, G#
shaker, A# open hat, B percussion. The closed hat cuts the open one. Each sampled piece has two takes that alternate.

**Space:** the app area is about 580 KB and the companion uses 218 KB, so about 360 KB is free. A sampled kit of ten
pieces at 22 kHz IMA ADPCM is about 25 to 35 KB, and the synthesized kits cost nothing, so the four kits need about
120 to 150 KB.

## The pages

- **PLAY:** the pads. K1 KIT, K2 POCKET, K3 GRID (as played, 1/16, 1/8), K4 FX. What you play is recorded as
  played; GRID and POCKET act on playback only.
- **IDEAS:** a style's beat falls onto its pads; listen, then your turn. K1 STYLE, K2 BEAT (main, busy, fill, break,
  drop), K3 SPEED, K4 HEAR.
- **AUTO:** hold a pad and the style's beat plays: main, busy, fill, break, drop on F3 to C4. K1 STYLE (boom bap,
  Dilla, G-funk, funk, disco, half-time, dembow, shatta, trap; each brings its kit, tempo and pocket), K2 POCKET,
  K3 KIT, K4 FX.

## POCKET

Each drum group (kick; snare and claps; hats and shakers; toms and percussion) gets its own swing (MPC %), shift (ms)
and looseness (ms, the same each loop unless "loose"). Velocities vary a little; ghost notes are soft snares.

| Pocket | Kick | Snare | Hats | Feel |
|---|---|---|---|---|
| Straight | | | | on the grid |
| Swing 55, 62, MPC 66 | swing | swing | swing | the off-sixteenths later |
| Dilla | swing 64, 6 ms early | 26 ms late | nearly straight, loose (14 ms) | two feels rubbing |
| Laid back | | 20 ms late | 12 ms late | relaxed |
| Pushed | | 6 ms early | 12 ms early | urgent |
| Dembow lean | | 9 ms late, light swing | light swing | |
| Shatta bounce | 9 ms early, swing 60 | swing 60 | swing 60 | |
| Drunk | loose 18 ms | loose 18 ms | loose 22 ms | too far, on purpose |

## FX (on the drums)

Punch (a compressor), MPC (a fast compressor and a soft clipper), Lofi (fewer bits, the highs cut, vinyl crackle),
Tape (warm saturation). In the firmware: integer DSP on the drum track, checked by the CPU gate.

## The user kit (proposal)

A folder in the repository, `kits/user/`, with one sound per key: `01.wav` is F3, `02.wav` F#3, ... `27.wav` G5 (numbers, the owner's
choice of 2026-11-12), and a `LICENSE` file, which the build requires. The build converts them (22 kHz IMA ADPCM),
checks the size (about 60 KB, some 5 seconds of sound), and puts them into the firmware as kit 5, USER. Installing
the firmware installs the kit. Later, with the web editor (#60), the same kit could be sent over USB without a rebuild.

## Stories it would add

- **ST-30** As a producer, I want four good drum kits with percussion, so that my beats sound real in the styles I like.
- **ST-31** As a producer, I want to give my drums a pocket (swing, Dilla, laid back, pushed), so that they groove like
  a drummer or an MPC instead of a grid.
- **ST-32** As a producer, I want to hold one pad and get a style's beat, with a busier one, a fill, a break and a drop,
  so that I have drums at once.
- **ST-33** As a producer, I want to learn a style's beat on the pads, so that I can play it myself.
- **ST-34** As a producer, I want an MPC-style compressor and soft clipper, lo-fi and tape on the drums, so that they
  hit hard or sound dusty.
- **ST-35** As a producer, I want to put my own samples (with their licence) on the keys as a kit, so that the FM-1
  plays my sounds.
