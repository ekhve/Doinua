# fm1-scales — User manual

**For:** anyone playing the FM-1 with the companion firmware (`apps/companion`), children with a grown-up, and
producers. **Version:** the chord pages PLAY, IDEAS and AUTO (D-042, D-075..D-080), the bass (D-040, D-041). Where this manual and the firmware disagree, the firmware is wrong: tell us in a Problem
report.

At startup, Companion briefly shows the Doinua artwork and the firmware’s build revision. Pressing a control brings up the normal page immediately.

## 1. The idea

The FM-1 holds one **song**: up to four tracks (CHORDS, BASS, MELODY, DRUMS) that loop together. You play a track,
record what you play, and play the next track over it. The FM-1 helps you with chords: which chords belong to the key,
which one could come next, and how to give them colour.

## 2. Getting around

| Control | Does |
|---|---|
| **ALGORITHM** | Picks the track: CHORDS, BASS, MELODY, DRUMS. The track's name is at the top left, in its colour |
| **SCL** | The track's pages, in turn: **PLAY** (you play, the FM-1 helps), **IDEAS** (it suggests), **AUTO** (one finger does more). Its light is on away from PLAY |
| **PLAY** | Plays the song, looping; again stops it. Its light is on while it plays |
| **REC (tap)** | Arms the track for recording: its light breathes, and is full while the song plays and records. Again: stops recording. While stopped with a COUNT-IN, it counts in and records at once |
| **REC (hold)** | The track's **REC layer**: GRID, FEEL, CLICK, COUNT-IN, and clearing the track (section 2.2) |
| **EDIT + key** | Erases that key's notes of the track as the song plays past them (section 2.2) |
| **SAVE (hold)** | Undoes the last recording, erase or clear (section 2.2). Tapped, it shows HOLD and undoes nothing |
| **PRESETS** | The track's sound, from the whole list |
| **OCT− / OCT+** | One octave down or up, two at most each way |
| **GLO (tap)** | The song page, **SONG**: K1 the key (AUTO, or lock one of the 24), K2 the tempo (40–240 BPM), K3 the length (1–8 bars), K4 4/4. It shows the key big, the loop's length, and the four tracks as you played them. Nothing you played changes. **SCL** steps to the pages behind it: **TRACKS**, **CLICK**, **MIDI**, **SYSTEM** (section 2.1). GLO again: back |
| **GLO (hold)** | Help for the page you are on. Let go to go back |
| **HOME (tap)** | Back to the track page, closing whatever is open over it: the song page, the REC layer, a question |
| **HOME (hold)** | The modes: Producer, Guided (the lessons, section 6g), Game (the games, section 6c). Turn SELECT to choose, press HOME, and HOME again to confirm (any other button: back). Your song is kept. The mode's colour is the line at the very top |
| **MASTER** | Volume (the FM-1 never goes above a safe level) |

### 2.1 The settings behind the song page (GLO, then SCL)

GLO opens the song itself; the rest waits one press of SCL away, on tabs at the bottom. The FM-1 keeps these settings
when it is switched off.

