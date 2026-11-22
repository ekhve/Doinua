# Chord pad and chord lane — usage scenarios and draft requirements

**Status:** draft for the owner's approval (2026-11-07). From the owner's feedback rounds on simulator v5–v7 and the
v0.3 workflow. Once approved, the requirements move to `docs/REQUIREMENTS.md` (section 7) and the scenarios to
`features/fm1-scales.feature`, and the stories are raised. The playable model: `simulator.html` (P1, P3 tabs).

## 1. Chord pad (CHORDS track, PAD)

### Usage scenarios

| # | Scenario | Steps (you do → the FM-1 does) |
|---|---|---|
| CP-1 | **Play a style's progression with one finger** | K1 STYLE → Neo soul · K2 → ii-V-I · press F5 NEXT, NEXT, NEXT → Dm9, G13, Cmaj7 sound in turn; the chord now is white on the keys and coral on screen, the next breathes light blue |
| CP-2 | **Make your own progression with ideas** | Press any pad → it sounds with the style's colour; the next chord of the progression (or, on Free, a likely next chord) breathes; press it or anything else, the suggestion picks up from where you are |
| CP-3 | **Add or lock a colour** | Hold 13 (a black key) and press a pad → that chord gets the 13 · hold 9 + tap AUTO → every chord is a 9 until AUTO is tapped again · tap AUTO alone → the style's colours off (plain triads) and on |
| CP-4 | **Change how chords sound** | K3 VOICING → root, inversion 1, 2, open, wide · K4 PLAY → block, strum, arpeggio · PRESETS → the sound (the whole list, the track's suggestions first) |
| CP-5 | **Record in real time** | REC + PLAY → what is played goes into the sequencer as played (not quantised). No step entry (the owner, 2026-11-07) |
| CP-6 | **See what is recorded** | PLAY → the pad of the chord recorded at this moment lights (also chords recorded in MANUAL) |
| CP-7 | **Know where you are** | Banner: mode, track, key, tempo, octave, sound · knob labels on top · hold GLO → help for this screen |

### Keys and knobs

| Control | Does |
|---|---|
| White F3–E4, F4–E5 | The key's chords I–vii°, twice (the second an octave up); I is always the first white key |
| F5 · G5 | NEXT (the progression's next chord) · AGAIN (the last chord again) |
| Black keys (held) | sus2, sus4, 7, maj7, 9, 11, 13, add9, 6, slash on the 3rd |
| F#5 | AUTO: the style's colours on/off; with a colour held: lock it; again: unlock |
| K1 / K2 | STYLE / PROGRESSION (usual first, each chord with its colour) |
| K3 / K4 | VOICING (root, inv 1, inv 2, open, wide) / PLAY (block, strum, arpeggio) |
| Lights | The chord now (played or recorded): full · the next: breathing · I: marked as home · the rest: dim |

### Chords and voices

Chords keep every note of their colour and voicing (no limit: the owner, 2026-11-07). The FM-1 has eight voices for
the four tracks (D-029); when they are all in use the chords give way first, so a big chord over a busy song may lose
a note while the bass, melody and drums keep theirs. The CPU gate measures the biggest chords. The theory comes from
Sketchpad, checked by the oracle (rule 7): the colour chords and voicings are a port to add.

### Draft requirements (EARS; to approve)

| ID | Requirement | Verify |
|---|---|---|
| CP-R1 | When a white pad is pressed, the chord pad shall sound the chord of that scale degree in the song's key or palette. | A |
| CP-R2 | While a progression is selected, the chord pad shall give each chord the colour the progression assigns to it. | A |
| CP-R3 | When NEXT is pressed, the chord pad shall sound the progression's next chord. | A |
| CP-R4 | When a chord is played, the chord pad shall light the next chord's pads at the breathing level. | A |
| CP-R5 | While a black key is held, the chord pad shall add that key's colour to the chord played. | A |
| CP-R6 | When AUTO is pressed with a colour held, the chord pad shall add that colour to every chord until AUTO is pressed again. | A |
| CP-R10 | When GLO is held, the firmware shall show help for the current screen. | A |
| CP-R11 | While the song plays, the chord pad shall light the pads of the chord recorded at that moment at the full level. | A |

Owner questions: the starter list of styles and progressions (to be curated from the owner's artists: hip hop, neo soul,
electronic, Funkadelic, G-funk / West coast, Chromeo); K4 patterns beyond block, strum and arpeggio (J-6-like, later).

Removed by the owner (2026-11-07): a four-note limit (CP-R7); a chord-pad copy of the sequencer's "keep what is played" (CP-R9, already R-063 and R-064).

## 2. Manual chords (CHORDS track, MANUAL) — the owner's direction of 2026-11-07

**Why:** to play chords yourself, learn and experiment, without the chord pad's automatic part. The knobs build a chord;
the piano on screen and the keys show it; you play it.

### Usage scenarios

| # | Scenario | Steps (you do → the FM-1 does) |
|---|---|---|
| MC-1 | **Build a chord and play it** | K1 root · K2 type · K3 colour · K4 voicing → its notes light (full) on the keys and on the screen's piano, with names; play them → the screen names what you hold, with a tick when it is the chord |
| MC-2 | **Choose in the key or outside it** | K1 steps through the key's chords first (each with its type), then the five roots outside the key (marked "outside the key"); the key is GLO's (AUTO: the one the recorded notes suggest) |
| MC-3 | **Play without help** | EDIT → the lights off; EDIT → on again |
| MC-4 | **Layer a sequence live** | PLAY + REC → play the lit chord; while it loops, turn the knobs to the next chord (lit) and play it at its moment; repeat. What is recorded breathes as it plays |
| MC-5 | **See chords made on the pad** | Chords recorded with the chord pad breathe on the keys as they play, named on screen ("Playing now: Am") |
| MC-6 | **Walk the ideas** | SELECT ▶ / ◀ → the knobs take the next / previous chord of the progression chosen on the chord pad (or I-V-vi-IV in the key when none is), with its colour; the screen shows the next one |
| MC-7 | **Arpeggiate** | ARP → the arpeggiator plays the held notes one by one at the tempo |

### Controls

| Control | Does |
|---|---|
| K1 ROOT | The key's chords I–vii° (with their type), then the five roots outside the key |
| K2 TYPE | major, minor, sus2, sus4, dim, aug |
| K3 COLOUR | –, 7, maj7, 9, add9, 6, 11, 13 |
| K4 VOICING | root, inversion 1, 2, open, wide |
| SELECT | ◀▶ the ideas: the pad's progression (shared), or I-V-vi-IV |
| EDIT | Lights on / off |
| ARP | The arpeggiator (on the held notes) |
| Keys | A normal keyboard: any note plays and is recorded as played |
| Lights | Your chord: full · recorded, playing now: breathing · the key: dim |

### Draft requirements (EARS; to approve)

| ID | Requirement | Verify |
|---|---|---|
| MC-R1 | When a knob changes the chord, the manual page shall light that chord's keys at the full level and show its notes on the screen's piano. | A |
| MC-R2 | When K1 turns, the manual page shall step through the seven chords of the song's key before the five roots outside it. | A |
| MC-R3 | While the song's key is AUTO, the manual page shall use the key that the recorded notes rank first. | A |
| MC-R4 | When a root outside the key is chosen, the manual page shall show that the chord is outside the key. | A |
| MC-R5 | When notes are held, the manual page shall show the name of the chord they form. | A |
| MC-R6 | When EDIT is pressed, the manual page shall switch its lights off or back on. | A |
| MC-R7 | While the song plays, the manual page shall light the keys of the chord recorded at that moment at the breathing level. | A |
| MC-R8 | When SELECT turns, the manual page shall set the knobs to the next or previous chord of the chord pad's selected progression, or of I-V-vi-IV when none is selected. | A |
| MC-R10 | While ARP is on, the manual page shall play the held notes one at a time at the song's tempo. | A |
| MC-R9 | The chord pad and the manual page shall show the chords track's progression on the screen, bar by bar. | A |

### Answered (the owner, 2026-11-07)

1. Two chords in one bar when layering live: both are kept, as played (that is the sequencer's business; here we put
   inputs).
2. The progression strip (MC-R9): always shown, the current bar marked.
3. ARP stays the arpeggiator; the ideas move to SELECT, based on the chord pad's progression (or a standard one).
