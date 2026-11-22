# Concept v0.5 — the FM-1 Music Companion: start a song anywhere

**Status:** draft by the agent, 2026-11-08, from the owner's direction of the same day, **for the owner to confirm**.
Builds on v0.2 (one song engine, three experiences), v0.4 (branching map, chord pad, manual chords) and what the
firmware already does. Once confirmed, the stories (`STORIES.md`) are raised, the scenarios (`SCENARIOS.md`) become
Gherkin, and the draft requirements below go to `docs/REQUIREMENTS.md` as they are built.

## 1. What this firmware is (for anyone)

The FM-1 Music Companion turns the M-VAVE FM-1 into a small studio that teaches while you make music. You build a
short song, a loop of up to eight bars, out of four tracks: **chords, bass, melody and drums**. You can start with
any of them. Begin with a groove on the drums, a melody you hum, or a few chords, and the FM-1 works out the key from
what you play. Then it helps you with the next part: which chords fit your bassline, which notes fit your chords,
what a bass could do in this style.

On every track you choose how much help you get. You can play freely, like on any keyboard. You can turn on the key,
so that every key you press sounds right. You can let the FM-1 show you what to play next. Or it can play the part
from one finger, the way the chord pad plays a whole chord progression. What you record is kept exactly as you
played it; the help only lights, suggests and offers.

There are three ways to use it:
- **Producer**: everything open. You make your own song, in any order, with as much help as you want.
- **Guided**: the same tools, in steps. The FM-1 leads you through building a song, from very easy to a bit harder.
  For beginners at producing music, older children and grown-ups.
- **Game**: playing along. A unicorn or a dinosaur runs and jumps while a child plays a melody; children can play it
  without reading.

## 2. Producer mode

### 2.1 Four tracks, one song

| Track | What you play | Help (the ladder, section 2.2) |
|---|---|---|
| CHORDS | chords | free · in key · guide (manual chords) · auto (chord pad) |
| BASS | a bassline | free · in key · guide · auto (to explore: section 5) |
| MELODY | melodies and motifs | free · in key · guide · (auto to discuss) |
| DRUMS | a beat | the kit (later: patterns). Later the fourth track may be swappable (a second melody or chords track instead of drums): backlog |

ALGORITHM picks the track. PLAY plays the song, looping; REC records the track you are on, as played (the
sequencer keeps every note's time, length and strength, nothing moved onto the beat). Each track keeps its own
sound (PRESETS). Tracks are recorded one over another while the loop goes round.

### 2.2 The ladder of help (the same on every track)

| Step | What the keys do | What lights and the screen show | Chords today |
|---|---|---|---|
| **Free** | Every key plays its own note | Nothing, or only what is recorded | MANUAL with EDIT (lights off) |
| **In key** | The keys stay a piano; the key's notes are lit, so you see what you can play; a knob sets whether the notes outside the key sound (yes, softly, or not at all) | The key's notes lit | — (to add) |
| **Guide** | Every key plays its own note | What you could play: the chord built with the knobs, the next idea of a style, the notes that fit the chord now | MANUAL |
| **Auto** | One key plays a whole part | The part's next step | PAD |

SCL steps through the track's pages (today: CHORDS PAD and MANUAL). The knobs set the help (the chord, the style, the
voicing) and the screen always shows what each knob does.

### 2.3 Start anywhere (the branching)

The song starts empty and its key is **AUTO**. The first notes recorded on the chords, bass or melody give the key
(the drums do not); it can also be chosen on GLO and **locked**. The other tracks then help in that key. A page never
changes what was recorded; it only changes what it lights, suggests and offers.

| The song has | CHORDS | BASS | MELODY | DRUMS |
|---|---|---|---|---|
| nothing | pads and chords in GLO's key (C major on AUTO) | free or in key; AUTO listens | free or in key; AUTO listens | the kit |
| drums only | as nothing | as nothing, with the beat to play to | as nothing | the kit |
| bass or melody | chords that fit what is recorded (Harmony Assist: v0.2, to design), in its key | the key's notes; the recorded notes | the key's notes | the kit |
| chords | as now (pad, manual); the recorded chords light as they play | the chord now: root and 5th lit, the key dim | the chord now lit, the key dim | the kit |
| chords and bass | as chords | as chords | as chords | the kit |

Recorded notes outside the key are kept as played and counted by a small ♯ icon. A locked key the notes contradict
shows the key they suggest beside it (v0.4, answered).

### 2.4 GLO: the song's settings

Tap GLO: the song page. **K1 key** (AUTO, then the 24 keys; on AUTO it shows the key it hears, and "?" while unsure),
**K2 tempo** (60–160 BPM), **K3 length** (1–8 bars), **K4 time signature** (4/4, fixed for now). Tap GLO again to go
back. Hold GLO: help for the screen you are on (built). Later: a length per track (a 2-bar bass looping under 4 bars
of chords), as the owner starts a bass with 2 bars and chords longer. Changing a setting never changes a recorded note; moving the song to
another key is a separate action (transpose, later).