| Page | Knobs |
|---|---|
| **TRACKS** | K1 to K4: each track's template, **CHORDS**, **BASS**, **MELODY** or **DRUMS**. Two tracks may share one (MELODY 1, MELODY 2). A track that has notes asks first: SAVE changes it, any other button keeps it; the notes stay. The help on every track listens to the other tracks' notes, whatever their template. At the bottom: the voices in use (up to 12) and the CPU they take |
| **CLICK** | K1 **CLICK**: Off, **Rec** (only while REC is on), **On** (always while the song plays). K2 **LEVEL** 1–10 (turning it plays a click). K3 **COUNT-IN**: Off, 1 or 2 bars: REC while stopped first counts a bar of clicks (the beats left show big and red, PLAY breathes), then the song starts and records; with REC already on, PLAY does the same. PLAY during the count-in stops it. The click is never recorded |
| **MIDI** | K1 **TRACK**, K2 its **CHANNEL** (1–16; the list shows all four). K3 **IN**: **Channels** (each channel plays its track), **This track** (every channel plays the track on screen), **Follow** (each channel plays its track, and the screen goes to it). K4 **WAKE**: Panel (only a key or a knob wakes the screen) or +MIDI (a MIDI note too). What you play goes out on these channels |
| **SYSTEM** | K1 **SCREEN OFF**: Never, 1, 5, 15, 30 or 60 minutes with nothing played, then the screen goes dark; the sound, the song, MIDI and the lights go on. The key, button or knob that wakes it does nothing else; a MIDI note that wakes it plays. K2 **TUNE**: −50 to +50 cents (A4 between 427 and 453 Hz); the drums do not change. K3 **EXPLAIN**: On (turning a knob shows, for under two seconds, what its new value does) or Off (as it starts) |
| **USB** | **USB AUDIO**: the FM-1 is a sound card for a phone, an MPC or a computer, both ways, over its USB cable (16-bit stereo, 44.1 kHz; no driver needed). **OUT**: what you hear goes to the phone, to record it. **K2 MONITOR**: when you hear what the phone plays into the FM-1: **off**, **sampling** (only on the sampler's TAKE and CHOP pages), **on** (always: play along). **K3 IN LEVEL** −12 to +24 dB. The page says whether a phone or computer has the FM-1's audio open. |

**The screen** has four parts, top to bottom:
1. **The song**: the track and the page, the key, the tempo, the octave, a red dot while REC is on, an amber dot for
   two seconds when the FM-1 was full and a note took another's voice, and the loop as a
   line: what the track has recorded, where it was played, and the playhead while the song plays. The colour of the
   very top line is the mode (blue: Producer).
2. **The knobs**: four cards, one above each knob, with what it sets.
3. **The chord**: big, with what it is.
4. **The keys**: on chord AUTO, the pads; elsewhere a piano.

**The song** is 4 bars of 4/4 at 90 BPM. Its key is **AUTO**: worked out from what you have recorded (C major while
the song is empty). The pads take the key when the song stops, so they never change while you play. Nothing is kept
when the FM-1 is switched off yet.

**Recording** keeps what you play exactly as you played it: when, how long, how hard. Nothing is moved onto the beat.
Press REC, then PLAY, and play. Press REC again to stop recording; the song keeps playing.

The FM-1 starts on **CHORDS · PLAY** (section 3): play some notes and it names the chord.

### 2.2 Recording: the REC layer, erasing and undo (every track)

GRID decides where what you play lands as you record it; FEEL decides how it plays back. Track by track.

**Hold REC** (half a second): the REC layer opens over the page. **REC** again closes it.

| On the layer | Does |
|---|---|
| **K1 GRID** | **off** · **1/8** · **1/16** · **1/32** · **1/8T** · **1/16T**: each note you record lands on the nearest place of the grid. **off**: kept as you played it. It changes what you record from now on, not what is there |
| **K2 FEEL** | How the track plays back, from where each note was recorded: **Straight**, **Swing 55**, **Swing 62**, **MPC 66** (the off-sixteenths late), **Dilla** (kick swung hard, snare late, hats loose), **Laid back**, **Pushed**, **Dembow**, **Shatta**, **Drunk**. On a drums track each drum keeps its own part of the feel (C kick, D and E snare, F to B hats) |
| **K3 CLICK** | Off, **rec** (while recording), **on** (always) |
| **K4 COUNT-IN** | Off, 1 or 2 bars |
| **OCT+** | Clear this track: it asks first. **OCT+** again clears it; any other button keeps it |

**Erase**: with **REC on** and the song playing, hold **EDIT** and the key whose notes you want gone: each of that
note's notes on this track (in any octave) is erased just before it would sound. Let go when they are gone. With REC
off nothing is erased, and the screen says **REC ON to erase**.

**Undo**: hold **SAVE** (half a second) to undo the last recording pass, erase or clear, up to four back. A tap shows
**HOLD: UNDO**. **Redo**: tap SAVE twice quickly (within 0.4 s) and what you undid comes back. A pass where you played
nothing is not counted.

**Practising slower**: on a melody's IDEAS page, **SPEED** slows the whole song (the top line shows, say, **63 of
90**). What you record then lands where it belongs in the song, which plays at its own tempo again when SPEED is back
at 100 %.

## 3. Chord play (CHORDS · PLAY): what am I playing?

The keys play notes like a normal keyboard, and the FM-1 names the chord you hold. A MIDI keyboard on channel 1 counts
too: its notes are named with the keys'. Everything you play is recorded as played.

| You hold | The screen shows |
|---|---|
| A, C, E | **Am**, and **vi** (its place in the key); under it the notes, A C E |
| then add G and B | **Am9**, vi; A C E G B |
| a chord with a note outside the key | its name, and **outside** |
| one note | its name and octave (C4) |
| two notes | C + G |
| notes that make no chord (C C# D) | the notes, smaller; under them **closest** and the chord they are nearest to, with how many of its notes are missing (C E B: closest Cmaj7, 1 missing), or **no chord** |

| Knob | Does |
|---|---|
| **K1 HELP** | Four steps, from no help to the most: **Free**, **Key**, **Shapes**, **Pads** (below) |
| **K2 TRANSPOSE** (Free, Key) | **whites**: the white keys play the song's key, a major key from C, a minor key from A. In E-flat major, C E G plays E-flat G B-flat |
| **K2 PROGRESSION** (Shapes, Pads) | the progression to play: the styles' progressions, one after the other |
| **K3 COLOUR** (Key, Shapes) | none, sus2, sus4, 6, 7, maj7, 9, add9, 11, 13, dim: the chords suggested get it. 7 is the key's own seventh (G7, Dm7, Cmaj7) |
| **K4 VOICING** (Shapes) | Root, Inv 1, Inv 2, Open, Wide |
| **SELECT** | Key: another chord with the notes you hold. Shapes: the next chord |

The HELP steps:

- **Free**: only what you hold lights.
- **Key**: the key's notes are dim and its home note half lit. Hold one note: the rest of a chord of the key with it lights full, and the screen says which (an arrow, then Am); SELECT gives the next chord that has your note (Am, F, Dm). The key's seven chords sit in a row on the screen: the one you hold is lit, the ones your note fits are half lit, and when you let go, the chords that usually come next.
- **Shapes**: the progression's chord lights full and its next chord half. Play it; the screen ticks it and shows the next chord. It starts on **5 Pop I-V-vi-IV**. SELECT moves on.
- **Pads**: each white key plays a chord of the key by its letter, in every key: **C** is I, **D** ii, **E** iii, **F** IV, **G** V, **A** vi, **B** vii, in the octave of the key you press. Hold a black key for a colour, then press a pad: F#3 sus2, G#3 sus4, A#3 6, C#4 7, D#4 maj7, F#4 9, G#4 add9, A#4 11, C#5 13, D#5 dim. **F5 NEXT** plays the progression's next chord, **G5 AGAIN** the last one again, **F#5 LOCK** keeps the colour held on every pad (again: off). The screen draws the keys with what each plays; the next chord's pads are half lit. K2 PROGRESSION, K3 VOICING, K4 FEEL.

**FEEL** (Pads) is how the hand plays the chord: **Tight** (together), **Pianist** (the bass first, the inner notes a little after and softer, the top note sings), **Laid back** (the whole hand a little late), **Rolled** (a harp, getting louder), **Strum up** and **Strum down** (a guitar, low to high or high to low), **Sloppy** (anyone's first try).

**PROGRESSION** (Shapes and Pads): turning it opens a numbered list with each progression's style and numerals (ii9 V13 Imaj7). First **None** (no progression: NEXT plays the last pad again), **Mine** (the last eight chords you played on the pads), **Follow bass** and **Follow melody** (the key's chord on the bass or melody recorded at that moment), then the styles' progressions (Pop to Rock) and Sketchpad's chord sets.

### Try this

1. Switch on: CHORDS · PLAY. Hold **A3 C4 E4**: Am. Add **G4** and **B4**: Am9.
2. Hold three keys side by side: the notes, and "no chord".
3. Plug in a MIDI keyboard and play a chord on channel 1.

## 4. Chord auto (CHORDS · AUTO): the song's parts

**SCL** to AUTO. The white keys are the parts of a song in the style: press one and it plays, a chord a bar, by the
style's player. The low white keys play each part **calm**, the high ones with **energy** (busier, louder):

| Keys | F3 / F4 | G3 / G4 | A3 / A4 | B3 / B4 | C4 / C5 | D4 / D5 | E4 / E5 |
|---|---|---|---|---|---|---|---|
| Part | INTRO | VERSE | PRE (pre-chorus) | CHORUS | BRIDGE | BREAK | OUTRO |

Stopped, a part's key starts the song with that part. Playing, another part's key waits for the next bar (its key lights
half until then). Each part is 2 or 4 bars: one of the style's progressions, or a piece of one (the intro its first two
chords, the break one chord, the outro ends home on I).

