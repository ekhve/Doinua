# Usage scenarios — concept v0.5 (draft for the owner)

Two levels (the owner, 2026-11-08):

- **Usage scenarios (U-n)**: how someone uses the FM-1, as a story of what they want and do, without keys or knobs.
  They show the concept and the branching, and become use cases in `docs/USE_CASES.md` once confirmed.
- **Test scenarios (T-n)**: the same journeys step by step with the FM-1's keys, knobs and screens: **you do → the
  FM-1 does → what a test checks.** They become Gherkin scenarios and executable checks as they are built, and serve
  the manual and instructional videos.

## 1. Usage scenarios

### U-1 Groove first (ST-01, ST-02, ST-03, ST-04, ST-11)
Ana has a rhythm in her head. She records a simple beat on the drums, then plays a bassline over it, freely. When she
stops, the FM-1 tells her the key it hears in the bass. She goes to the chords: the pads are now that key's chords,
and the bassline shows as it plays, so she finds chords that sit on it, one finger at a time, and records them. Last,
a short motif on the melody, with the key's notes lit so every note she plays fits.

### U-2 Melody first (ST-01, ST-02, ST-10)
Leo whistles a tune. He plays it on the melody track and records it. The FM-1 works out the key, or says it is not
sure yet. He adds a single bass note under it, then asks for chords that fit the melody and holds one chord for the
whole loop. A beat on the drums finishes the idea.

### U-3 Chords first (ST-01, ST-03, ST-04)
Mia picks a style she likes and plays its progression with one finger on the chord pad, adding a colour here and there,
and records it as the loop goes round. On the bass she sees the root of each chord light as it comes, so her first
bassline follows the chords; later she plays freely around it. She ends with a few motifs on the melody, and goes back
to manual chords to change one chord by hand.

### U-4 Change your mind (ST-02, ST-04, ST-06)
Halfway through, Ana wants it faster and in another key. On the song page she turns up the tempo while it plays; the
loop goes on from the same place. She locks a key; the pads move, her recorded notes stay as played, and the FM-1 shows
the key the notes suggest beside the one she chose. She re-records the bass over the old one, makes the loop shorter to
work on two bars, and switches to another mode and back: the song is still there.

### U-5 Learn step by step (ST-07, ST-08)
Sam has never made a song. He chooses Guided. The FM-1 plays a beat and asks him to press the lit pad; it records his
chord and explains why it sounds like home. Step by step he adds a second chord, a bassline that plays the roots, and
a motif, each step opening the page it needs with the right help. Later lessons leave him more freedom.

### U-6 Play along (ST-07, ST-09)
Noa, six, chooses Game and a song she knows. A unicorn runs across a field; the next key breathes. Each right key
makes the unicorn jump a fence; a wrong one gives a soft bonk and the game waits for her. She can't read yet, and
doesn't need to.

### U-7 Just play (ST-03, ST-04)
Someone wants the FM-1 as a plain instrument: four tracks, any key, no help. They pick each track's sound, play and
record freely, and use the help only when they want it.

## 2. Test scenarios

Keys are the FM-1's (F3–G5); "the song" is 4 bars at 90 BPM unless the song page changes it. A step marked *(later)*
needs a part not built yet; the rest can be tested today on the emulator.

### Producer

