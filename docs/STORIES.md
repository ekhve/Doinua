# fm1-scales — User stories

As a … I want … so that … Who wants what, and why: the reason a feature exists. Extracted from the concepts
(`concepts/v0.5/`) when the owner agreed them (2026-11-08); the concepts stay exploratory. Each story is served by use
cases (`USE_CASES.md`, the journeys) and proven by Gherkin scenarios (`../features/fm1-scales.feature`) and
requirements (`REQUIREMENTS.md`). On the board, a story is raised as a Story issue when it is built.

| # | Story | Covers | Accepted by | State |
|---|---|---|---|---|
| ST-01 | As a **producer**, I want to start a song on any track (chords, bass, melody or drums), so that I begin with whatever idea I have. | start anywhere | UC-01, UC-02, UC-03 | partly: chords built; bass, melody, drums are plain keyboards |
| ST-02 | As a producer, I want the key to come from what I record first, or to lock it myself, so that the other tracks help me in the right key. | key AUTO / locked | UC-01, UC-04 · R-087, R-096, R-097 | AUTO built (read when the song stops); GLO page to build |
| ST-03 | As a producer, I want to choose how much help each track gives me (free, in key, guide, auto), so that I play freely or let the FM-1 carry me. | the ladder | UC-01, UC-03, UC-07 | chords built (manual, pad); bass, melody to design |
| ST-04 | As a producer, I want to record each track over the loop as I play it, and go back to any track, so that the song grows in any order. | sequencer, back and forth | UC-04 · R-063, R-064 | built (REC, PLAY, four tracks) |
| ST-05 | As a producer, I want to see on every page the song's key, tempo and what is recorded where, so that I always know where I am. | top line, loop line | all | built (D-037) |
| ST-06 | As anyone, I want a song page for the key, tempo, length and time signature, so that the song's settings are in one place and never change what I played. | GLO | UC-04 · R-096 to R-098 | built (#44) |
| ST-07 | As anyone, I want to choose between Producer, Guided and Game, so that the FM-1 fits what I want to do today. | HOME | UC-05, UC-06 · R-099 to R-103 | built (#44; Guided and Game as placeholders) |
| ST-08 | As a **beginner** (an older child or a grown-up), I want the FM-1 to lead me step by step through making a song, from very easy to a bit harder, so that I learn producing without knowing where to start. | Guided | UC-05 | later |
| ST-09 | As a **child**, I want to play a melody with a unicorn or a dinosaur that jumps when I hit the right key, so that practising feels like a game. | Game | UC-06 | later |
| ST-10 | As a producer, I want chords that fit a bassline or melody I already recorded, so that I can harmonise it. | Harmony Assist | UC-02 | later (v0.2) |
| ST-11 | As a producer, I want bass help like the chords' (in key, the chord now, a style's ideas, one-finger basslines), so that my basslines groove and fit. | bass ladder | UC-01, UC-03 | built (the bass's PLAY, IDEAS, AUTO; D-040) |
| ST-12 | As a producer, when the song already has chords, I want the bass keys to light a note that works with the chord playing at that moment (its root), so that while the loop plays I see a safe option for my bassline without leaving my own playing. | cross-track hints (the tracks help each other) | UC-03 | owner's idea, 2026-11-08; to refine after the lab |
| ST-13 | As a producer with chords recorded, I want the scale choice to show the scales that fit my chords, best first and with how well each fits, so that I find a good scale quickly instead of guessing. | scale fit | UC-03 | built (D-041) |
| ST-14 | As a producer (or anyone learning chords), I want to play some notes, on the keys or a MIDI keyboard, and have the FM-1 tell me the chord I am holding (Am, then Am9 when I add a note), so that I learn what I play and can name an idea I found by ear. | chord finder (CHORDS · PLAY) | UC-08 | built (D-042), to confirm |
| ST-15 | As a producer with other gear, I want the FM-1 to follow a MIDI clock (its tempo, Start, Stop and Continue), so that my song plays in time with a drum machine or a computer. | MIDI clock in (Felucca B) | UC-09 | built (D-044), to confirm |
| ST-16 | As a producer with other gear, I want the FM-1 to send MIDI clock and Start / Stop over USB while the song plays, so that a computer or a drum machine follows my song. | MIDI clock out (C) | UC-09 | built (D-044), to confirm |
| ST-17 | As a producer with a MIDI keyboard or a computer, I want to choose which MIDI channel plays which track (and send each track out on its channel), so that my setup talks to the right track. | MIDI channels (D) | UC-09 | built (D-045), to confirm |
| ST-18 | As a player with a MIDI keyboard, I want its pitch bend, mod wheel and sustain pedal to work, and "all notes off" to silence the FM-1, so that it plays like an instrument and nothing hangs. | MIDI controls (E) | UC-09 | built (D-044), to confirm |
| ST-19 | As a producer, I want a click (off, while recording, always) with its level, and a count-in of one or two bars before recording, so that I record in time. | click and count-in (J; the agreed metronome) | UC-01, UC-09 | built (D-045), to confirm |
| ST-20 | As anyone, I want the screen to go dark after a while with nothing played (a key or a MIDI note wakes it) and to tune the FM-1 by a few cents, so that the screen lasts and the FM-1 plays in tune with others. | screen off and wake, TUNE (K) | UC-09 | built (D-045), to confirm |
| ST-21 | As a producer, I want to map my controller's knobs to the FM-1's knobs (MIDI LEARN), so that I play its sound from my controller. | MIDI learn (F) | UC-09 | backlog |
| ST-22 | As a producer, I want a MIDI Program Change to choose a track's sound, so that my computer sets the sounds. | program change (G) | UC-09 | backlog |
| ST-23 | As a producer, I want to edit and back up my songs and sounds from a web page over USB, so that nothing is lost and big edits are easy. | web editor and backup (H; with song storage, #26) | UC-04 | backlog |
| ST-24 | As a producer, I want the FM-1 to be a USB sound card (its sound recorded on the computer), so that I record it without cables. | USB audio (I) | UC-09 | backlog |
| ST-25 | As a producer with other gear, I want the FM-1 to send MIDI on the TRS jack too. | TRS MIDI out (A) | UC-09 | not possible on the FM-1 (D-043) |
| ST-36 | As a producer, I want to choose what each of the four tracks is (chords, bass, melody or drums), so that a song can have two melodies, two chord parts or whatever it needs. | track templates (GLO, TRACKS) | UC-00, UC-07 | built (D-046) |
| ST-37 | As a producer, I want as many notes as the FM-1 can play, and to see when it is full, so that my parts don't lose notes without me knowing. | twelve voices, fair stealing, the dot | UC-00 | built (D-047) |
| ST-26 | As a producer, I want the melody keys to show which notes land well on the chord playing, which move and which pull, so that my melody fits the chords and I learn why. | melody: the chord guide | UC-11, UC-03 | approved, to build |
| ST-27 | As a producer, I want melody ideas (hooks and figures in a style) that fit my chords and fall onto the keys, in versions and at a slower speed, so that I learn a melody and make it mine. | melody: IDEAS | UC-11, UC-12 | approved, to build |
| ST-28 | As a producer, I want to hold one key and have a phrase in a style play over the chord (a call and an answer), so that I hear a melody in my song at once. | melody: AUTO phrases | UC-11, UC-17 | approved, to build |
| ST-29 | As a producer, I want a second voice (a 3rd, a 6th or the octave, in the key) or an echo on what I play, so that my melody sounds fuller. | melody: PLUS | UC-11 | approved, to build |
| ST-30 | As a producer, I want four good drum kits of 27 sounds each, one on every key and named as their files (kick1, snare2, conga1), so that my beats sound real in the styles I like and I find any sound by playing it. | drums: kits | UC-10 | approved, to build |
| ST-31 | As a producer, I want to give a track a feel (swing, MPC 66, Dilla, laid back, pushed, dembow, shatta) and see where each drum lands, so that my beats groove like a drummer instead of a grid. | the feel (pocket) | UC-10, UC-13 | approved, to build |
| ST-32 | As a producer, I want to hold one pad and get a style's beat, with a busier one, a fill, a break and a drop, automatic fills and ghost notes, so that I have drums at once. | drums: AUTO | UC-10, UC-17 | approved, to build |
| ST-33 | As a producer, I want to learn a style's beat on the pads, slower if I need, so that I can play it myself. | drums: IDEAS | UC-12 | approved, to build |
| ST-34 | As a producer, I want an MPC-style compressor and soft clipper, lo-fi and tape on the drums, so that they hit hard or sound dusty. | drums: FX | UC-10 | approved, to build |
| ST-35 | As a producer, I want to put my own samples (with their licence) on the keys as a kit, so that the FM-1 plays my sounds. | drums: the user kit | UC-14 | approved, to build |
| ST-38 | As a producer, I want each pad to have its own repeat, tuning and decay, and the tuning shown as a note, so that I roll a hat, keep a kick single and tune my 808 to the song. | drums: per pad | UC-10, UC-14 | approved, to build |
| ST-39 | As a producer, I want to put any sound from any kit on any key and keep it as my kit, so that I build my own kit as on an MPC. | drums: my kit | UC-14 | approved, to build |
| ST-40 | As a producer, I want each track's recording settings in one place (the grid and the feel on playback, the click, the count-in, clearing the track), so that I record and tidy any track the same way. | the REC layer | UC-13 | approved, to build |
| ST-41 | As a producer, I want to erase the notes of one key as the song plays, and undo my last changes, so that I fix a take without starting again. | erase and undo | UC-13 | approved, to build |
| ST-42 | As a learner, I want to practise an idea slower (50 to 100 %) while the whole song slows with it, so that I can copy it, and what I record still fits the song. | practice speed | UC-12 | approved, to build |
| ST-43 | As anyone, I want a short explanation when I turn a knob, and to switch the explanations off once I know the FM-1, so that I learn the controls without a manual and they don't get in my way later. | explanations | UC-00 | approved, to build |
| ST-44 | As anyone, I want the controls that do nothing in the current setting greyed out, so that I know what matters now. | greyed controls | UC-00 | approved, to build |
| ST-45 | As a producer, I want to drop MIDI files in a folder and have them become the ideas and the AUTO patterns of a template (melody, bass, chords, drums), so that I learn and use grooves I love. | MIDI files as ideas | UC-15 | approved, to build |
| ST-46 | As a producer, I want to edit a sound's glide (portamento), attack and release, so that a chord or a lead slides and breathes the way I want. | sound edit; glide and mono, first of the sound stories (`SOUNDS.md`) | UC-07 | in progress: glide and mono in the sound (API 27, D-073, R-234..R-236); where they are set comes with the sound pages (R-237) |
| ST-47 | As a producer, I want a tempo from 40 to 240 BPM, so that drum and bass, footwork and slow songs all fit. | tempo range | UC-04 | built (D-048) |
| ST-48 | As a producer, I want the styles I love on every template (reggae, afrobeats, shatta, reggaeton, drum and bass, as well as boom bap, Dilla, G-funk, funk, disco, trap): beats, chords, basslines and phrases, so that a whole song can be in one style. | styles | UC-17 | approved, to build |
| ST-49 | As a producer, I want the scale choice to tell me in words whether a scale fits my chords or which of their notes it lacks, so that I understand what the fit means. | scale fit in words | UC-11, UC-03 | approved, to build |
| ST-50 | As anyone, I want every template to open on PLAY, so that I always start by playing. | PLAY first | UC-00 | approved, to build |
| ST-51 | As a child of four, I want to make a unicorn jump fences by playing the key that lights, so that I play a song without knowing it, and laugh when it bonks or poops. | Game: Unicorn Jump | UC-18 | built, the owner to approve |
| ST-52 | As a parent, I want to teach the unicorn a song by playing it, on the keys or over MIDI, so that my child can jump to the songs we love, which cannot ship in the firmware. | Game: Unicorn Jump (#84) |
| ST-53 | As a child, I want to make the three little ladies of Donostia dance by playing their song, so that I learn a song from home and laugh when one falls or a seagull gets her. | Game: Hiru Damatxo |
| ST-54 | As a child, I want a robin on an apple tree to sing the notes I play, so that I play Aldapeko sagarraren and see the sheep dance at the end. | Game: Aldapeko Sagarra |
| ST-55 | As a child, I want to drive an old steam train through the desert night by playing its song, so that I play I've Been Working on the Railroad or Funiculì, funiculà and win the train a star at the station. | Game: the Night Train |
| ST-56 | As a child, I want the keys to light up as a game plays its song on PLAY, so that I see which keys make the song before I play it. | Games: PLAY lights the keys |
| ST-57 | As a grown-up who never made a beat, I want a guided mode that teaches me, step by step, what is in a beat, so that I learn by playing, without knowing anything first. | Guided: the guided mode and lesson 1 (#98) | UC-19 | built, approved by the owner (#99) |
| ST-58 | As a beginner, I want lesson 2: the tempo with my hands, then my first beat recorded (kick, snare, hats), so that I feel the beat and build one. | Guided: lesson 2 | UC-19 | backlog |
| ST-59 | As a beginner, I want lesson 3: groove (accent, ghosts, swing, pocket, real grooves, fills, rolls, the grid), so that my beats sound human. | Guided: lesson 3 | UC-19 | backlog |
| ST-60 | As a beginner, I want lesson 4: keys and chords (the scale, outside notes, chord types, minor or major, progressions, playing the chords), so that I know how notes are chosen. | Guided: lesson 4 | UC-19 | backlog |
| ST-61 | As a beginner, I want lesson 5: chord colours (the chord pad, 7, 9, sus, voicings, strum, arp), so that my chords have colour. | Guided: lesson 5 | UC-19 | backlog |
| ST-62 | As a beginner, I want lesson 6: the bass (the root, the kick, octaves, patterns), so that my songs have a low end. | Guided: lesson 6 | UC-19 | backlog |
| ST-63 | As a beginner, I want lesson 7: the melody (chord notes, pentatonic, rhythm, call and answer, a second voice), so that I can write a tune. | Guided: lesson 7 | UC-19 | backlog |
| ST-64 | As a beginner, I want lesson 8: three ways to start a song (beat, chords, melody first), so that I can start from what I have. | Guided: lesson 8 | UC-19 | backlog |
| ST-65 | As a beginner, I want lesson 9: make my own beat, recorded with help, so that I end with a 4-bar song of my own. | Guided: lesson 9 | UC-19 | backlog |
| ST-66 | As a beginner, I want practice games for scales and chords with the lights, for stars, so that I remember them while playing. | Guided: practice | UC-19 | backlog |
| ST-67 | As a producer, I want the sounds I picked for the engines the FM-1 already has (Synthesthetic's 808s and G-FUNK LD, the chip sounds, Tessera's and GHOULBOX's) as presets, so that I play them today. | sounds: presets for our engines (`SOUNDS.md`) | UC-07 | backlog, next after ST-46 |
| ST-68 | As anyone, I want to choose a sound by its category first (bass, lead, keys, organ, pad, strings and voices, brass and winds, pluck and bell, chip, effects, drums), so that a long list stays easy to find my way in. | sounds: the browser by category | UC-07 | built (D-087): the list as the Sound Browser Lab, with the sounds of the engines built |
| ST-69 | As a producer, I want chorus, delay, plate reverb, drive and a compressor (after OberVave), so that every sound is wider and fuller. | sounds: effects | UC-07 | backlog |
| ST-70 | As a producer, I want fmini-1's Minimoog engine with the 66 patches I picked, playing chords when the CPU allows, so that I have its basses, leads and pads. | sounds: the fmini engine (D-070) | UC-07 | built (D-086): the engine and the 66 sounds; their effects come with ST-69, their arpeggios with ST-77 |
| ST-71 | As a producer, I want the Prophet engine with the 32 sounds I picked (Sequential's, copied, and Tessera's ORACLE), so that I have its pads, brass and the whiny lead. | sounds: the Prophet engine (#100, D-069) | UC-07 | backlog |
| ST-92 | As a player, I want my 11 Bass Something 2 patches in PRESETS with one voice per track. | November work order, D-095 | UC-07 | in progress |
| ST-72 | As a producer, I want OberVave's engine with the 19 programs I picked, arpeggios included, so that I have its arps, leads and pads. | sounds: the OberVave engine | UC-07 | backlog |
| ST-73 | As a producer, I want the VA engine (ChoralRoot's, in CoralFlower) with the 9 pads and basses I picked, so that I have its chord pads. | sounds: the VA engine | UC-07 | backlog |
| ST-74 | As a child or a producer, I want the VOICE engine with TALKBOX, WOW BASS and the choirs I picked, so that the FM-1 talks and sings. | sounds: the VOICE engine | UC-07 | backlog |
| ST-75 | As a producer, I want sampled drum kits (mine, Felucca's Hügelton kits, CC0 sets) kept in their own place in the flash, so that real drums do not take the room the engines need. | sounds: sampled kits | UC-10 | backlog |
| ST-76 | As a producer, I want the sounds I picked on engines we do not port (FM TONE, FM6, GRAIN, SID, CZ-1) designed again on ours, so that I keep them without seven more engines. | sounds: our own designs | UC-07 | backlog |
| ST-77 | As a producer, I want an arpeggiator with up, down, up and down, random, rates, octaves and latch, kept with each sound, so that the arp sounds I picked play as they should. | sounds: the arpeggiator (Arp Lab concept, 2026-11-19) | UC-07 | built (D-088) |
| ST-78 | As a producer, I want ARP on a drum track to be note repeat (1/8, 1/16, 1/32, triplets) with flams and rolls, so that I play hat rolls and fills by hand. | drums: note repeat (with ST-38) | UC-10 | built (D-088) |
| ST-79 | As a producer, I want the chords' PLAY page to give help in steps (Free, Key, Shapes, Pads), with one note showing the chords around it and the white keys transposed to the key if I want, so that I play chords my way with as much help as I need. | chords: the HELP ladder (Chord Lab v1–v5) | UC-03, UC-07 | built (D-075) |
| ST-80 | As a producer, I want the chord pads on the white keys as in C (C = I … B = vii) in every key, ten colours on the black keys, the whole keyboard drawn with what it plays, and the progressions in a numbered menu (None, Mine, Follow bass, Follow melody and 45 more), so that I find and play a progression fast. | chords: Pads and PROGRESSION | UC-03 | built (D-076) |
| ST-81 | As a producer, I want a chord's notes to sound as a person would play them (tight, pianist, laid back, rolled, strum up, strum down, sloppy), so that the chords sound human. | chords: FEEL | UC-03, UC-07 | built (D-077) |
| ST-82 | As a learner, I want chord ideas that are short pieces of songs (2 or 4 bars, with their rhythm), lit on the keys a bar ahead, so that I can follow and learn them. | chords: IDEAS as snippets | UC-12 | built (D-079) |
| ST-83 | As a producer, I want AUTO to give me the parts of a song in my style (intro, verse, pre-chorus, chorus, bridge, break, outro; calm or with energy) on the keys, with moves on the black keys, so that I get something good going fast and build a song from it. | chords: AUTO as song parts | UC-03, UC-17 | built (D-080) |
| ST-84 | As a producer, I want the bass to work as the melody does (SPEED on IDEAS, the octave or the fifth on PLUS), and the drums' AUTO feel on K3 and ghosts on K4, so that every track reads the same way. | tracks: one pattern (Track Modes) | UC-07 | built (D-074) |
| ST-85 | As a producer, I want sequences (a section of the four tracks each) on the keys of a SONG view, to add, repeat, move, copy and delete (with a question), so that I arrange a whole song. | sequencer: SONG | UC-20 | built (D-081) |
| ST-86 | As a producer, I want to record live as played, with quantize (1/4 to 1/64, triplets), swing and strength, to undo the last take and to clear a track at once, so that live mistakes cost nothing. | sequencer: SEQUENCE | UC-13, UC-20 | built (D-082) |
| ST-87 | As a producer, I want to write notes, chords and hits on steps (one bar at a time, beats numbered, a grid from 1/4 to 1/64), so that I place a kick on step 1 without playing it in time. | sequencer: STEPS | UC-20 | built (D-083) |
| ST-88 | As a producer, I want every knob I turn on every page to be recorded while REC records, and to see, change and clear those lanes, so that the song moves (filter, octave, vibrato, effects). | sequencer: MOTION | UC-20 | backlog, drafts for the owner (REQUIREMENTS-PLANNED.md) |
| ST-89 | As a producer, I want my song kept when the FM-1 is switched off, so that I carry on tomorrow. | song storage (with #26) | UC-04 | built (D-085) |
| ST-90 | As a producer, I want punch-in effects (stutter, filter, reverb and delay throws, crush) on all tracks or on one, recorded with the song, so that I perform a beat. | FX: punch-in on PERFORM | UC-10, UC-20 | backlog, drafts for the owner (REQUIREMENTS-PLANNED.md) |
| ST-91 | As anyone, I want GLO held to explain the page I am on, and every page to show the sequence REC records into, so that the screens can stay simple without leaving me lost. | help and the top line | all | backlog, drafts for the owner (REQUIREMENTS-PLANNED.md) |

**Cross-track hints (ST-12, the owner, 2026-11-08).** What one track has recorded helps the next: recorded chords light
their root on the bass keys at their moment, and later the chord's notes on the melody keys and the bass's notes on the
chord pages. The lit key is an option, never a rule: every key still plays. Open: on by default whenever chords exist,
or only with HELP set to Chord (as in the bass lab today).

**The chord finder (ST-14, the owner, 2026-11-12).** The CHORDS track's pages follow the convention of every track,
PLAY · IDEAS · AUTO on SCL: **PLAY** (new, where the FM-1 starts) names what you play; **IDEAS** is the page that was
called MANUAL (the knobs build a chord, SELECT walks the ideas); **AUTO** is the chord pad (one finger, a whole chord).
A chord with no name shows its notes, and under them, smaller, the closest chord and how many of its notes are missing
(Sketchpad's partial reading). Open to confirm: the closest line, and K1 HELP (the key's lights, or none).

**MIDI and the FM-1's other connections (ST-15 to ST-25, the owner, 2026-11-12).** From the list of what Felucca
supports: "I think all midi. A, B, C, D, E, J, K. The rest we keep as stories in backlog." Built in that order (B, C, E
need no new setting; D, J, K live on the settings behind the song page, Decision #63). A, MIDI out on the TRS jack, is not possible: the FM-1's jack is
MIDI IN only (D-043). F to I are in the backlog.

**Track templates (ST-36, ST-37, the owner, 2026-11-12).** "Each track can select which template we use (chord, bass,
melody, drums)": chosen on GLO's TRACKS page, so they are set up once and not changed while playing. Changing a
template keeps the track's notes (a track with notes asks first: SAVE). The help listens to the notes on the other
tracks, not to their names, so changing a template changes no other track's help. "We can have a dot or something
showing we have voice issues and/or cpu": the amber dot.

**From the labs (ST-26 to ST-50, the owner, 2026-11-12 and 13).** The melody lab and the drum lab were tried and
reviewed three times; these stories are what was agreed. The labs are `concepts/v0.5/melody-lab.html` and
`drum-lab.html`, the concepts `MELODY.md` and `DRUMS.md` (exploratory: where they differ, the stories win). The owner's
words that shaped them:
- melody: "I dont need so many text parts in the screen"; lands well, moves and pulls "would be nice ... but not sure
  how" (now the note's colour); "the 29 % ... notes that are outside?" (now "fits" or "lacks F#"); "speed for melody
  ideas is needed"; "What is Take?" (now a version, on SELECT); glide "should be part of the sound".
- drums: "the first screen for any template is always play"; "the ability to remove the recordings is important";
  "the metronome ... start recording from 4, 3, 2, 1"; "repeat, decay and tune ... per sound/key"; "people like to
  choose their drums like in the mpc"; "i dont like that we have the same for every octave ... if i press it it sounds
  and i see the name of the sound ... kick1.wav, then you show kick1"; "people like to tune their 808s"; the
  quantization "for each track"; the speed "probably shouldnt simply be half or full"; Shatta "like Kybba,
  Limitlezz"; "you can add reggaeton ... afrobeat ... reggae, and drum and bass"; "reggae chords, afrobeat pads".
- everywhere: the explanations "an option in global ... to remove it"; with HELP Free "the scale and other options ...
  should be grayed out"; MIDI files as ideas "for the melody ideas and for the auto. the same for the other template".

**The user kit's files (ST-35).** One file a key, `kits/user/01-kick1.wav` to `27-....wav`: the number is the key (01
F3 to 27 G5, the owner's choice of numbers), the rest is the name the screen shows ("kick1"). A `LICENSE` file is
required. Confirmed by the owner (2026-11-13: "yes, we can use the naming you specify").