### 2.5 HOME: the three modes

Hold HOME: the mode picker (Producer, Guided, Game). Turn SELECT to choose (the FM-1's SELECT has no push), press HOME
to pick, and HOME again to confirm ("Switch to Guided? Your song is kept"); any other button goes back. Tap HOME: the
mode's own home (Producer: the track page for now; a song overview later). The mode is the colour of the screen's top line: Producer blue, Guided teal,
Game violet. The song stays as it is when you switch. **For now** Guided and Game are placeholder screens: their name
and "coming soon".

## 3. Guided mode (preliminary)

Lessons that build a song step by step, with the Producer's own pages. Each step says what to do and why, plays a demo,
opens the page it needs with the right help, and moves on when the step is done. From very easy (one chord over a
beat, with the chord pad) to harder (your own bassline in the key, then chords that fit it, then a motif). It is a
simpler path through the Producer's tools, not other tools.

## 4. Game mode (preliminary)

Play along with a song: a unicorn or a dinosaur runs over a field and jumps a fence for each note; the next key
breathes; a wrong key gives a soft bonk and the game waits (D-026). For children, no reading needed. A study view (a
piano roll) for older players. Songs are original pieces in a style, and public-domain songs.

## 5. Bass (to explore next)

The bass follows the chords' ladder. Free: a bass keyboard an octave down. In key: every key in the scale. Guide: the
chord now (root, 5th, octave, 3rd) lit while the song plays, and the bass ideas of a style (G-funk slides, boom-bap
roots, reggaeton's tresillo, Chromeo's octaves) shown on the keys and walked with SELECT. Auto: one key plays a style's
bassline over the chord now. **Bass with drums** may be the better start: a groove (drums and bass together, or a drum
pattern under the bass page) is what makes a bassline make sense.

## 6. What the chord pad and manual chords need later

- Harmony Assist: when a bass or a melody is recorded first, the chords that fit it (the pad's palette from the
  recorded notes; SELECT walks the candidates).
- The styles and progressions from the owner's artists (`../v0.4/STYLES.md`), chords from outside the key, 8 steps.
- Rhythm patterns for the pad (like the J-6), beyond block, strum and arpeggio.

## 7. What is reused, and what comes later

The sequencer (four tracks, real-time recording, loop, tempo changes that keep the place, key AUTO), the player and the
arpeggiator, the four-track synth with eight voices, and the theory checked against Sketchpad are the base of every
mode. Sound design (EDIT macros, ENV, LFO, FX: delay, reverb, chorus) and more engines (the Minimoog port) come later;
they are about the sounds, not about playing and recording.

## 8. Draft requirements (EARS; to approve, then built story by story)

| ID | Requirement |
|---|---|
| CV-R1 | When a note is recorded on the chords, bass or melody track of an empty song whose key is AUTO, the song shall take its key from the recorded notes. |
| CV-R2 | While the song's key is AUTO or locked, the chords, bass and melody pages shall offer their help in that key. |
| CV-R3 | When a track is recorded, the firmware shall keep the notes already recorded on the other tracks. |
| CV-R4 | When GLO is tapped, the firmware shall show the song page with the key, the tempo, the length and the time signature on the four knobs. |
| CV-R11 | When a different mode is chosen in the mode picker, the firmware shall ask for a confirmation before it switches. |
| CV-R5 | When K1 turns on the song page, the firmware shall step through AUTO and the 24 keys. |
| CV-R6 | When the key or the tempo changes on the song page, the song shall keep its recorded notes. |
| CV-R7 | When HOME is held, the firmware shall show the mode picker with Producer, Guided and Game. |
| CV-R8 | When a mode is chosen, the firmware shall show that mode's home and colour the screen's top line in the mode's colour. |
| CV-R9 | While Guided or Game is not built, the firmware shall show that mode's name and a notice that it is not built yet. |
| CV-R10 | When the mode changes, the firmware shall keep the song. |

## 9. Answered by the owner (2026-11-08)

1. **The fourth track**: drums for now. A swappable fourth track (a second melody or chords) later: backlog.
2. **Bass and drums**: the bass alone, or with drums. Drums don't set the key, but give a sense of rhythm.
3. **The song's length**: a knob on GLO (K3, 1–8 bars). The owner starts a bass with 2 bars, chords longer: a length
   per track is for later.
4. **In key**: the keys stay a piano, the key's notes lit; a knob, where one is free, sets whether the notes outside
   the key sound.
5. **The mode picker**: hold HOME; SELECT chooses; HOME picks, with a confirmation; the mode's colour is the top line
   on every screen.
