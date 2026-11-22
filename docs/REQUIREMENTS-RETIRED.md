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
