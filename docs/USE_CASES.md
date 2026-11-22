# fm1-scales — Use Cases

**How this fits together.** These describe *intent*: what a person is trying to do. `features/fm1-scales.feature`
turns intent into Given/When/Then scenarios, `@auto` where the factory can check them and `@manual` where only a person
with an FM-1 (or a child) can. `REQUIREMENTS.md` holds the numbered statements and the traceability. When a use case
changes, its scenarios change in the same pass.

**Where they come from.** The experience is drafted with the owner in the concepts in [`concepts/`](concepts/README.md) (experiences, controls, screens); once agreed, a concept's walk-throughs become the use cases here.

**Who writes them.** The product owner, with the requirements agent. Builder agents read them and never change them.

**Actors** (concept v0.5, the owner, 2026-11-08)
- *Producer*: makes their own songs, any age that reads; wants the tools open and the help on demand (Producer mode).
- *Beginner*: new to making music, an older child or a grown-up; wants to be led step by step (Guided).
- *Child*: plays along, may not read yet (Game).
- *Grown-up*: a parent or teacher who sets it up.
- *Builder*: us and the factory's agents, adding lessons, styles and sounds as data.

**Level.** These are the stakeholder-level scenarios: the journey, without keys. The user stories they serve are in
[`STORIES.md`](STORIES.md); the steps with keys are the Gherkin scenarios, which tag the use case they belong to.
(Way of working: [`WAY_OF_WORKING.md`](WAY_OF_WORKING.md).)

---

## UC-00 — The main scenario

**Make a song, starting anywhere, with as much help as you want** (concept v0.5, the owner, 2026-11-08). Someone has an
idea (a groove, a tune, a few chords) and turns it into a short song of chords, bass, melody and drums. They start on
whichever track has the idea; the FM-1 works out the key from what they record; each next track helps in that key, as
much or as little as they choose; what they record is kept as played; they can go back to any track, change the tempo,
the key or the length, and nothing they played is lost. Every other use case is a part of it, and a change that makes
UC-00 worse is wrong, however good it looks alone.

## Approved use cases (the owner, 2026-11-08, from concept v0.5)

### UC-01 — Groove first
**Stories:** ST-01, ST-02, ST-03, ST-04, ST-11. **Actor:** Producer.

Ana has a rhythm in her head. She records a simple beat on the drums, then plays a bassline over it, freely. When she
stops, the FM-1 tells her the key it hears in the bass. She goes to the chords: the pads are now that key's chords,
and the bassline shows as it plays, so she finds chords that sit on it, one finger at a time, and records them. Last,
a short motif on the melody, with the key's notes lit so every note she plays fits.

### UC-02 — Melody first
**Stories:** ST-01, ST-02, ST-10. **Actor:** Producer.

Leo whistles a tune. He plays it on the melody track and records it. The FM-1 works out the key, or says it is not
sure yet. He adds a single bass note under it, then asks for chords that fit the melody and holds one chord for the
whole loop. A beat on the drums finishes the idea.

### UC-03 — Chords first
**Stories:** ST-01, ST-03, ST-04. **Actor:** Producer.

Mia picks a style she likes and plays its progression with one finger on the chord pad, adding a colour here and there,
and records it as the loop goes round. On the bass she sees the root of each chord light as it comes, so her first
bassline follows the chords; later she plays freely around it. She ends with a few motifs on the melody, and goes back
to manual chords to change one chord by hand.

### UC-04 — Change your mind
**Stories:** ST-02, ST-04, ST-06. **Actor:** Producer.

Halfway through, Ana wants it faster and in another key. On the song page she turns up the tempo while it plays; the
loop goes on from the same place. She locks a key; the pads move, her recorded notes stay as played, and the FM-1 shows
the key the notes suggest beside the one she chose. She re-records the bass over the old one, makes the loop shorter to
work on two bars, and switches to another mode and back: the song is still there.

### UC-05 — Learn step by step
**Stories:** ST-07, ST-08. **Actor:** Beginner.

Sam has never made a song. He chooses Guided. The FM-1 plays a beat and asks him to press the lit pad; it records his
chord and explains why it sounds like home. Step by step he adds a second chord, a bassline that plays the roots, and
a motif, each step opening the page it needs with the right help. Later lessons leave him more freedom.

