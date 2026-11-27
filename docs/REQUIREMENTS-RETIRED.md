# Retired requirements

Requirements the owner retired: they describe behaviour the firmware no longer has. Kept for the record, with the decision that retired them; `factory/trace.py` does not read this file. Their scenarios are in `features/retired.feature`, and their checks were removed from `factory/scenarios/` in the PR that retired them (git keeps them).

## The manual chord builder (D-036), retired by ST-82

Retired on 2026-11-20 by the owner (#108, D-079): the chords' IDEAS page is now chord ideas (R-261..R-264). Building a chord moved to PLAY's HELP Key and Shapes (K3 COLOUR, K4 VOICING); ARP comes back with ST-77.

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-085 | When a knob changes the chord, the manual page shall light that chord's keys at the full level and show its notes on the screen's piano. | D-036 | A | Retired (#108) | The knobs build a chord that the keys and the screen show |
| R-086 | When K1 turns, the manual page shall step through the seven chords of the song's key before the five roots outside it. | D-036 | A | Retired (#108) | The knobs build a chord that the keys and the screen show |
| R-087 | While the song's key is AUTO, the manual page shall use the key that the recorded notes rank first. | D-036, D-032 | A | Retired (#108) | The manual page takes its key from what is recorded |
| R-088 | When a root outside the key is chosen, the manual page shall show that the chord is outside the key. | D-036 | A | Retired (#108) | The knobs build a chord that the keys and the screen show |
| R-089 | When notes are held, the manual page shall show the name of the chord they form. | D-036, D-035 | A | Retired (#108) | The knobs build a chord that the keys and the screen show |
| R-090 | When EDIT is pressed, the manual page shall switch its lights off or back on. | D-036 | A | Retired (#108) | EDIT switches the lights off and back on |
| R-091 | While the song plays, the manual page shall light the keys of the chord recorded at that moment at the breathing level. | D-036, D-031 | A | Retired (#108) | Chords made on the pad breathe on the manual page as the song plays |
| R-092 | When SELECT turns, the manual page shall set the knobs to the next or previous chord of the chord pad's selected progression, or of I-V-vi-IV when none is selected. | D-036 | A | Retired (#108) | SELECT walks the ideas of the chord pad's progression |
| R-093 | The chord pad and the manual page shall show the chords track's notes on the screen where they were played in the loop. | D-036, D-037 | A | Retired (#108) | Chords made on the pad breathe on the manual page as the song plays |
| R-094 | While ARP is on, the manual page shall play the held notes one at a time at the song's tempo. | D-036 | A | Retired (#108) | ARP plays the held notes one at a time at the song's tempo |

## The chord pad on AUTO (D-034), retired by ST-83

Retired on 2026-11-20 by the owner (#108, D-080): the chords' AUTO page is now song parts (R-265..R-270). Playing chords from one finger moved to PLAY's HELP Pads (C = I, the colours on the black keys, NEXT, AGAIN, LOCK: R-254..R-260).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-075 | When a white pad is pressed, the chord pad shall sound the chord of that scale degree in the song's key or palette. | D-034 | A | Retired (#108) | One finger plays a style's progression with its colours |
| R-076 | While a progression is selected, the chord pad shall give each chord the colour the progression assigns to it. | D-034, D-033 | A | Retired (#108) | One finger plays a style's progression with its colours |
| R-077 | When NEXT is pressed, the chord pad shall sound the progression's next chord. | D-034 | A | Retired (#108) | One finger plays a style's progression with its colours |
| R-078 | When a chord is played, the chord pad shall light the next chord's pads at the breathing level. | D-034 | A | Retired (#108) | One finger plays a style's progression with its colours |
| R-079 | While a black key is held, the chord pad shall add that key's colour to the chord played. | D-034, D-033 | A | Retired (#108) | A held black key adds its colour and AUTO locks it |
| R-080 | When AUTO is pressed with a colour held, the chord pad shall add that colour to every chord until AUTO is pressed again. | D-034 | A | Retired (#108) | A held black key adds its colour and AUTO locks it |
| R-082 | While the song plays, the chord pad shall light the pads of the chord recorded at that moment at the full level. | D-034, D-031 | A | Retired (#108) | Chords played on the pad are recorded as played and light their pads as the song plays |

## The drums PLAY knobs REPEAT, TUNE, DECAY (D-053), retired by ST-96

Retired on 2026-11-26 by the owner (D-121, the Sampler Lab): PLAY's knobs are LEVEL, REVERB, GRID, FEEL (R-367..R-370); TUNE and DECAY moved to EDIT SOUND (R-383); the ARP repeats a held drum (R-305).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-129 | When K1, K2 or K3 turns on a drums track's PLAY page, the companion app shall change the repeat, the tuning or the decay of the sound played last. | D-053, UC-10, ST-38 | A | Retired (D-121) | Each pad keeps its own repeat, tuning and decay |
| R-130 | While a key with a repeat is held on a drums track, the companion app shall play its sound again at the repeat's interval at the song's tempo. | D-053, UC-10, ST-38 | A | Retired (D-121) | Each pad keeps its own repeat, tuning and decay |

## GRID on playback, and erasing with REC off (D-051), retired by ST-96

Retired on 2026-11-26 by the owner (D-121, the Sampler Lab): GRID places each note as it is recorded and FEEL moves the
notes afterwards, from where each was recorded (R-371, R-372; D-127); erasing with EDIT and a key needs REC on (R-374,
R-375).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-140 | While a track's GRID is set, the companion app shall play the track's notes at the nearest grid position. | D-051, UC-13, ST-40 | A | Retired (D-121) | GRID and FEEL change the playback, never the recording |
| R-141 | The companion app shall keep a track's recorded notes as played when its GRID or FEEL changes. | D-051, UC-13, ST-40 | A | Retired (D-121) | GRID and FEEL change the playback, never the recording |
| R-142 | While a track's FEEL is set and its GRID is on, the companion app shall move each note by that feel's swing and shift for the note's drum group or track. | D-051, UC-10, ST-31 | A | Retired (D-121) | GRID and FEEL change the playback, never the recording |
| R-146 | While EDIT is held with a key held as the song plays, the companion app shall remove that key's notes of the track as the playhead passes them. | D-051, UC-13, ST-41 | A | Retired (D-121) | EDIT and a key erase that key's notes as they play |