#### T-1 Groove first (U-1): drums, bass, then the key, chords, a motif
1. Hold HOME, choose Producer → the song overview, top line blue → *the mode picker and Producer's home (later)*.
2. ALGORITHM to DRUMS; REC, PLAY; play a kick on the beat and a snare on 2 and 4 (C4 a kick, D4 a snare) → they are recorded as
   played → *the loop line shows the hits; they play back the next time round; the key is still AUTO (drums don't set it)*.
3. ALGORITHM to BASS; REC; play A3 C4 D4 E4 over the beat (the bass sounds an octave down); REC off; PLAY stops → the key is worked out from the bass
   → *the top line shows A minor (or "?" while unsure); the drums are unchanged*.
4. ALGORITHM to CHORDS, SCL to PAD → the pads are A minor's chords; the recorded bass breathes on the keys as it plays
   *(the bass on the chord pages: later)* → *the first pad plays Am*.
5. REC, PLAY; one finger on NEXT (or the pads) plays the chords → recorded as played → *the loop line on CHORDS shows them*.
6. ALGORITHM to MELODY; play a short motif with "in key" on *(later)* → *every key sounds in A minor*.

#### T-2 Melody first (U-2): a motif, a bass, drums, one chord
1. ALGORITHM to MELODY; REC, PLAY; play E4 G4 A4 G4 E4 D4 → recorded → *it loops back as played*.
2. PLAY stops → the key comes from the melody → *top line: E minor or G major; "?" when ambiguous*.
3. ALGORITHM to BASS; REC; play E4 under the motif (sounding E3) → *recorded; the melody unchanged*.
4. ALGORITHM to CHORDS: the pads offer the chords that fit the motif (Harmony Assist, *later*); pick one, hold it for the
   whole loop → *one chord recorded*.
5. ALGORITHM to DRUMS; record a beat → *four tracks play together*.

#### T-3 Chords first (U-3): chords, a bassline, motifs
1. CHORDS · PAD; K1 Neo soul, K2 ii-V-I; REC, PLAY; F5 NEXT three times, a chord a bar → Dm9 G13 Cmaj7 recorded →
   *the chords play back; their pads light full as they come round* **(built, tested)**.
2. PLAY stops → the key: C major → *top line C maj*.
3. ALGORITHM to BASS; while it plays, the root and 5th of the chord now light *(later)*; REC; play D4 G4 C4 (sounding an octave down) →
   *recorded, the chords unchanged*.
4. ALGORITHM to MELODY; play a motif over it → *recorded*.
5. Back to CHORDS · MANUAL: the recorded chords breathe as they play; SELECT walks the ideas **(built, tested)**.

#### T-4 Back and forth (U-4): change the song without losing it
1. Any song with chords and bass. Tap GLO → the song page: K1 key, K2 tempo, K3 length, K4 4/4 *(later)*.
2. K2 from 90 to 100 BPM while playing → it goes on from the same place, faster → *no note changes* (engine built, D-032).
3. K1 to a locked key → the pads move to it; the recorded notes stay; the key the notes suggest shows beside it *(later)*.
4. Go back to BASS, REC: play a new bassline over the old one → *the bass's notes are added; the chords unchanged* (built).
5. HOME held, choose Guided, then Producer again → *the song is still there* *(later)*.

#### T-5 The modes (U-4, U-5, U-6) (placeholders first)
1. Hold HOME → the picker: Producer, Guided, Game → *three names; the current one marked*.
2. Choose Guided → its screen: "Guided" and "coming soon"; the top line teal.
3. Choose Game → "Game", "coming soon", violet. Choose Producer → back to the song, blue, nothing lost.

### Guided (preliminary)

#### T-6 My first loop (U-5) (very easy)
1. Lesson 1: "A beat and one chord." The FM-1 plays a beat on the drums track (prepared).
2. "Press the lit pad." CHORDS · PAD opens; only I is lit; the FM-1 records the chord on the beat.
3. "Now try the next chord." NEXT lights; the step ends when two chords are recorded. Why: "the I chord is home".

#### T-7 A bassline in the key (U-5) (a bit harder)
1. "Turn on in key: every key sounds right." BASS opens with in key on.
2. "Play the root of each chord when it changes." The roots light while the song plays; the step ends after a loop.
3. "Now add a motif on the melody."

### Game (preliminary)

#### T-8 The unicorn jumps (U-6)
1. Hold HOME, Game; K1 a song (Twinkle), K2 the melody → the field, the unicorn, the next key breathing.
2. Play the breathing key → the unicorn jumps the fence; a wrong key → a soft bonk; the game waits (D-026).
3. The end → stars; HOME back to the songs.