### UC-06 — Play along
**Stories:** ST-07, ST-09. **Actor:** Child.

Noa, six, chooses Game and a song she knows. A unicorn runs across a field; the next key breathes. Each right key
makes the unicorn jump a fence; a wrong one gives a soft bonk and the game waits for her. She can't read yet, and
doesn't need to.

### UC-07 — Just play
**Stories:** ST-03, ST-04. **Actor:** Producer.

Someone wants the FM-1 as a plain instrument: four tracks, any key, no help. They pick each track's sound, play and
record freely, and use the help only when they want it.

### UC-08 — Name what I play (proposed, 2026-11-12: to confirm)
**Stories:** ST-14. **Actor:** Producer, or anyone learning chords.

Leo finds a chord by ear on his MIDI keyboard and wants to know what it is. The FM-1 starts on CHORDS · PLAY; he
holds A, C and E and the screen says **Am**, the vi of the key. He adds G and B: **Am9**. He tries three notes side by
side: there is no such chord, so the screen shows the notes, and under them the closest chord with the notes it is
missing. He records the Am9 he liked, then goes to IDEAS to see which chord could come next.

### UC-09 — Play with other gear (proposed, 2026-11-12: to confirm)
**Stories:** ST-15 to ST-22, ST-24. **Actor:** Producer.

Ana plugs the FM-1 into her computer and her MIDI keyboard into the FM-1's jack. Her computer's MIDI clock starts the
FM-1's song in time; or the FM-1 leads, and her drum machine follows its clock. Her keyboard plays the track she
chose, its pitch bend and sustain pedal work, and what she plays goes out to the computer on each track's channel.
Before she records, the FM-1 counts in a bar with its click. When she stops for a while the screen goes dark, and the
next note wakes it.

### UC-10 — Make a beat that grooves (approved by the owner, 2026-11-13)
**Stories:** ST-30, ST-31, ST-32, ST-34, ST-38. **Actor:** Producer.

Ana wants a Dilla beat. On the drums track she holds the first pad on AUTO: a Dilla beat on the dusty kit, the kick
swung hard and the snare late. She turns on the fills, then goes to PLAY and plays her own hats over it, holding one
pad with REPEAT on sixteenths. She records a version of her own, opens the REC layer, sets the grid to sixteenths and
the feel to Dilla, and watches each hit move off the grid on the screen. MPC on the FX makes it hit hard.

### UC-11 — Write a melody over my chords (approved by the owner, 2026-11-13)
**Stories:** ST-26, ST-27, ST-28, ST-29, ST-49. **Actor:** Producer.

Leo has chords. On the melody track the keys light the key's pentatonic: nothing clashes. He turns HELP to Chord: the
notes of the chord now glow and change with each chord, and the note he plays shows green when it lands well, blue
when it moves, amber when it pulls. He tries other scales: the pop-up says which fit his chords and which lack a note
of them. He adds a third above with PLUS, records a phrase, and ends it on a green note.

### UC-12 — Learn a part slowly (approved by the owner, 2026-11-13)
**Stories:** ST-27, ST-33, ST-42. **Actor:** Learner (any age that reads), Producer.

Mia wants to play a hook and a beat she heard. On IDEAS she sets SPEED to 70 %: the whole song slows. The idea plays
once, then it is her turn: its notes fall onto the keys she must play. She raises the speed step by step to 100 % and
records it; at full tempo the recording fits the song.

### UC-13 — Fix a take (approved by the owner, 2026-11-13)
**Stories:** ST-40, ST-41. **Actor:** Producer.

Sam recorded a bassline with one wrong note and a sloppy start. He holds REC: the track's REC layer. He sets the grid
to sixteenths (what he played is kept; only the playback moves) and a little swing. He presses EDIT and holds the
wrong key while the song plays past it: its notes go. One too many: SAVE undoes it. Later he clears the melody track
from the same layer (it asks first) and records it again after a count-in, 4, 3, 2, 1.

### UC-14 — Build my own kit (approved by the owner, 2026-11-13)
**Stories:** ST-35, ST-38, ST-39. **Actor:** Producer.