Hold a black key to move the part while you hold it: **F#** fill (busy, the last beat in sixteenths), **G#** build (busier
and louder, up an octave after two seconds), **A#** half time, **C#** an octave up, **D#** stop. Both octaves do the same.

| Knob | Does |
|---|---|
| **K1 STYLE** | the 13 styles, each with its own three players (Neo soul: neo soul, pads, ballad; Funk: funk, house, pop piano; ...) |
| **K2 DENSITY** | sparse, normal, busy |
| **K3 SWING** | straight, light, medium, heavy |
| **K4 FOLLOW** | **off**, or **bass** / **melody**: the parts play the key's chord on the bass or melody you recorded, at each moment |

With **REC** armed, what AUTO plays is recorded on the track.

### Try this

1. SCL to AUTO, Neo soul. Press **G3**: the verse starts. Press **B4**: the chorus comes in on the next bar, with energy.
2. Hold **G#4** for a few seconds: the build. Let go on the bar and press **C4** for the bridge.
3. Record a bass line, then K4 FOLLOW to bass: the parts play on your bass.

The chord pad that was here (the key's chords twice over the white keys) is retired (#108): it is PLAY's HELP **Pads** now,
with C = I in every key.

## 5. Chord ideas (CHORDS · IDEAS): play along with a progression

**SCL** from PLAY to IDEAS. An idea is a style's progression with a rhythm, two or four bars long. Press **PLAY**: the
idea plays over the song (heard, never recorded), the chord to play now lights full on the keys, and the next one lights
half for the whole bar before it comes. Play along on the keys; the screen ticks it when you hold the chord.

| Control | Does |
|---|---|
| **K1 STYLE** | Pop, Neo soul, Hip hop, West coast, Funk, Electronic, Jazz, Reggae, Afrobeats, Shatta, Reggaeton, Drum & bass, Rock |
| **K2 IDEA** | each of the style's progressions in its two rhythms (bar, push, halves, hip hop, stabs): "1 / 6" |
| **K3 SPEED** | the whole song slower while you practise; back to full when you leave IDEAS |
| **K4 HEAR** | **on** (the idea plays every time), **once** (the first time round, then it is your turn), **off** |
| **SELECT** | the idea's next or previous chord, lit; stopped, you hear it once |

How long each chord lasts: one chord fills 4 bars; two chords go A B A B; three go A B C-C (the last one held for two
bars); four get a bar each.

### Try this

1. SCL to IDEAS: Neo soul, idea 1 (ii-V-I, push). PLAY: Dm9 lights, G13 half. Play the lit keys.
2. K4 HEAR to once: the first time round you listen, then it's your turn.
3. K3 SPEED to 70 %: practise slower; SCL away and it is back to the song's tempo.

The manual chord builder that was here (K1 root, K2 type, K3 colour, K4 voicing) is retired (#108): build chords on
PLAY with HELP Key or Shapes, K3 COLOUR and K4 VOICING.

## 6. The bass (BASS)

ALGORITHM to BASS. Three pages, on **SCL**: **PLAY**, **IDEAS**, **AUTO**. One scale for the whole track: what PLAY
lights, what the white keys play on Whites, and what IDEAS and AUTO keep their notes inside. It sits on the song's key
(GLO).

### PLAY: you play, with as much help as you like

The keys play the bass an octave down (OCT moves it).

| Knob | Does |
|---|---|
| **K1 HELP** | **Free** (no lights) · **Key** (the scale dim, its tonic breathing) · **Key+root** (while the song plays, the root of the chord recorded at that moment lights full over the dim scale: the safe note) · **Whites** (the white keys play the scale from its tonic, so nothing can sound wrong; the black keys rest) |
| **K2 SCALE** | With chords recorded: every scale, in the order it fits your chords (best first), and a pop-up lists the ones around your choice with how much of the chords each holds. Without chords: the scales that suit the key, its own first. Minor: natural minor, minor pentatonic, blues, dorian, phrygian, harmonic minor, melodic minor, hirajoshi, locrian. Major: major, major pentatonic, major blues, mixolydian, lydian, lydian dominant, phrygian dominant, whole tone |
| **K3 OUTSIDE** | A note outside the scale: **play**s, plays **soft**ly, or is **mute** |
| **K4 PLUS** | **off**, **octave** or **fifth**: a second note above each one you play |

With HELP **Free** the scale does nothing, so SCALE and OUTSIDE are greyed; turning them says so.

### IDEAS: learn a style's bassline

A style's line plays over your chords (or over the key's I-V-vi-IV when there are none). Its notes fall onto the keys
they belong to and land on their time; the next key breathes. Play each note as it lands; REC keeps what you play, not
the idea.

| Knob | Does |
|---|---|
| **K1 STYLE** | G-funk, Boom bap, Latin, Disco, Blues, Shatta, Jazz walk, Reggae, Afrobeats, Reggaeton, Drum & bass |
| **K2 LINE** (or SELECT) | The style's lines |
| **K3 SPEED** | 50 to 100 %: the whole song slows while you learn, and is back when you leave |
| **K4 HEAR** | **listen/play** (the line plays one time round, then it is your turn), **on**, **off** |

### AUTO: one finger, a whole bassline

The white keys are the key's degree pads (as on the chord pad). Hold one: the song starts and the style's pattern plays
on that root; let go and it stops. Move between pads with your chords. REC records what it plays.

| Knob | Does |
|---|---|
| **K1 STYLE**, **K2 PATTERN** | The pattern (tresillo, octaves, boogie, ...) |
| **K3 FEEL** | Straight or swing |
| **K4 FOLLOW** | **chords**: with the song playing, the bass follows the recorded chords by itself |

## 6b. The melody (MELODY)

ALGORITHM to MELODY. Like the bass: **PLAY**, **IDEAS** and **AUTO** on **SCL**, one scale for the track, on the song's
key. Every track opens on PLAY.

### PLAY: play a tune, see what it does over the chords

The keys play at their own pitch. The notes you hold are named on the screen (no octave), and three or more that make a
chord show its name too.

| Knob | Does |
|---|---|
| **K1 HELP** | **Free** (only what you play lights) · **Key** (the scale dim, its tonic breathing; the key's pentatonic first) · **Chord** (while the song plays, the chord sounding lights full over the dim scale; the note you play is named green in the chord, amber outside the scale) · **Whites** (the white keys play the scale) |
| **K2 SCALE** | The scales for your chords, best first: each says **fits**, or which of the chords' notes it **lacks** (up to two). Without chords: the key's own, its pentatonic first |
| **K3 OUTSIDE** | A note outside the scale **play**s, or is **mute** |
| **K4 PLUS** | **off** · **3rd** or **6th** (a second voice above, in the key; the screen shows it, "+E") · **oct** · **echo** (two softer repeats, a dotted eighth apart) |

With HELP Free, SCALE and OUTSIDE are greyed and turning them says why.

### IDEAS: learn a figure

A figure from Sketchpad plays over your chords (or the key's I-V-vi-IV); its notes fall onto their keys and the next
one breathes.

| Knob | Does |
|---|---|
| **K1 STYLE** | Hip hop, Soul, Funk, House, West coast, Cinematic (Sketchpad's styles, each with its figures) |
| **K2 IDEA** | The figure (the screen's IDEA shows figure.version, "1.2") |
| **SELECT** | Another version: the same rhythm, other notes |
| **K3 SPEED** | 50 to 100 %: the whole song slows while you practise (the top line shows the tempo), and is back when you leave |
| **K4 HEAR** | **listen/play** (one time round, then your turn), **on**, **off** |

### AUTO: hold a pad, a phrase plays

The white keys are the degree pads. Hold one: the song starts and the style's phrase plays over that chord, its call one
bar and its answer the next, every note in the scale; let go and it stops.

| Knob | Does |
|---|---|
| **K1 STYLE**, **K2 PHRASE** (or SELECT) | Pop, G-funk, Ghibli, Neo soul, Reggaeton, Shatta, Afrobeats, Reggae, Cinematic, Boom bap, Disco, Drum & bass, and their phrases |
| **K3 FEEL** | Straight or swing |
| **K4 FOLLOW** | With the song playing, the phrase follows the recorded chords |

## 6c. The game: Unicorn Jump (HOME held → Game)

For small players, at their own pace. A white winged unicorn waits in a night meadow and every key breathes: press
any key and it runs. Fences come, and the unicorn **waits in front of each one until she
plays**: then it jumps high over it and the fence's note rings, so the jumps play the song. Play early and it dashes
to the fence. Under the bells a soft pad and a bass play the song's chords, with a reverb (K4 turns them off). In Key
mode a wrong key makes it bump the fence (stars!) and wait; two wrong in a row and it poops (*pfrrt ... plop*; the
top line counts them), and the poop comes back later in a fence's place: it waits in front of it, any key hops it.
At the end of the song (in Key, once it has earned its wings with eight good jumps in a row: WINGS!) it flies away to the moon, smaller and smaller, goes round it and comes back, leaving a rainbow across the sky, and
the song starts again when it lands. Left waiting, it neighs after 8 seconds and lies down asleep after 20 (z z Z);
any key wakes it. The song done: YAY, and it starts again. It never ends.

| Knob | Does |
|---|---|
| **K1** | The mode: **Easy** (plain fences; any key jumps and the song still plays), **Key** (each fence's note as its letter in its colour, and its key lights: play that one) |
| **K2** | How fast the unicorn runs: slow, medium, fast |
| **K3** | The song: Twinkle Twinkle, Mary's Lamb, Frère Jacques, Row Your Boat, Ode to Joy, Happy Birthday, and the Unicorn Song (the game's own) |
| **K4** | The music under the bells (a pad, a bass, the reverb): on (up) or off (down) |

**Teach the unicorn a song.** Turn K3 past the built-in songs to MY SONG 1 to 4 (an empty one says TEACH ME) and
press **REC**: the unicorn stops running and listens, swaying, and dances a little hop for each note you play, the
note floating up to it. Play the tune on the keys, or play it into the FM-1 over MIDI from a computer or a phone (any
MIDI file or player works; notes too high or too low are moved into the keys). Only the notes count, not their
rhythm: the unicorn waits for each jump anyway. **K1** back takes back the last note. **SAVE** (or REC again) keeps
it, up to 120 notes: LEARNED! Holding HOME while it listens keeps nothing. Taught songs stay on your FM-1; the pad
and bass play them in their own key. The seven built-in songs can't be taught over.

Hold HOME to go back to Producer: your song is kept.

## 6d. The game: Hiru Damatxo (Game, SELECT to the next game)

In the Game mode, **SELECT** turns from one game to the next: Unicorn Jump, Hiru Damatxo, Aldapeko Sagarra.

Three little ladies of Donostia in the dance costume wait on La Concha's promenade at sunset. Play the song
"Donostiako hiru damatxo" (traditional): each note makes the next one jump; at the end of each line all three jump,
and at the end of the song they turn together and fireworks burst over the bay: ZORIONAK! In Key mode a wrong key
makes the dancer sit down hard (the others giggle); two in a row and a seagull flies in and poops on her head.
**PLAY** plays the song first, danced. **K1** Easy or Key, **K4** the music under the flute.

## 6e. The game: Aldapeko Sagarra

A robin sings from the top of an old apple tree on a hillside in the golden late afternoon, above a baserri whose
windows light up and whose chimney smokes. Play "Aldapeko sagarraren" (traditional): the robin sings each note an
octave higher. In the last part ("nork dantzatuko ote du soinutxo hori?", who will dance to that little tune?) the
sheep dance; at the end, TXIRULIRULI! In Key mode a wrong key drops an apple on a sheep's head; two in a row and the
sheep poops while the robin hides its eyes. **PLAY** plays the song first. **K1** Easy or Key, **K2** the robin's
voice (flute, music box, bird whistle), **K4** the music.

## 6f. The game: the Night Train

Old Number 7 steams across the desert at night under a huge moon, a puppy riding on the logs in its tender. Each note
you play is a chuff: the wheels turn, the engine moves on and a puff in the note's colour goes up. At the end of each
line the whistle blows (choo choo), the bell rings and the coyote howls along. Two songs on **K3**: "I've Been Working
on the Railroad" and "Funiculì, funiculà". In Key mode a wrong key makes the engine cough black smoke; two in a row and
it sneezes, ah... ah... CHOO!, while the puppy barks and the driver shakes his fist. Eight right notes in a row make
the golden super train. At the song's end the train races to Star Station, where the sheriff throws it a star; the
train keeps its stars. Along the line: an owl on the telegraph poles, a ranch, a camp, a longhorn, prairie dogs, a
jackrabbit racing you, a canyon bridge now and then, and cacti that bloom in the lamplight near the end. **PLAY**
plays the song first. **K1** Easy or Key, **K2** the voice (reed, pluck, organ), **K4** the music.

**In every game, PLAY** plays the song by itself (in Unicorn Jump, the unicorn runs it and jumps every fence) and each key lights as its note sounds, so you can see the song before you play it. PLAY again stops it.

## 6g. The guided mode: lessons (HOME held → Guided)

For a grown-up who never made a beat. The screen says what to do, a step at a time, and the FM-1 remembers where you
were.

| Control | Does |
|---|---|
| **SELECT** | The next step, or back one |
| **HOME (tap)** | The lessons: SELECT chooses, HOME opens. Lesson 1 for now; the others are coming |
| **PLAY** | The lesson's song, looping; again: stop |
| **K1 to K4** | The mixer: the drums, the bass, the chords, the melody, 0 to 10 |
| **ALGORITHM** | The tempo, 60 to 180 BPM |
| **PRESETS** | The drums: three parts, then five styles (each with its own speed) |
| **OCT− / OCT+** | The key: the whole song up or down |
| **C4, D4, E4** | On the drums step: the kick, the snare, the hat, on their own |

**The screen**, top to bottom: the lesson and the step (3 / 14); **what to do**, the biggest text, in amber, and when it
is done a green tick, "Done!" and a chime; the step's words; the song's sequencer (kick, snare, hats, bass, chord,
melody) with the rows the step is about lit; the tempo, the key and the drums; the four knobs. A red line is a
warning: a key outside the key, or REC (which comes in lesson 2).

**Lesson 1, What's in a beat** (14 steps): the four tracks; the mixer, one knob at a time; the chords out and back;
the drums alone on three keys; drum parts and styles; the tempo; the key (HOME and its family); OCT; then you play the
melody on the lit keys; then your own mix.

## 7. With other gear (MIDI)

The FM-1 talks MIDI over USB (in and out) and on its TRS jack (in only: the jack can't send).

| Comes in | Does |
|---|---|
| Notes | Each track's channel (GLO, SCL to MIDI): at first 1 the chords, 2 the bass, 3 the melody, 10 the drums. On CHORDS · PLAY they are named |
| **Start / Continue / Stop** | Play the song from the top, play on from where it stopped, stop it. While the other gear's clock runs, the song follows it exactly, and the top line shows its tempo. PLAY on the FM-1 takes the lead back |
| **Pitch bend** | Two semitones up or down, on the channel's track |
| **Mod wheel** | A vibrato, up to half a semitone |
| **Sustain pedal** | Notes keep sounding after you let go, until the pedal lifts |
| **All notes off** | Silence on that track (reset controllers puts bend, vibrato and pedal back) |

| Goes out (USB) | When |
|---|---|
| Notes | What you play, on the track's channel (the drums on 10) |
| **Start, clock, Stop** | PLAY sends Start and then the clock at the song's tempo (24 a beat), so a computer or a drum machine follows; stopping sends Stop |

## 8. The drums (DRUMS)

ALGORITHM to DRUMS. Every key is its own sound: **27 a kit**, laid out on the **drum map**, the same on every kit. The
first octave, **F3 to E4**, holds the 12 sounds AUTO and IDEAS play, as on General MIDI and the MPC, so you can drum them
with one hand: **F3 kick**, **F#3 hat**, **G3 snare**, **G#3 open hat**, **A3 clap**, **A#3 shaker**, **B3 rim**, **C4 tom 1**,
**C#4 ghost**, **D4 tom 2**, **D#4 perc**, **E4 tom 3**. **F4 to G5** hold the rest: more kicks, snares and hats, the kit's
own percussion, a crash at the top. Play a key and the screen names it: its role on the first octave (KICK, with its
sound, kick1, under it), its sound above (kick2, conga1). The keys' colours show where each kind lives (red kicks, amber
snares and claps, teal hats and shakers, purple toms and percussion). OCT− and OCT+ do not move the drums.

| Control | Does |
|---|---|
| **PRESETS** | The kit: **Dusty**, **808**, **Live**, **Latin**, then **MY KIT 1**, **2** and **3** (your own; an empty one starts as Dusty). Its name is on the top line |
| **A key held + PRESETS** | Puts the next sound of any kit on that key: build your own kit, as on an MPC. The top line says **My kit** |
| **K1 LEVEL** | Of the key played last: 0 to 10 (8 is the kit as it plays) |
| **K2 REVERB** | Of the key played last: how much of it goes to the reverb (0 to 10) |
| **K3 GRID**, **K4 FEEL** | The track's, as on the REC layer (section 2.2) |
| **FX** | The kit's effects, a page a tap: **PUNCH** (K1 COMP, a compressor: tighter, louder; K2 SPEED fast, medium, slow; K3 CLIP, rounds the peaks; K4 SHAPE soft or hard), **COLOR** (K1 TAPE, K2 LOFI, K3 DRIVE, K4 CRUSH: they add up), **SPACE** (K1 DELAY 1/16 to 1/4 dotted at the song's tempo, K2 FEEDBACK, K3 REVERB room, plate, hall, spring, K4 SIZE; the song's delay and reverb, shared with the other tracks). Each runs once for the whole kit; each key decides how much it sends (EDIT, PAD FX). HOME: back |
| **EDIT** | Builds the kit, a page a tap: **KIT** (PRESETS gives the key played last the next sound of any kit, K3 the next kit; K1 LEVEL, K2 VELOCITY: fixed to 10), **SOUND** (K1 TUNE, a pitched sound names its note; K2 ATTACK, K3 HOLD in ms then *full* or *key* (as long as the key is held), K4 DECAY; SELECT its mute group: keys of one group stop each other, the hats at first), **TRIM** (your samples), **PAD FX** (the key's sends: K1 PUNCH, K2 COLOR, K3 DELAY, K4 REVERB; 10, 10, 0, 0 at first, so a new kick and snare go through PUNCH and COLOR, and you take the hats out of COLOR; an effect that is off on FX shows *off*). A new sound on a key starts from its defaults; turning a knob plays nothing. HOME: back. The ARP repeats a held drum (hi-hat rolls) |

Once a beat is recorded, PLAY draws its bar: a row for each key you recorded, in the order you first recorded them,
sharing the height; each hit where it plays, taller the harder you hit, with a line back to where you recorded it, so
you see the feel (GRID and FEEL, section 2.2).

**Keep your kit**: on any EDIT page, hold **SAVE**: **MY KIT 1, 2, 3** show; K1 picks one, hold **SAVE** again and the
kit (every key's sound and settings) is kept there, also when the FM-1 is off. HOME: cancel. PRESETS then finds it after
the built-in kits.

### IDEAS: learn a style's beat

**SCL** to IDEAS, then **PLAY**: the style's beat plays, and its hits fall onto the keys of their sounds; the next one
breathes. Play each as it lands.

| Knob | Does |
|---|---|
| **K1 STYLE** | Boom bap, Dilla, G-funk, Funk, Disco, Half-time, Reggaeton, Shatta, Afrobeats, Reggae, Drum & bass, Trap |
| **K2 BEAT** (or SELECT) | **main**, **busy**, **fill**, **break**, **drop** |
| **K3 SPEED** | 50 to 100 %: the whole song slows while you learn (the top line shows, say, **63 of 90**) |
| **K4 HEAR** | **listen/play** (once round, then your turn), **on**, **off** |

### AUTO: hold a pad, a whole beat

**SCL** again to AUTO. Five keys are the beats: **F3 main**, **G3 busy**, **A3 fill**, **B3 break**, **C4 drop**. Hold
one: the song starts and the beat plays on your kit; let go and it stops. REC records it.

| Knob | Does |
|---|---|
| **K1 STYLE** (or SELECT) | The style |
| **K2 FILLS** | **off**, **4 bars**, **8 bars**: holding main, the last bar of each 4 or 8 is a fill |
| **K3 FEEL** | The track's feel, as on the REC layer: where each drum sits against the beat. The graph shows it |
| **K4 GHOSTS** | **off**, **soft**, **more**: the quiet snares between the beats |

A MIDI keyboard on channel 10 plays the drums too: its notes F3 to G5 are the 27 keys, and General MIDI's drum notes
(36 kick, 38 snare, 42 hat ...) play the kit's kick, snare and hat. **Your own kit**: put your sounds in `kits/user/` named `01-kick1.wav` (the number is the key on the drum map, 01 kick, 02 hat, 03 snare … for F3 to 27 for
G5; the rest is the name the screen shows) with a `LICENSE` file saying whose they are, and build the firmware. They
are the fifth kit, **USER**. Without the `LICENSE` file the build stops. `kits/user/README.md` has the details.
The firmware comes with a USER kit already: the owner's own kick (F3), snare (G3) and hat (F#3), free for anyone (CC0);
the other keys play Dusty's sounds. Replace or add files to make it yours.

## 9. The sequencer: SONG (SEQ)

**SEQ** opens the sequencer; **HOME** goes back to the track. A song is made of **sequences**: up to 16, each a section of
the four tracks (what you record on the track pages goes into the sequence shown). On **SONG** the song is a row of
blocks, each a sequence, as wide as its bars; a block played more than once shows its repeats as dashed copies.

The 16 white keys are the 16 sequences: half lit when they hold notes, dark when empty, the one shown full. The screen shows the song as blocks (a bar wide each, repeats dashed) with a lane per track under them, coloured where that track plays in the block, and the 16 sequence keys below.

| Do | What happens |
|---|---|
| a lit key (stopped) | that sequence goes into the song after the cursor's block |
| a lit key (playing) | that sequence plays from the next bar |
| hold a lit key, press a dark one | the sequence is copied there |
| EDIT + a lit key | "Delete sequence n?"; EDIT + the same key again within 3 s deletes it and its blocks |
| EDIT tapped | the cursor's block goes |
| PLAY | the song from the cursor's block: each block its repeats, then the next, round to the top |

| Knob | Does |
|---|---|
| **K1** | the cursor (also SELECT) |
| **K2 REPEATS** | how many times the block plays (1 to 16) |
| **K3 MOVE** | the block one place left or right |
| **K4 SEQUENCE** | which sequence the block plays |

A new song is sequence 1, empty, as its only block. The tempo and the key are the song's: the same in every sequence;
each sequence keeps its own length (K3 on the song page).

## 9b. The sequencer: SEQUENCE

**SEQ** again from SONG: the sequence's four tracks, each note where it plays (a faint mark where you played it). The
top line says which sequence (SEQ 1). Play and record as on the track's own pages: REC, then PLAY.

| Control | Does |
|---|---|
| **K1 QUANTIZE** | off, 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/64: the notes play on that grid; what you played is kept |
| **K2 SWING** | 50 % (straight) to 75 % |
| **K3 STRENGTH** | how far the notes move toward the grid: 0 % as played, 100 % on it |
| **K4 LENGTH** | the sequence's bars, 1 to 8 |
| **SAVE held** | undoes the last take (or clear, or erase) |
| **EDIT + REC** | clears the track in this sequence (SAVE held brings it back); works on every page |

A note played just before the loop's end (under half a 16th) lands on the first step of the next loop.

## 9c. The sequencer: STEPS

**SEQ** again from SEQUENCE: one bar of the track, its four beats numbered, the cursor's beat split by the GRID. The keys
always sound; with **REC** on, each key writes its note at the cursor.

| Control | Does |
|---|---|
| **K1 GRID** | 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32: the cursor's step |
| **K2 STEP** (and SELECT) | the cursor, a step at a time, across the whole sequence |
| **K3 LENGTH** | the notes at the cursor, and the next ones written: 1 to 16 steps |
| **K4 VELOCITY** | the same, how hard |
| **EDIT + a key** | removes that note at the cursor |

The keys of the notes at the cursor are lit.

## 9c2. Choosing a sound: the Sound Browser

Turn **PRESETS** on a track's page: the list of sounds opens over the page, on the track's group (CHORDS on Keys, BASS on
Bass, MELODY on Lead). PRESETS goes through the sounds one at a time, and on into the next group at the end of one.
While the list shows, **K1** jumps a whole group (K1 GROUP), back to the sound you were on in it. Play the keys to hear a
sound: the list stays open while a key is held, and closes by itself 2 seconds after you stop turning or playing (or
press HOME). The list holds the sounds you picked in the audition, as their engines arrive: now FMini's 66, the Juno-106's 28
(FM-106's engine), the 11 Bass Something 2 patches, 19 OberVave programs, nine CoralFlower VA sounds and five of ours. The DRUMS track's PRESETS still chooses its kit.

### Effects

Every sound brings its own effects, as it was made: its chorus, its echo and its reverb (and some a little drive), so
it sounds like it did in the audition. The echo and the reverb ring on a moment after you let go.

On a Producer sound track, tap **FX** to step through PERFORM, DELAY, REVERB and DIRT. Hold **FX** to return to
PERFORM and press **HOME** to return to playing. On PERFORM, **SELECT** changes the X/Y pair and **K3/K4** move
its two controls. For WOBBLE, **K3** sets the cutoff sweep speed and **K4** sets its depth. Turn depth to zero to
bypass the sweep. The sweep is temporary and applies to the selected track.

On FMini and Juno-106 sounds, PERFORM FILTER uses **K3** for cutoff and **K4** for resonance. Reselect the preset to reload
its original settings. The X and Y values show dashes on sounds that do not have these controls mapped yet.

Tap **EDIT** to open OSC, then tap **EDIT** again or turn **SELECT** to reach FILTER. Hold **EDIT** and release to
return to OSC. On an FMini sound, the four knobs on OSC change the first two waves, oscillator 2 detune and their
balance. On FILTER they change cutoff, resonance, envelope amount and key tracking. The cutoff and resonance
also appear on PERFORM FILTER. **HOME** returns to playing.
FILTER draws the selected FMini or Juno-106 track's live spectrum before the shared delay and reverb.

Press **ENV** for F.ENV and again for A.ENV. The four knobs change attack, decay, sustain and release. An FMini
preset's release follows its decay until you turn RELEASE. On that envelope, release then has its own time.
Juno-106 shows **SHARED ADSR** on both pages. Changes on either page appear on the other. The preset's VCA
mode determines whether the amp follows this envelope or the key gate.

On VOICE, choose POLY, MONO or LEGATO, then set the glide time, when it slides, and the pitch-bend range. Bend defaults
to two semitones. Changing the mode releases the track's currently sounding notes. VOICE is available for Basic, the
shared Felucca engines and FMini.

On FMini, press **LFO** to edit rate, depth, target and wave. Pitch, cutoff and volume are available as targets on
the mono FM-1. At zero depth only the preset's original modulation routes apply.

On FMini's **MOD** page, choose two destinations and their signed amounts. PERFORM MOD and MIDI CC1 apply the
same routes. Bringing MOD to zero returns the sound to its base settings. Reselecting the sound restores the
original vibrato route.

### How many notes at once

The FM-1 plays up to 12 notes at once across the four tracks, as long as they fit its processor. Some sounds take more
of it than others. Bass Something 2 plays one note per track: a second key takes over the same voice. A Juno
sound plays up to five notes on its track (a Cm7 and its bass), an FMini sound up to four. When everything plays at
once and there is no room left, the newest note takes the place of the oldest one (a note that is fading out first,
then a track with two or more notes, the song's before yours); a track never loses its last note. GLO, TRACKS shows the
notes in use and how busy the processor is.

## 9c3. ARP on every track

Tap **ARP** to turn the track's arpeggiator on (tap again: off). Tap it **twice** quickly for **latch**: the notes keep
playing after you let go, until you hold new ones. While ARP is on, the keys feed it and the ARP page shows:

| Control | What it does |
|---|---|
| **K1 MODE** | the order: up, down, up-down, down-up, as played, random, converge, diverge, pinky, thumb, chord |
| **K2 RATE** | 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, in time with the song |
| **K3 OCTAVES** | 1 to 4 |
| **K4 GATE** | how long each note sounds, short to legato |
| **SELECT** | the rhythm: straight, or one of 21 with rests and accents (reggaeton, clave, trap roll …) |

On **CHORDS** each key arpeggiates its chord. On **DRUMS** ARP is note repeat: hold a sound and it repeats at RATE; K1
RAMP makes it grow or fade, K3 PITCH makes it rise or fall. REC records what ARP plays.

Selecting an FMini patch with a factory arpeggio turns on the track's ARP with its factory settings. It follows the
song's tempo and keeps the preset's octave span, gate, swing and note order, including repeat and up + oct where used.
Selecting a sound without a factory arpeggio restores the track's previous ARP settings.

## 9d. Your song is kept

Switch the FM-1 off and on and your song is there: every sequence, the song's order, each track's sound, GRID and
FEEL, the drums' pads and each track's settings (ST-89). The song is kept **about 3 seconds after you press STOP**, with
no key held; while it plays, keeping waits. So stop the song and wait a moment before you switch off.

There is one song. Another FM-1 firmware (Felucca, FM-106, OberVave or fmini-1) uses the same flash for its own
projects: running one of them may overwrite the kept song. Their projects are never touched by this firmware.

## 10. Installing, and the two flash chips

FM-1s are made with one of two 1 MiB flash chips: the original **Puya P25Q80H** (JEDEC ID 0x856014) and, on newer units,
the **UCUN UC25HQ80** (0xB36014). fm1-scales saves and updates on both (D-010), as Felucca 1.8.2 (its stable line),
Melodee 1.6.0 and CoralFlower 0.7 do. Not yet tried on a unit with the newer chip.

If a unit with the newer chip runs a firmware that does not know it (Felucca 1.8.1 or older, for example), that firmware
cannot start an update: hold **OCT−** and **OCT+** for 5 seconds to reach the chip's own update mode, put the official
M-VAVE V15 back with a tool such as M-Flasher, then install fm1-scales.
