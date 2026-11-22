# MELODY: a proposal (concept v0.5, 2026-11-12, for the owner)

**Status:** tried and reviewed (2026-11-12 and 13); the agreed result is in `docs/STORIES.md` (ST-26 to ST-29, ST-40 to ST-50), `docs/REQUIREMENTS-PLANNED.md` and `features/planned.feature`, which win where this concept differs. Lab: `melody-lab.html` (M-U1 to M-U10). It reuses the bass (D-040, D-041) and Sketchpad's melody theory (rule 7).

## The idea

The melody track works like the bass: the same three pages on SCL, **PLAY · IDEAS · AUTO**, and the same one-scale
help. Two things are the melody's own:

- **Help is about the chord, not only the scale.** Sketchpad's melody guide (`core/melody.mjs`, `melodyRole`) sorts
  every note over the chord playing now:
  - **lands well**: a chord tone, safe to hold and to end on;
  - **moves**: in the scale but not the chord, good for passing through;
  - **pulls**: outside the scale, for a moment on the way to a chord tone.

  The keys show it (full, dim, off) and the screen names it.
- **Ideas are figures, not fixed notes.** Sketchpad's nine melody figures (`core/figures.mjs`) are written in roles
  ("chord tone", "a step up", "the top of the chord", "approach the next chord"). `renderFigure` turns them into notes
  over any chord, in any scale, so one figure fits every song.

## PLAY: you play, with as much help as you like

The keys play the melody where they are (no octave down). OCT moves them.

| Knob | Does | From the bass? |
|---|---|---|
| **K1 HELP** | **Free** (no lights) · **Key** (the scale dim, its tonic breathing) · **Chord** (while the song plays, the notes that land well on the chord playing now full, the rest of the scale dim: Sketchpad's guide) · **Whites** (the white keys play the scale, the black keys rest) | Free, Key and Whites as on the bass; Chord replaces Key+root |
| **K2 SCALE** | The same list and pop-up: with chords (and now bass) recorded, the scales by fit, best first, with %; without, the key's own. It starts on the key's **pentatonic**: five notes, nothing clashes | same, a different first choice |
| **K3 OUTSIDE** | A note outside the scale **plays** or is **mute** | same (without "soft") |
| **K4 HARMONY** | **off** · **3rd** · **6th** · **octave**: each note gets a second voice above it, kept in the scale (a 3rd above C in C major is E, above D is F) | new |

**The screen:** the note you play big, and what it does over the chord now ("E · lands well on Am"). Under it, the
chord now and the scale. At the bottom, the piano with the lights.

## IDEAS: learn a hook

A figure plays over your chords (or the key's I-V-vi-IV when there are none). Its notes fall onto the keys they belong
to and land on their time, as the bass ideas do. You play each one as it lands; REC keeps what you play.

| Knob | Does |
|---|---|
| **K1 STYLE** | Sketchpad's styles: hip hop, soul, funk, house, west coast, cinematic |
| **K2 FIGURE** (or SELECT) | The style's figures: pentatonic hook, sparse and late, stabs, rising line, call and answer, two-note motif, arpeggio up, tumbling, falling line |
| **K3 TAKE** | Another version of the same figure: Sketchpad's figures are seeded, so each take picks other chord tones and steps in the same rhythm |
| **K4 HEAR** | listen/play · on · off (as on the bass) |

## AUTO: one finger, a whole phrase

As on the bass: the white keys are the key's degree pads. Hold one and the song starts, and the figure plays over that
chord; let go and it stops. **FOLLOW chords**: with the song playing, the figure follows the recorded chords by itself.
K1 STYLE, K2 FIGURE, K3 FEEL (straight or swing), K4 FOLLOW. REC records what it plays.

## What is reused, what is new

| Part | Reused | New |
|---|---|---|
| Pages, SCL, help screen | the bass's | the melody's texts |
| SCALE, its fit and pop-up | `bass_scale_list`, `kit_fit_scales`; the fit takes the chords and the bass | the pentatonic first |
| Lights: Key, Whites, OUTSIDE | `bass_role`, `bass_white_note` | Chord: Sketchpad's `melodyRole` (ported, with the oracle) |
| IDEAS: falling notes, HEAR, listen/play | the bass's IDEAS engine | figures from `renderFigure` (ported, with the oracle: the same notes as Sketchpad for the same seed) |
| AUTO: degree pads, the scheduler, swing, FOLLOW | the bass's AUTO | |
| HARMONY | | a diatonic second voice |

**In the code:** the bass's logic (`apps/companion/bass.c`) becomes a "line track" shared by the bass and the melody,
each with its own settings (octave, help names, styles). Its tests and scenarios keep passing unchanged. fm1kit gets
`kit_melody_role` and `kit_render_figure` (Sketchpad's, checked by the oracle).

## The owner's answers (2026-11-12)

1. The keys where they are (F3 to G5). 2. The first scale is the key's pentatonic. 3. A couple of styles now (a lead
sound), more later. 4. HARMONY now. 5. A lab first, with use cases to try, and nice phrases in styles for AUTO: Pop
(hook, climb), G-funk (a whistle lead that glides), Ghibli / Zelda (a bell arpeggio, a fanfare), Neo soul (a lick),
Reggaeton (a hook), Cinematic / Enya (a long line), Boom bap (a sample riff), Disco / Chromeo (octaves). Each phrase is a
call bar and an answer bar, written as degrees from the chord's root in the key, so it fits any chord; a note outside
the melody's scale moves to the nearest note in it.

## Questions for the owner (answered above)

1. **Where the keys sit:** as they are (F3 to G5), or an octave up by default (a melody often sits higher)?
2. **The first scale:** the key's pentatonic (safest: nothing clashes) or the key's own scale?
3. **Styles:** start with Sketchpad's nine figures in its six styles, and add figures for your artists later (a Zelda
   or Ghibli arpeggio, a G-funk whistle lead with a glide, a reggaeton hook, an Enya line)?
4. **HARMONY** (a second voice in 3rds or 6ths): in, or later?
5. **Try it first:** a melody lab in the browser like the bass lab (the FM-1's look, the three pages, your chords)
   before the firmware? (Recommended: the bass lab caught six things before any code.)

## Stories it would add

- **ST-26** As a producer, I want the melody keys to show which notes land well on the chord playing, which move and
  which pull, so that my melody fits the chords and I learn why.
- **ST-27** As a producer, I want melody ideas (hooks and figures in a style) that fit my chords and fall onto the keys,
  so that I learn a melody and make it mine.
- **ST-28** As a producer, I want to hold one key and have a melody phrase play over the chord, so that I hear a
  melody idea in my song at once.
- **ST-29** As a producer, I want a second voice in 3rds or 6ths, kept in the scale, so that my melody sounds fuller.