Noa likes the 808 kit but wants a dusty snare and a conga. She holds the snare's key and turns PRESETS through every
kit's sounds until "snare2" of the dusty kit; then a conga on another key. The top line says My kit. She tunes the long
808 kick: the screen names its note, and she sets it to A1, the song's key. Later she puts her own WAV files, with
their licence, in the user kit's folder: after the next install they are on the keys, named as their files.

### UC-15 — Bring my own ideas (approved by the owner, 2026-11-13)
**Stories:** ST-45. **Actor:** Producer, Builder.

Leo has MIDI files of grooves he loves (his own, or ones he may use). He drops them in the folders of the templates,
melody, bass, chords or drums, by style, with their licence. After the build they are ideas he can learn on IDEAS and
patterns he can hold on AUTO, in any key: they are stored relative to their own key and fit the song's.

### UC-16 — Arrange with the tracks I need (approved by the owner, 2026-11-13)
**Stories:** ST-36, ST-37. **Actor:** Producer.

Ana wants two chord parts, an arpeggio and plain chords, and a melody she plays with four fingers. On GLO's TRACKS she
makes track 4 a second CHORDS track. While the song plays, everything sounds; when she plays a big chord on top, the
FM-1 is full for a moment, an amber dot shows it, and TRACKS shows the voices in use and the CPU. (Built: D-046, D-047.)

### UC-17 — Make a song in one style (approved by the owner, 2026-11-13)
**Stories:** ST-28, ST-32, ST-48. **Actor:** Producer.

Mia wants shatta. She chooses Shatta on the drums (an 808 four-on-the-floor dembow with clipped rims and triplet
hats), shatta chords on the chord pad (dark, a half step apart), a sliding 808 bassline on the bass, and the "de
notes" phrase on the melody. Every track speaks the same style.

### UC-18 — A child plays Unicorn Jump (draft, from the owner's game design, 2026-11-14)
**Stories:** ST-51. **Actor:** Child (four and a half, does not read yet), with a parent.

She holds HOME, a parent picks Game, and a white winged unicorn waits in a night meadow while every key breathes.
She presses a key and it runs to the first fence and waits. In Easy she presses any key: it jumps and a bell sings
C, and fence after fence, at her own pace, the jumps play Twinkle Twinkle. Later a parent turns K1 to Key: now the
key to play lights; a wrong one makes the unicorn bump the fence, and two make it poop, and she laughs; the poop comes
round again and she hops it. At the end of the song it flies away to the moon, round it and back, leaving a rainbow (in Key mode once eight good jumps have earned its wings). A soft band plays the song's chords under her notes. Nothing ever ends.

### UC-19 — A grown-up learns to make a beat in the guided mode (draft, from the beat-guide lab the owner shaped, 2026-11-16)
**Stories:** ST-57 to ST-66. **Actor:** A grown-up who never made a beat and reads only the screen.

He holds HOME and picks Guided. Lesson 1 opens: "Press PLAY", in big amber letters, and only the chords sound. Step by
step the screen says what to do (turn K1 up: the drums come in; K2 the bass; take the chords out and back), and each
time he does it a green tick and a chime say so, and SELECT takes him on. He hears the kick, the snare and the hats
alone on three lit keys, tries drum parts and styles on PRESETS, the tempo on ALGORITHM, the key on OCT, and plays a
melody of his own on the lit keys while the band plays; a key outside the key is named and he hears why it rubs. The
next lessons take him through tempo, groove, chords, bass and melody to a beat of his own, recorded. HOME tapped
shows the lessons; the FM-1 remembers where he was.


### UC-20 — Make a whole song from sections (draft, from the Sequencer Lab the owner played, 2026-11-19)
**Stories:** ST-85 to ST-91. **Actor:** Producer.

Ana has a 2-bar loop she likes: sequence 1. She holds its key and taps an empty one: sequence 2 is a copy, and she
records a busier chorus over it, undoing a bad take with SAVE held and wiping the melody with EDIT + REC when it went
wrong. On SONG she adds 1, 1, 2, 1 and a quiet 3 she draws on STEPS, kick on beat 1, a chord on beat 3. While it plays
she taps 2 early to try the chorus sooner, sweeps the filter on the sound page with REC on (it stays in the sequence),
and holds FX to stutter the melody while the drums keep going. She switches off; tomorrow the song is still there.
