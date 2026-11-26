# fm1-scales — behaviour scenarios
#
# Each scenario names a starting state, an action, and what must then be true. Two audiences:
#   1. The factory — scenarios tagged @auto are checked by a machine. Each one is implemented either by an
#      executable scenario in factory/scenarios/*.json (its "scenario" field is this title, exactly) and run
#      on the host simulator and the emulated firmware, or by a gate named in a @gate:<name> tag
#      (build, unit, oracle, safety, usb_midi, cpu; see factory/check.sh).
#   2. A person — scenarios tagged @manual are checked on a real FM-1 or in a play test with a child.
#
# Traceability: every scenario is cited by a requirement in docs/REQUIREMENTS.md; @UC-nn links to
# docs/USE_CASES.md, @D-nnn to docs/PLATFORM.md. factory/trace.py fails if any link is broken.

Feature: Playing the keys
  The first thing the platform must do: a key makes its note, in tune, and the screen and lights say what it is.

  @D-004 @auto
  Scenario: A key plays its note in tune and the screen names it
    Given the hello app has started
    When C4 is held, then E4
    Then C4 sounds within 15 cents of 261.6 Hz, then E4 within 15 cents of 329.6 Hz
    And it is silent before, between and after the notes
    And the device's screen is the host simulator's, pixel for pixel

  @D-004 @auto
  Scenario: The keys of the scale glow, the tonic breathes, and they follow the key
    Given the key is "C major"
    Then the keys of C major are lit dim and the Cs breathe
    And a held key is lit fully
    When SELECT turns two clicks
    Then the keys of D major are lit dim and the Ds breathe

  @D-014 @auto
  Scenario: Every engine's first preset plays its note
    Given each engine's first preset in turn
    When C4 is held
    Then Analog, Phase and Lo-fi sound C4, Trio FAT BASS and Organ wheel FULL ORGAN sound C3, Noise makes sound
    And every note stops when released

  @D-025 @auto
  Scenario: PLAY plays the scale up at the tempo
    Given the key is "C major" and the tempo 90
    When PLAY is pressed
    Then C4, D4, E4 and F4 sound in turn, one every 667 ms
    And the key of each sounding note is lit fully

  @D-025 @auto
  Scenario: The patterns change the order
    Given the pattern "Down"
    When PLAY is pressed
    Then C5, B4 and A4 sound in turn

  @D-025 @auto
  Scenario: PLAY again stops the playing
    Given the hello app is playing
    When PLAY is pressed again
    Then the note stops and no other note follows

  @D-025 @auto
  Scenario: OCT- and OCT+ move the keys at most two octaves
    When OCT+ is pressed three times
    Then C4's key sounds C6
    When OCT- is pressed five times
    Then G5's key sounds G3

  @D-025 @gate:unit @auto
  Scenario: The player keeps time and note length
    Given a sequence at a step of 667 ms
    When the main loop runs for 200 steps, polled every 1 ms or every 7 ms
    Then each note starts within 2 ms of its step and the error does not grow
    And each note sounds for 92 % of a step
    And stopping silences the note at once, and a sequence that does not loop ends once

  @D-028 @auto
  Scenario: The firmware draws the concept's game and study screens
    When HOME is pressed
    Then the device shows the kids' game screen with its pixel-art images, pixel for pixel as the host
    When HOME is pressed again
    Then it shows the study screen, pixel for pixel as the host

  @D-028 @gate:unit @auto
  Scenario: The firmware draws images inside the screen only
    Given a small image with a transparent pixel
    Then it is drawn with the right colours, at scale 1 and 2, the transparent pixel unchanged
    And drawn across every edge of the screen it writes nothing outside it

  @D-029 @auto
  Scenario: Four tracks sound at once with their own sounds
    Given the bass on Analog SQR BASS, the melody on Phase BELL and the drums on the kit
    When a note arrives on MIDI channel 2, then 3, then 10
    Then the bass sounds A2 and the melody E5 in tune, and the drum sounds
    When notes arrive on all four tracks together
    Then all of them sound

  @D-029 @gate:unit @auto
  Scenario: The four tracks share eight voices, chords first
    Given eight voices in use on the four tracks
    When a melody or bass note arrives
    Then it takes a chord voice, a releasing one first
    When a chord note arrives with only bass, melody and drum voices sounding
    Then the chord note is not played and their notes go on

  @D-031 @auto
  Scenario: A song plays its four tracks, looping, and stops
    Given the demo song at 120 BPM: in bar 1 the bass, the melody, a chord and the drums each alone, in bar 2 all together
    When SEQ is pressed
    Then the bass sounds A2 and the melody E5 in tune on their beats, then the chord and the drums, then all of them
    And the song starts again from the top after two bars
    When SEQ is pressed again
    Then the notes are released and nothing more is played

  @D-031 @gate:unit @auto
  Scenario: A recorded performance plays back as it was played
    Given an empty two-bar song at 120 BPM with the bass track armed
    When a note is played at tick 30 and held for 24 ticks while it loops
    Then the song keeps it at tick 30, 24 ticks long, and plays it back there on the next loop
    And a note held over the loop's end keeps its whole length
    And a step-entered chord fills its bar, and the notes of the four tracks start and stop on their ticks
    And at 97 BPM, loop 1000 starts within one main-loop pass of its exact time

  @D-032 @gate:unit @auto
  Scenario: The song's settings never change what was played
    Given a song with C F C on the bass and a drum on C sharp
    Then its AUTO key is C major, ambiguous, and with E and B on the melody C major for sure
    When its key is locked to A minor and set back to AUTO
    Then every note is as it was
    When it is transposed up a tone
    Then its notes and its locked key move up a tone, and the drum stays
    When it is shortened to one bar, the note in bar 2 is kept and not played, and back at two bars it is there
    When the tempo doubles on beat 2
    Then beat 3 comes half a beat-length later at the new tempo

Feature: Input
  How hard a key is pressed, and notes from other instruments.

  @D-017 @auto
  Scenario: Velocity comes from how hard a key is pressed
    Given the Sine sound
    When C4 is pressed softly (velocity 32), then hard (127), then F3 at 64
    Then the hard press is 3.97 times as loud as the soft one, and F3 twice, within 10 %

  @D-017 @manual
  Scenario: Velocity feels right on a real FM-1
    Given hello on a real FM-1
    When a person plays soft and hard on keys F3..C5
    Then the loudness follows the touch evenly, from a whisper to full

  @D-020 @auto
  Scenario: Notes from a MIDI keyboard on the TRS jack play and stop
    Given a MIDI keyboard on the TRS jack
    When it sends C4, E4 and G4 using running status, note-on 0 as release and a clock byte mid-message
    Then each note sounds in tune and stops when released

  @D-020 @gate:usb_midi @auto
  Scenario: Keys go out over USB-MIDI and notes from a computer come in
    Given the FM-1 connected to a computer by USB
    When C4 is pressed at velocity 100 and released
    Then the computer receives note-on 90 3C 64 within 20 ms and note-off 80 3C 00
    And a note-on sent from the computer reaches the app

  @D-008 @auto
  Scenario: The output stays at or below −6 dBFS
    Given MASTER at full and the Organ sound
    When eight keys are held at velocity 127
    Then the output's peak is at or below −6 dBFS

Feature: Remembering
  What a child set up is still there next time.

  @D-019 @auto
  Scenario: Settings survive a power-off
    Given the key is set to D, the scale to Natural minor and the names to Do-Re-Mi
    When the FM-1 is switched off and on again
    Then it comes back in D Natural minor with Do-Re-Mi names

  @D-019 @gate:unit @auto
  Scenario: A power cut during a save keeps a whole record
    Given a saved record
    When the power fails at any step of saving a new one
    Then the old record or the new one loads, whole

  @D-023 @gate:unit @auto
  Scenario: The store paces its flash writes
    Given an app that saves a new record every 10 ms for 10 minutes
    Then nothing is written until the record has stayed the same for 2 s
    And then it is written once, and the last record saved is the one stored
    And two writes are at least 60 s apart
    And no write happens while a key is held or a voice sounds
    And a record equal to the stored one is not written

  @D-022 @gate:unit @auto
  Scenario: The store never erases another firmware's data
    Given another firmware's data in some of the store's sectors
    When the app saves many times, with power cuts in between
    Then that data is unchanged, byte for byte
    And when every sector holds another firmware's data, the save is refused and nothing changes

Feature: Staying updatable
  An FM-1 running this firmware can always be given another firmware.

  @D-006 @gate:safety @auto
  Scenario: The installer can always replace the firmware
    Given the firmware running, connected by USB
    Then it enumerates as USB-MIDI and answers the installer's handshake
    And the UBOOT soft key takes it into update mode

  @D-018 @gate:safety @auto
  Scenario: Holding OCT- and OCT+ for 5 seconds enters update mode, and not sooner
    When OCT- and OCT+ are held for 2 seconds
    Then nothing happens
    When they are held on
    Then update mode starts 5 seconds after the press

  @D-006 @HAZ-04 @gate:safety @auto
  Scenario: A firmware that crashes at boot drops into update mode
    Given a test build of the firmware that faults 1 s after boot
    When the FM-1 boots, faults, and boots again
    Then on the third boot the firmware enters update mode instead of running the app
    And the normal build neither resets nor asks for update mode

  @D-011 @gate:safety @auto
  Scenario: Every build carries the pinned update loader
    Then the package's update loader is byte for byte the pinned felucca-bsp loader

  @D-006 @manual
  Scenario: It installs on a real FM-1 and goes back to stock
    Given a real FM-1 with a backup of its flash
    When hello.fwsc is installed with the felucca-bsp installer
    Then it starts and plays
    When the official firmware is installed over it
    Then the FM-1 is back to stock

Feature: Fitting the hardware
  @D-016 @gate:cpu @auto
  Scenario: Every sound with eight voices fits the CPU
    Given each of the 42 sounds with eight notes held
    Then the interrupts take less than 75 % of the CPU on the cycle-modelled emulator
    And so they do with four tracks at once: each sound on track 0 with four notes, and the bass, melody and drums

  @D-001 @gate:build @auto
  Scenario: The firmware fits the flash and the RAM
    Then the image fits 90 % of the app area, and RAM and the pool fit theirs

Feature: Music theory
  @D-005 @gate:oracle @auto
  Scenario: Note names and scales match Sketchpad in every key
    Then the C port's note names, key spellings and scales equal Sketchpad's for all 24 keys and 17 scales

  @D-024 @gate:oracle @auto
  Scenario: Run orders and tempo match Sketchpad
    Then the C port's run orders equal Sketchpad's for every scale, four tonics, the four patterns, with and without ending on the top, and four seeds
    And its tempo limits and step lengths equal Sketchpad's

  @D-030 @gate:oracle @auto
  Scenario: Chords, voicings and key analysis match Sketchpad
    Then the C port's triads, numerals and inversions equal Sketchpad's for the seven-note scales in every key
    And its judgement of progressions against keys (keyFit) equals Sketchpad's on 469 progressions
    And its ranking of keys for played notes (keysForNotes), ambiguity included, equals Sketchpad's on 407 note sets

  @D-033 @gate:oracle @auto
  Scenario: Colour chords, voicings and the next chord match Sketchpad
    Then the C port's chord for each triad quality and colour is a chord of Sketchpad's dictionary, with its symbol
    And its notes in the five voicings (root, inversion 1 and 2, open, wide) equal Sketchpad's for 12 roots in 3 octaves
    And Sketchpad's identifyChord names each chord as the C port labels it
    And its likely next chord after each of a key's chords equals Sketchpad's suggestNextChords in the 24 keys

  @D-035 @gate:oracle @auto
  Scenario: Names of played chords match Sketchpad
    Then the C port names played notes as Sketchpad's identifyChord does, in the key's spelling, for the colour chords in two voicings and 600 random note sets

Feature: The chord pad
  (Since D-042 the chord pad is the CHORDS track's AUTO page: SCL steps PLAY, IDEAS, AUTO.)
  The CHORDS track's PAD page (concept v0.4, docs/MANUAL.md): one finger plays a style's progression with its colours,
  the black keys add colours, and what is recorded lights its pads as the song plays.

  # CP-1: play a style's progression with one finger

  # CP-3: add or lock a colour

  # CP-5 and CP-6: record in real time, see what is recorded

  # CP-7: know where you are
  @D-034 @auto
  Scenario: Holding GLO shows help for the screen
    Given the chords' AUTO page
    When GLO is held
    Then the AUTO page's help is shown over its lights, and when GLO is let go the page again
    When SCL goes back to the keyboard page and GLO is held
    Then that page's help is shown

Feature: The song page and the modes
  GLO tapped: the song's settings (concept v0.5 §2.4). HOME held: Producer, Guided, Game (§2.5); Guided and Game are
  placeholders for now.

  @D-038 @auto
  Scenario: Tapping GLO opens the song page, where the key, tempo and length change and nothing played does
    Given a C4 recorded on the chords track of the empty song
    When GLO is tapped
    Then the song page shows KEY, TEMPO, LENGTH and TIME on the four knobs, and the four tracks as played
    When K1 is turned to D, K2 to 100 BPM and K3 to 2 bars, and GLO is tapped again
    Then the manual page is in D major: its first chord, D, lights D4 F#4 A4, and C4 is dark
    And the recorded C4 still plays when the 2-bar loop comes round

  @D-038 @auto
  Scenario: Holding HOME opens the modes; a new mode asks first, shows its home and keeps the song
    Given a C4 recorded in a 1-bar loop that is playing
    When HOME is held
    Then the mode picker shows Producer (now), Guided and Game
    When SELECT is turned to Guided and HOME is pressed
    Then it asks "Switch to Guided?", and HOME again switches
    And Guided shows its first lesson with a teal top line, while the song goes on
    When Producer is chosen and confirmed the same way
    Then the song is still there: the C4 plays when the loop comes round

  @D-071 @approved:owner @auto
  Scenario: The song page closes when the track's sound or octave changes
    Given the song page is open
    When OCT+ is pressed
    Then the song page closes, the octave goes up and C4 sounds C5

  @D-071 @approved:owner @auto
  Scenario: HOME tapped closes the song page, the REC layer and a question
    Given the REC layer is open and asks to clear the track
    When HOME is tapped
    Then the layer and the question are gone, and OCT+ raises the octave again

  @D-071 @HAZ-02 @approved:owner @auto
  Scenario: A question takes the knobs and new keys until it is answered
    Given the REC layer asks to clear the track
    When a key is pressed
    Then it makes no sound
    When another button answers no
    Then the key plays again

  @D-071 @approved:owner @auto
  Scenario: SAVE tapped says how to undo and keeps the take
    Given a C4 recorded in a 1-bar loop that is playing
    When SAVE is tapped
    Then the screen says to hold SAVE to undo, and the C4 still plays the next time round

  @D-071 @HAZ-02 @approved:owner @auto
  Scenario: A key let go while the modes show stops its note
    Given an organ chord held on CHORDS
    When HOME is held and the key is let go while the modes show
    Then the organ stops

Feature: The bass track
  BASS, three pages on SCL (concept v0.5, BASS.md, tried in the bass lab and approved by the owner on 2026-11-08):
  PLAY (the keys play the bass an octave down, with HELP and one SCALE), IDEAS (a style's line falls onto its keys)
  and AUTO (a degree pad plays a pattern on its root).

  @D-040 @UC-01 @approved:owner @auto
  Scenario: The bass plays an octave down and lights the key; notes outside it play softly or not at all
    Given the song is empty, in C major, and ALGORITHM is on BASS (page PLAY, HELP Key)
    When A3 is played
    Then A2 sounds, and the key's notes glow dim with C breathing
    When OUTSIDE is set to mute and C#4 is played
    Then nothing sounds

  @D-040 @UC-01 @approved:owner @auto
  Scenario: On Whites the white keys play the scale and the black keys rest
    Given BASS, page PLAY, HELP Whites, in C major
    When the first white key (F3) is played
    Then C3 sounds, and the white keys glow with the tonic's breathing
    When a black key is played
    Then nothing sounds

  @D-040 @UC-03 @approved:owner @auto
  Scenario: Key and root light the root of the chord recorded at that moment over the dim scale
    Given a 1-bar song with Am recorded on the chords track, playing
    When BASS is on PLAY with HELP Key+root
    Then while the Am sounds, the A keys light full and the rest of the scale glows dim

  @D-040 @UC-03 @approved:owner @auto
  Scenario: IDEAS plays a style's line, then leaves the next time round to you
    Given a 1-bar song with no chords (so the line goes over the key's I) and BASS on IDEAS, HEAR listen/play
    When PLAY is pressed
    Then the line plays the first time round (its first note C3), is silent the second, and plays again the third
    And its notes fall onto the keys they belong to on the screen

  @D-040 @UC-01 @approved:owner @auto
  Scenario: AUTO: holding a pad plays a pattern on its root, starting and stopping the song
    Given BASS on AUTO, Latin tresillo, in C major, the song stopped
    When the I pad (F3) is held
    Then the song starts and the pattern plays on C3, the I pads lit
    When it is let go
    Then the song stops
    When the ii pad (G3) is held
    Then the pattern plays on D3

  @D-040 @gate:unit @auto
  Scenario: The bass keeps its ideas and patterns inside the scale
    Then each note of a line or pattern outside the chosen scale moves to the nearest note in it, on the song's key
    And the white keys play the scale from its tonic, and the scales offered follow the key's mode

  @D-041 @UC-03 @approved:owner @auto
  Scenario: With chords, SCALE walks the scales that fit them, best first, in a pop-up with their fit
    Given a song with Am recorded on the chords track, in C major, and BASS on PLAY
    When K2 SCALE turns
    Then the scales come in the order they fit the chords (Sketchpad's fit), best first, the chosen one lit
    And a pop-up lists the scales around it with how much of the chords each holds, then closes by itself
    And the keys light the chosen scale

Feature: Chord play
  CHORDS · PLAY, where the FM-1 starts (ST-14, D-042): play notes on the keys or a MIDI keyboard on channel 1, and the
  FM-1 names the chord they make; a chord with no name shows its notes and the closest chord. Proposed by the agent from
  the owner's request of 2026-11-12, for the owner to confirm.

  @D-042 @UC-08 @approved:agent @auto
  Scenario: Held notes are named as a chord, and a note added names the new chord
    Given the companion app has started on CHORDS, PLAY, in C major (HELP Key)
    Then the key's notes glow dim
    When A3, C4 and E4 are held
    Then they sound, their keys light full, C breathes as the key's tonic, and the screen says Am, vi
    When G4 and B4 are added
    Then the screen says Am9, vi, and lists the notes A C E G B

  @D-042 @UC-08 @approved:agent @auto
  Scenario: Notes that are no chord show the notes and the closest chord
    Given CHORDS, PLAY, in C major
    When C4, C#4 and D4 are held
    Then the screen shows the notes C Db D, and says no chord
    When C4, E4 and B4 are held
    Then the screen shows the notes C E B, and under them, smaller, closest Cmaj7 with 1 missing

  @D-042 @UC-08 @approved:agent @auto
  Scenario: Notes from a MIDI keyboard on channel 1 are named as a chord
    Given CHORDS, PLAY, in C major
    When a MIDI keyboard holds A3, C4 and E4 on channel 1
    Then they sound, their keys light full, and the screen says Am with the MIDI channel
    When it lets go
    Then the keys go back to the key's lights

Feature: Playing with other gear over MIDI
  MIDI clock in and out, and a keyboard's controls (ST-15, ST-16, ST-18, D-044), from the owner's choice of 2026-11-12
  among what Felucca supports. Proposed by the agent, for the owner to confirm.

  @D-044 @UC-09 @approved:agent @auto
  Scenario: The song follows a MIDI clock: Start plays it, each clock moves it, Stop stops it
    Given a 1-bar song at 90 BPM with C4 recorded near its start
    When MIDI Start and then a clock at 100 BPM come in on the TRS jack
    Then the C4 sounds after Start and again a bar of 96 clocks later, on the clock's time and not the song's
    When MIDI Stop comes in
    Then the song stops

  @D-044 @UC-09 @approved:agent @gate:usb_midi @auto
  Scenario: PLAY sends MIDI Start, the clock and Stop over USB
    Given the companion app, connected over USB
    When PLAY is pressed, and pressed again 0.7 s later
    Then Start goes out, then a clock 24 times a beat at the song's tempo, then Stop

  @D-044 @UC-09 @approved:agent @auto
  Scenario: A MIDI keyboard's sustain pedal, pitch bend and all notes off play on its track
    Given a MIDI keyboard on channel 2 (the bass)
    When it holds C4, presses the sustain pedal and lets C4 go
    Then C4 keeps sounding until the pedal lifts
    When it plays A3 and bends all the way up
    Then B3 sounds, and A3 again when the bend goes back to the middle
    When it sends all notes off
    Then the FM-1 is silent
    And on channel 1 (the chords' piano) the bend works the same

Feature: The settings behind the song page
  GLO opens SONG; SCL steps to CLICK, MIDI and SYSTEM (the owner, Decision #63, D-045: "a sub menu with midi. Not all
  in home visible. Leave home for the main song properties"). ST-17, ST-19, ST-20. Kept over power-off.

  @D-045 @UC-09 @approved:agent @auto
  Scenario: The click on CLICK plays every beat while the song plays
    Given CLICK is set to On
    When PLAY is pressed
    Then a click sounds on each beat, and nothing between

  @D-045 @D-051 @UC-01 @approved:agent @auto
  Scenario: With a count-in, REC counts a bar of clicks before recording
    Given a 1-bar song, COUNT-IN 1 bar and the click Off, the song stopped
    When REC is pressed
    Then four clicks count in, the beats left shown on screen, and then the song starts
    When C4 is played on the song's first beat
    Then it is recorded there and comes round a bar later

  @D-045 @UC-09 @approved:agent @auto
  Scenario: A track's MIDI channel is set on the MIDI page and kept over power-off
    Given CHORDS is set to channel 5 on the MIDI page
    When a note comes in on channel 5, and then one on channel 1
    Then the first plays on the chords and the second plays nothing
    And after a power-off a note on channel 5 still plays the chords

  @D-045 @UC-09 @approved:agent @auto
  Scenario: MIDI IN sends every channel to the track on screen, or follows the track played
    Given MIDI IN This track
    When a note comes in on channel 7
    Then it plays on the track on screen
    Given MIDI IN Follow
    When a note comes in on channel 2
    Then the screen goes to BASS, and the keys then play the bass

  @D-045 @UC-09 @approved:agent @auto
  Scenario: SYSTEM tunes the FM-1 and darkens the screen when nothing is played; a key wakes it and does nothing else
    Given TUNE +50 cents and SCREEN OFF 1 minute
    When C4 is played
    Then it sounds 50 cents sharp
    When nothing is played for a minute
    Then the screen is dark
    When a key is pressed
    Then the screen wakes and nothing sounds
    When a key is pressed again
    Then it plays

Feature: Track templates and voices
  Each track picks a template on GLO's TRACKS page (ST-36, D-046); the help listens to the notes of the other tracks.
  Up to twelve voices within a CPU budget, taken fairly, and a dot when one is taken (ST-37, D-047). Agreed with the
  owner on 2026-11-12.

  @D-046 @UC-07 @approved:owner @auto
  Scenario: Two tracks can share a template
    Given GLO's TRACKS page
    When K1 turns track 1 from CHORDS to MELODY (it has no notes)
    Then track 1 is MELODY 1 at once, track 3 is MELODY 2, and track 1's keys play notes

  @D-046 @UC-04 @approved:owner @auto
  Scenario: A track with notes changes its template only after SAVE, and its notes stay
    Given C4 recorded on track 1 (CHORDS) in a 1-bar loop
    When K1 turns track 1 to BASS on TRACKS
    Then the page asks first, and SAVE changes it
    And the C4 still plays as the loop comes round, and track 1's keys now play the bass

  @D-046 @UC-03 @approved:owner @auto
  Scenario: The help hears every track's notes, whatever its template
    Given Am recorded on track 3, a MELODY track, in a 1-bar loop
    When the bass's HELP is Key+root while the song plays
    Then when the Am comes round, A lights full over the dim scale

  @D-047 @UC-00 @approved:owner @gate:unit @auto
  Scenario: Twelve voices within the CPU budget, taken fairly
    Then eleven voices across four tracks sound at once and nothing is taken
    And with no room a note takes a releasing voice, else the oldest of another track with two or more (the song's before the live ones), else its own oldest
    And a track never loses its last note, and heavy sounds fit fewer voices so the CPU stays under 75 %

  @D-047 @UC-00 @approved:owner @auto
  Scenario: When a note has to take a voice, a dot shows it, and TRACKS shows the voices and the CPU
    When thirteen keys are held
    Then twelve sound, an amber dot shows on the top line for two seconds, and TRACKS reads Voices 12 of 12 with the CPU

Feature: The melody track
  @D-050 @ST-50 @UC-00 @approved:owner @auto
  Scenario: Every template opens on PLAY
    Given a track of any template, CHORDS, BASS, MELODY or DRUMS
    When ALGORITHM moves to it for the first time
    Then it shows its PLAY page

  @D-050 @ST-44 @UC-00 @approved:owner @auto
  Scenario: With HELP Free, the scale's settings are greyed
    Given a melody or bass track on PLAY with HELP Free
    Then the SCALE and OUTSIDE cards are greyed and PLUS is not
    And only the keys being played light
    When SCALE turns
    Then the explanation says HELP is Free, so the scale does nothing now

  @D-050 @ST-26 @UC-11 @approved:owner @auto
  Scenario: The melody starts on the key's pentatonic
    Given a song in C major and a melody track on PLAY with HELP Key
    Then the keys of C D E G A glow, C breathing

  @D-050 @ST-26 @UC-11 @approved:owner @auto
  Scenario: HELP Chord lights the chord now and colours the note played
    Given chords C G Am F recorded, the song playing, a melody track with HELP Chord
    When the Am sounds
    Then A C E light full and the scale's other notes dim
    When E is played
    Then its name shows in green
    When F# is played
    Then its name shows in amber

  @D-050 @ST-26 @UC-11 @approved:owner @auto
  Scenario: The melody page shows the notes held, and the chord they make
    When C4, E4 and G4 are held on the melody's PLAY page
    Then the screen shows C, and under it C E G, with no octave

  @D-050 @ST-29 @UC-11 @approved:owner @gate:unit @auto
  Scenario: PLUS adds a voice in the key, or an echo
    Given C major and PLUS 3rd
    When C, D and E are played
    Then E, F and G sound with them
    When PLUS is 6th and E is played
    Then C above sounds with it
    When PLUS is echo and C is played
    Then C repeats twice, softer, a dotted eighth apart

  @D-050 @ST-49 @UC-11 @approved:owner @gate:unit @auto
  Scenario: The scale list says which scales fit the chords and what the others lack
    Given Am9 and Dm9 recorded in A minor
    When SCALE turns on the bass or the melody
    Then natural minor says fits and dorian says it lacks F

  @D-050 @ST-27 @UC-12 @approved:owner @auto
  Scenario: A melody idea falls onto its keys, in versions
    Given a melody track on IDEAS, the Pentatonic hook, chords C G Am F
    When PLAY is pressed
    Then the hook's notes fall onto their keys and the first time round it plays
    When SELECT turns
    Then another version plays, the same rhythm with other notes

  @D-050 @ST-28 @UC-11 @approved:owner @gate:unit @auto
  Scenario: Holding a pad plays a phrase in a style, call and answer
    Given a melody track on AUTO, style Pop, in C major pentatonic
    When the first pad is held
    Then the song starts and the Pop hook plays over C, its answer the next bar, every note in the scale

Feature: Recording, the same on every track
  @D-051 @ST-40 @UC-13 @approved:owner @auto
  Scenario: Holding REC opens the track's REC layer
    When REC is held
    Then GRID, FEEL, CLICK and COUNT-IN are on the knobs, and OCT+, EDIT and SAVE say what they do
    When REC is pressed while stopped with COUNT-IN 1 bar
    Then four clicks count in, 4 3 2 1, and recording starts on the next beat

  @D-051 @ST-40 @UC-13 @approved:owner @gate:unit @auto
  Scenario: GRID and FEEL change the playback, never the recording
    Given a bassline recorded a little early and late
    When GRID is 1/16
    Then it plays on the sixteenths
    When GRID is back to as played
    Then it plays as it was recorded

  @D-051 @ST-40 @UC-13 @approved:owner @auto
  Scenario: OCT+ in the REC layer clears the track after asking
    When OCT+ is pressed in the REC layer
    Then the screen asks to clear the track
    When OCT+ is pressed again
    Then the track is empty; any other button keeps it

  @D-051 @ST-41 @UC-13 @approved:owner @auto
  Scenario: EDIT and a key erase that key's notes as they play
    Given a melody recorded with two Bb notes
    When EDIT is held with Bb as the song plays past them
    Then both Bb notes are gone and the others stay

  @D-051 @ST-41 @UC-13 @approved:owner @auto
  Scenario: SAVE held undoes the last change
    When a recording, an erase and a clear are each undone with SAVE held
    Then the track is as it was before each of them

  @D-051 @ST-42 @UC-12 @approved:owner @auto
  Scenario: SPEED slows the whole song while practising
    Given a 90 BPM song and a track on IDEAS
    When SPEED is 70 %
    Then every track plays at 63 BPM and the top line shows 63 of 90
    When a note is recorded at that speed and SPEED goes back to 100 %
    Then it plays at its place in the song at 90 BPM

Feature: Explanations
  @D-052 @ST-43 @UC-00 @approved:owner @auto
  Scenario: A knob explains its value, unless explanations are off
    Given explanations are off as the FM-1 starts, and turned on (GLO, SYSTEM)
    When a knob turns
    Then a short explanation of the new value shows for under two seconds
    When explanations are off and a knob turns
    Then no explanation shows

Feature: The drums
  @D-053 @ST-30 @UC-10 @approved:owner @gate:unit @auto
  Scenario: Every key is its own drum sound, named on the screen
    Given a drums track on PLAY with the Dusty kit
    When each of the 27 keys is played
    Then 27 different sounds play and the screen names each one (kick1, snare2, tamb1 ...)

  @D-053 @ST-39 @UC-14 @approved:owner @auto
  Scenario: PRESETS changes the kit; a held key and PRESETS change that key's sound
    When PRESETS turns
    Then the next kit plays and the top line names it
    When a key is held and PRESETS turns
    Then that key takes the next sound of any kit and the top line says My kit

  @D-053 @ST-38 @UC-14 @approved:owner @auto
  Scenario: Each pad keeps its own repeat, tuning and decay
    Given hat1 played last and K1 REPEAT on 1/16
    When hat1 is held for a beat
    Then it plays four times, and kick1 still plays once
    When kick4 of the 808 kit is played and K2 TUNE turns
    Then the screen names the note it sounds, A1 after enough turns

  @D-054 @ST-32 @UC-10 @approved:owner @gate:unit @auto
  Scenario: Holding a pad plays a style's beat, with fills and ghost notes
    Given a drums track on AUTO, style Boom bap
    When the main pad is held
    Then the song starts and the beat plays; letting go stops it
    When FILLS is 4 bars
    Then the fourth bar is the fill
    When GHOSTS is off
    Then no ghost snare plays

  @D-054 @ST-33 @UC-12 @approved:owner @auto
  Scenario: A drum beat falls onto its keys to learn
    Given a drums track on IDEAS, Boom bap, SPEED 70 %
    When PLAY is pressed
    Then the song plays at 63 of 90 BPM and the hits fall onto the keys of their sounds

  @D-054 @ST-31 @UC-10 @approved:owner @auto
  Scenario: The screen draws where the feel puts each hit
    Given a drums track with a beat recorded, GRID 1/16, FEEL Dilla
    Then each hit is drawn where it plays, with a line from its grid position: the snare to the right, the kick swung

  @D-055 @ST-34 @UC-10 @approved:owner @auto
  Scenario: The drums go through Punch, MPC, Lofi or Tape
    When the drums' FX turns to MPC
    Then the drums are compressed and soft-clipped, louder at the same peak

  @D-056 @ST-35 @UC-14 @approved:owner @gate:build @auto
  Scenario: The user kit comes from numbered sound files with a licence
    Given kits/user/01-kick1.wav and 05-snare1.wav and a LICENSE file
    When the firmware is built
    Then the USER kit has kick1 on F3 and snare1 on A3
    When the LICENSE file is missing
    Then the build stops and says why

  @D-057 @ST-48 @UC-17 @approved:owner @gate:unit @auto
  Scenario: A style is on every template
    Then reggae, afrobeats, shatta, reggaeton and drum and bass are styles of the drums, the chord pad, the bass and the melody

Feature: The game: Unicorn Jump
  For a child of four (the owner's daughter): it waits for her, any key in Easy, the lit key in the Key modes; a bump
  for a wrong key, a poop for two, wings for eight and a flight to the moon; a pad and a bass under the bells;
  endless and friendly (the owner, 2026-11-14).

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: The game waits for a key, then the unicorn runs to the first fence and waits there
    Given the Game mode opened (HOME held, Game)
    Then the unicorn waits in a night meadow and every key breathes
    When C4 is played
    Then the unicorn runs to the first fence and stops in front of it, the keys breathing

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: In Easy mode any key jumps the fence and plays its note
    Given K1 Easy and the unicorn waiting at the first fence (Twinkle's C)
    When F3 is played
    Then the unicorn jumps the fence, lands beyond it, and C4 sings
    When A4 is played as the next fence comes
    Then the unicorn dashes to it, jumps it, and C4 sings

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: In Key mode the next fence's key lights; a wrong key bumps and it waits; two wrong, a poop
    Given K1 Key
    Then C4, the first fence's note, lights
    When D4 is played at the fence
    Then the unicorn bumps it and waits on
    When E4 is played
    Then it bumps again, poops (a low buzz, then a plop), and the top line counts one poop
    When C4 is played
    Then it jumps the fence and C4 sings

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: Eight good jumps in Key mode earn the wings, and the unicorn flies when the song ends
    Given K1 Key, Happy Birthday (25 notes) and K2 fast
    Then each fence's note shows as its letter in its colour, over the fence
    When the lit key is played for eight fences in a row
    Then WINGS! shows and the unicorn runs on, in the middle of the song
    When the lit key is played to the song's last fence
    Then YAY, and the unicorn flies away to the moon, smaller and smaller, goes round it and comes back, leaving a rainbow
    And it lands, and the song starts again at its first note

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: A poop comes round again and any key hops it
    Given a poop made at the first fence
    When the unicorn moves on and the poop leaves the screen behind it
    Then the poop comes back ahead in the place of a fence and the unicorn waits in front of it
    When F3 is played
    Then the unicorn hops the poop

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: The song never ends
    Given K1 Easy and Mary's Little Lamb
    When a key is played for every fence
    Then the song done, the unicorn cheers YAY and the song starts again

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: In Easy mode the fences are plain and the unicorn flies when the song ends
    Given K1 Easy, Mary's Little Lamb (26 notes) and K2 fast
    When a key is played for each fence
    Then the unicorn runs with no rainbow behind it, and the fences show no notes
    When it jumps the song's last fence
    Then YAY, and it flies away to the moon with a rainbow behind it
    And no fence comes until it lands; then the song starts again

  @D-059 @ST-51 @UC-18 @approved:agent @auto
  Scenario: The unicorn has its own song
    Given K1 Easy and K3 turned to the Unicorn Song, the last of the built-in songs
    When a key is played at each of the first fences
    Then the fences sing the Unicorn Song: G4, E4, G4

  @D-059 @D-060 @ST-51 @UC-18 @approved:agent @auto
  Scenario: A unicorn left waiting neighs, then falls asleep until a key
    Given the unicorn waiting at a fence with no key played
    Then after 8 s it neighs
    And after 20 s it lies down asleep, z z Z over its head
    When a key is played
    Then it wakes and jumps

  @D-059 @D-060 @ST-51 @UC-18 @approved:agent @auto
  Scenario: With the music on, a pad and a bass play under the bells, and K4 turns them off
    Given the game started with C4 and the unicorn waiting at the first fence
    Then a pad and a bass go on sounding while it waits
    When K4 turns the music off
    Then it is quiet while it waits
    When K4 turns the music on again
    Then the pad and the bass sound again

Feature: Teach the unicorn a song
  The owner, 2026-11-14 (#84): "you teach, the unicorn learns is quite fun. So i can play zelda songs ... I would still
  keep a couple of songs that ppl cant rewrite. REC alone is fine ... the unicorn listens to you, no jumping ... it
  dances or does sth ... When u are finished, u can press save". Taught songs stay on the FM-1, never in the repository.

  @D-061 @ST-52 @UC-18 @approved:agent @auto
  Scenario: A parent teaches the unicorn a song, and it keeps it
    Given the game on MY SONG 4, not taught yet, and the music off
    Then the unicorn waits with TEACH ME
    When REC is pressed
    Then the unicorn listens: no fences, it dances a step to each note and the note floats up to it
    When C4, E4, G4 and A4 are played, K1 is turned back, C5 is played and C6 comes in over MIDI
    Then the unicorn has learned C4 E4 G4 C5 C5 (C6 moved down into the keys)
    When SAVE is pressed
    Then it says LEARNED!
    When the FM-1 is switched off and on, and the game opened on MY SONG 4
    Then the fences sing C4, E4, G4

  @D-061 @ST-52 @UC-18 @approved:agent @auto
  Scenario: The built-in songs cannot be taught over, and leaving while teaching keeps nothing
    Given the game on Twinkle Twinkle
    When REC is pressed
    Then nothing changes: PRESS A KEY
    When K3 goes to MY SONG 4, REC is pressed and two notes are played
    And HOME is held and the game picked again
    Then MY SONG 4 still says TEACH ME

  @D-061 @ST-52 @UC-18 @approved:agent @manual
  Scenario: A taught song's pad and bass follow its key
    Given a song in G major taught to MY SONG 1 on a real FM-1, the music on
    When it is played through
    Then the pad and the bass play G, C and D chords under the bells, and nothing sounds out of key

Feature: The games: choosing one
  The games in the Game mode (the owner, 2026-11-14: "Implement the rest (not the train) in the firmware"; 2026-11-15,
  the Night Train: "Fix the train and u can implement it").

  @D-062 @D-064 @ST-53 @ST-55 @UC-18 @approved:agent @auto
  Scenario: SELECT turns from one game to the next
    Given the Game mode open on Unicorn Jump
    When SELECT is turned up
    Then Hiru Damatxo shows
    When SELECT is turned up again
    Then Aldapeko Sagarra shows
    When SELECT is turned up once more
    Then the Night Train shows
    When SELECT is turned up a fourth time
    Then Unicorn Jump shows again

Feature: The game: Hiru Damatxo
  The three little ladies of Donostia dance on La Concha's promenade to the traditional tune, a jump a note; they wait
  for her (the owner's sketch, 2026-11-14: the dance costume, no castanets, a fall, a seagull).

  @D-062 @ST-53 @UC-18 @approved:agent @auto
  Scenario: In Hiru Damatxo a key dances the next note
    Given Hiru Damatxo, K1 Easy and the music off
    When D4 is played
    Then the first dancer jumps and the flute sings D4

  @D-062 @ST-53 @UC-18 @approved:agent @auto
  Scenario: In Hiru Damatxo a wrong key sits a dancer down, and two bring the seagull
    Given Hiru Damatxo, K1 Key and the music off
    When C4 is played (the first note is D4)
    Then the first dancer sits down hard and the other two giggle behind their hands
    When C4 is played again
    Then a seagull cries, flies in low and poops on her head

  @D-062 @ST-53 @UC-18 @approved:agent @auto
  Scenario: In Hiru Damatxo PLAY plays the song, danced, and its end brings fireworks
    Given Hiru Damatxo
    When PLAY is pressed
    Then the song plays and the dancers dance it
    And at its end the three turn together, fireworks burst over the bay and ZORIONAK! shows

Feature: The game: Aldapeko Sagarra
  A golden late afternoon on a Basque hillside: the robin on the old apple tree sings the traditional tune a note a key,
  the sheep dance its end (the owner's sketch, 2026-11-14).

  @D-063 @ST-54 @UC-18 @approved:agent @auto
  Scenario: In Aldapeko Sagarra the robin sings each note an octave up, in the voice K2 picks
    Given Aldapeko Sagarra, K1 Easy and the music off
    When D4 is played
    Then the robin sings D5, an octave above the key, on the flute
    When K2 is turned to the music box and D4 is played
    Then the robin sings D5 again, in the music box's voice

  @D-063 @ST-54 @UC-18 @approved:agent @auto
  Scenario: In Aldapeko Sagarra a wrong key drops an apple on a sheep, and two make it poop
    Given Aldapeko Sagarra, K1 Key and the music off
    When C4 is played (the first note is D4)
    Then an apple drops on a sheep's head: stars and a baa
    When C4 is played again
    Then that sheep leaves four droppings and the robin hides its eyes

  @D-063 @ST-54 @UC-18 @approved:agent @auto
  Scenario: In Aldapeko Sagarra PLAY plays the song, the sheep dance its end, and TXIRULIRULI!
    Given Aldapeko Sagarra
    When PLAY is pressed
    Then the song plays, and in its last part the sheep dance
    And at its end TXIRULIRULI! shows and the baserri's windows light

Feature: The game: the Night Train
  Old Number 7 steams across the desert at night, a chuff a note: the owner's sketch, 2026-11-15, the one their son
  liked most ("Sneeze is amazing. With maybe the dog barking and the driver kind of angry?"; "the train gets a star").

  @D-064 @ST-55 @UC-18 @approved:agent @auto
  Scenario: The train waits under the moon, and a key is a chuff
    Given the Night Train, K1 Easy and the music off
    Then Old Number 7 waits on the line under the moon, its lamp lit
    When G4 is played
    Then the reed sings G4, the first note of the Railroad, a puff goes up and the engine moves on

  @D-064 @ST-55 @UC-18 @approved:agent @auto
  Scenario: In the train game a wrong key is a cough, and two in a row are a sneeze
    Given the Night Train, K1 Key and the music off
    When C4 is played (the first note is G4)
    Then the engine coughs black smoke
    When C4 is played again
    Then the engine sneezes: ah... ah... CHOO, its front rearing up and the driver's cap flying off
    And the puppy barks, the driver goes red and shakes his fist, and the sooty cloud blows back over them

  @D-064 @ST-55 @UC-18 @approved:agent @auto
  Scenario: K3 changes the train's song to Funiculì, funiculà
    Given the Night Train and the music off
    When K3 is turned
    Then the whistle blows and the top line says FUNICULI
    When A4 is played
    Then the reed sings A4, the first note of Funiculì, funiculà in D

  @D-064 @ST-55 @UC-18 @approved:agent @auto
  Scenario: Eight right notes in a row in Key mode make the golden super train
    Given the Night Train and K1 Key
    When the Railroad's first eight notes are played right
    Then the engine turns gold, rainbow stars come from its stack and a cowboy gallops alongside

  @D-064 @ST-55 @UC-18 @approved:agent @auto
  Scenario: The song's end brings the train to Star Station, where it wins a star it keeps
    Given the Night Train on Funiculì, funiculà
    When PLAY is pressed
    Then the song plays with the train running it
    And at its end the train makes a golden run, then stops at Star Station
    And the sheriff throws a star onto the tender
    When the FM-1 is switched off and on again and the Night Train opened
    Then the star is still on the tender

  @D-064 @ST-55 @UC-18 @approved:agent @manual
  Scenario: The night line's life and sounds, in a play test
    Given the Night Train on a real FM-1, played by a child
    When a song is played through
    Then each line's end blows the whistle and rings the bell
    And along the line go telegraph poles with an owl, a ranch, a camp, a longhorn, prairie dogs and a jackrabbit, and now and then a canyon on a trestle
    And near the song's end the cacti the lamp lights open their flowers

Feature: The games: PLAY lights the keys
  PLAY plays a game's song by itself; each key lights as its note sounds, so a child sees the song before playing it
  (the owner, 2026-11-15: "Maybe just the play button").

  @D-065 @ST-56 @UC-18 @approved:owner @auto
  Scenario: In Unicorn Jump PLAY runs the song by itself, each fence's key lit as its note sounds
    Given Unicorn Jump on Twinkle Twinkle
    When PLAY is pressed
    Then the unicorn runs the song by itself, jumping each fence as its bell sounds
    And C4 lights for the first fence, C4 again for the second, then G4, the other keys dark

  @D-065 @ST-56 @UC-18 @approved:owner @auto
  Scenario: While a game plays its song on PLAY, each note's key lights as the note sounds
    Given Hiru Damatxo, Aldapeko Sagarra or the Night Train
    When PLAY is pressed
    Then the song plays by itself, and each note's key lights as it sounds, the other keys dark

Feature: The guided mode
  Lessons for a grown-up who never made a beat, a step at a time (ST-57, #98). Its own simple controls (#94): K1-K4 the
  mixer, ALGORITHM the tempo, PRESETS the drums, OCT the key, SELECT the step, HOME tapped the lessons. The screen is
  layout E (the owner, 2026-11-16): what to do the largest, the sequencer, the knobs.

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: Guided opens on lesson 1, step 1, and says what to do
    Given the FM-1 just switched on
    When HOME is held, SELECT turned to Guided and HOME pressed twice
    Then the screen shows "1 WHAT'S IN A BEAT" and "1 / 14", "Press PLAY" in amber in the large font, the step's words
    And under them the lesson's song in a 16-step sequencer (kick, snare, hats, bass, chord, melody), and the four knobs

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: A step done shows a green tick and sounds a chime
    Given Guided on step 2, "Turn K1 up"
    When K1 is turned up to 6
    Then the screen shows a green tick and "Done!", and two clicks sound

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: SELECT turns the guided mode's steps
    Given Guided on step 1
    When SELECT is turned one click clockwise, then another, then one back
    Then the screen shows step 2, step 3, then step 2

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: HOME tapped shows the lessons, and HOME opens the chosen one
    Given Guided on step 3
    When HOME is tapped
    Then the lessons show, lesson 1 "WHAT'S IN A BEAT" with its 14 steps and the others coming soon
    When HOME is tapped again
    Then lesson 1 opens at step 1

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: K1 to K4 are the guided mode's mixer
    Given Guided on step 1 and its song playing, only the chords up (K3 at 8)
    When K3 is turned down to 0
    Then the song goes silent

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: The three drum keys play the kick, the snare and the hat on their own
    Given Guided on step 7, just the drums
    Then C4, D4 and E4 light: KICK, SNARE, HAT
    When C4 is played
    Then the kick sounds and the sequencer shows the kick alone

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: REC in lesson 1 says that REC comes in lesson 2
    Given Guided on step 1
    When REC is pressed
    Then the screen says "REC comes in lesson 2" in red and nothing records or sounds

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: A key outside the key is named and said not to be in the key
    Given Guided on step 13, where you play the melody
    Then the keys of A minor light and the black keys stay dark
    When D#4 is played
    Then it sounds and the screen says "D#: not in the key" in red

  @D-068 @ST-57 @UC-19 @approved:owner @auto
  Scenario: The guided mode opens on the lesson and step it was on, over power-off
    Given Guided on step 5
    When the FM-1 is switched off and on again and Guided opened
    Then it opens on step 5 of 14

  @D-068 @ST-57 @UC-19 @approved:owner @manual
  Scenario: Lesson 1, played through by a grown-up who never made a beat
    Given a real FM-1 in Guided mode, and a grown-up who never made a beat, who reads only the screen
    When they go through lesson 1's 14 steps
    Then every step says clearly what to do, and they can do it without help

Feature: Glide and mono (ST-46)
  How a synth track plays its notes (API 27, D-073). The engine part: the screen that sets it is R-237, still open.
  Checked by the unit gate, which measures the pitch in the rendered sound, on the Basic synth and on the engines.

  @D-073 @ST-46 @UC-07 @approved:owner @auto @gate:unit
  Scenario: A key pressed while another is held slides there over the glide time
    Given a synth track with a glide of 200 ms
    When C4 is held and G4 pressed
    Then G4 starts near C4, is between them after 100 ms and at G4 after 200 ms
    And with no glide G4 sounds at once

  @D-073 @ST-46 @UC-07 @approved:owner @auto @gate:unit
  Scenario: A mono track sounds the last key, and goes back to the key still held
    Given a synth track set to mono
    When C4 is held and G4 pressed
    Then one note sounds, G4
    When G4 is let go while C4 is still held
    Then C4 sounds again, without a new attack in legato

Feature: One pattern for the tracks (ST-84)
  @D-074 @ST-84 @approved:owner @auto
  Scenario: The bass and the drums follow the tracks' pattern
    Given the BASS track's IDEAS page
    Then K3 is SPEED, and on PLAY K4 PLUS offers the octave and the fifth
    And on the DRUMS track's AUTO page K3 is FEEL and K4 GHOSTS

Feature: The chords' HELP steps (ST-79)
  @D-075 @ST-79 @approved:owner @auto
  Scenario: The chords' HELP steps from Free to Pads
    Given the CHORDS track's PLAY page on HELP Free
    When K1 is turned three clicks clockwise
    Then HELP shows Key, then Shapes, then Pads

  @D-075 @ST-79 @approved:owner @auto
  Scenario: One note held on Key lights a chord of the key around it
    Given C major on HELP Key
    When A3 is held
    Then C4 and E4 light full, the row lights Am, and SELECT then lights F's notes

  @D-075 @ST-79 @approved:owner @auto
  Scenario: TRANSPOSE whites plays the key on the white keys
    Given E-flat major on HELP Free with TRANSPOSE on whites
    When C4, E4 and G4 are played
    Then E-flat, G and B-flat sound and the screen names Eb

  @D-075 @ST-79 @approved:owner @auto
  Scenario: Shapes lights the chord to play and the next one
    Given HELP Shapes on Pop I-V-vi-IV in C major
    Then C, E and G light full and G, B and D half
    When SELECT is turned one click
    Then G, B and D light full and A, C and E half

Feature: The chord pads and the progressions (ST-80)
  @D-076 @ST-80 @approved:owner @auto
  Scenario: Pads: C plays I and D plays ii in every key
    Given D major on HELP Pads
    When C4 is pressed, then D4 with C#4 held
    Then D major sounds, then Em7, and the screen draws every key with its chord or colour

  @D-076 @ST-80 @approved:owner @auto
  Scenario: PROGRESSION opens a numbered menu
    When K2 PROGRESSION is turned on Pads
    Then a list shows numbered progressions, None, Mine, Follow bass and Follow melody first, each with its style and numerals

Feature: How the pads play a chord (ST-81)
  @D-077 @ST-81 @approved:owner @auto
  Scenario: FEEL pianist plays the bass first and the inner notes softer
    Given HELP Pads with FEEL on pianist
    When a pad is pressed
    Then the lowest note sounds first, the inner notes 10 to 40 ms later and softer, and the top note louder

Feature: Chord ideas (ST-82)
  @D-079 @ST-82 @approved:owner @auto
  Scenario: A chord idea lights its next chord a bar ahead and fills 4 bars
    Given IDEAS on Neo soul, idea 1 (ii-V-I, push)
    When PLAY is pressed
    Then Dm9 lights full and G13 half, and the idea lasts 4 bars, Cmaj7 held for the last two

Feature: Chord auto: song parts (ST-83)
  @D-080 @ST-83 @approved:owner @auto
  Scenario: AUTO plays the style's verse, then the chorus from the next bar
    Given AUTO on Neo soul, stopped
    When G3 (VERSE, calm) is pressed
    Then the verse plays on its own
    When B4 (CHORUS, energy) is pressed
    Then the chorus starts on the next bar, busier and louder

Feature: The sequencer: SONG (ST-85)
  @D-081 @ST-85 @approved:owner @auto
  Scenario: SONG adds a sequence after the cursor, and asks before deleting one
    Given a song of blocks 1, 2 with the cursor on block 1 and the song stopped
    When the key of sequence 3 is pressed
    Then the song is 1, 3, 2
    When EDIT and the key of sequence 3 are pressed
    Then the screen asks "Delete sequence 3?"
    When EDIT and the same key are pressed again
    Then sequence 3 and its block are gone

Feature: The sequencer: SEQUENCE (ST-86)
  @D-082 @ST-86 @approved:owner @auto
  Scenario: A take is undone with SAVE held, and quantize keeps what was played
    Given sequence 1 playing, MELODY recorded with one take
    When QUANTIZE is set to 1/16 and back to off
    Then the notes play on the grid and then as played again
    When SAVE is held
    Then the take is gone

Feature: The sequencer: STEPS (ST-87)
  @D-083 @ST-87 @approved:owner @auto
  Scenario: STEPS writes only with REC on and removes with EDIT
    Given STEPS on DRUMS, the cursor on beat 1 and REC off
    When the kick key is pressed
    Then the kick sounds and nothing is written
    When REC is on and the kick key is pressed
    Then a kick is on beat 1 and its key lights
    When EDIT and the kick key are pressed
    Then the kick is removed

Feature: The song kept over power-off (ST-89)
  @D-085 @ST-89 @approved:owner @auto
  Scenario: The song is there after a power-off
    Given a song of three sequences
    When the FM-1 is switched off and on
    Then the same song plays

Feature: fmini-1's Minimoog engine (ST-70)
  @D-086 @ST-70 @auto
  Scenario: An FMini sound plays its note in tune and stops when released
    Given hello on FMini SINE SOLO
    When C4 is held and let go
    Then C4 sounds in tune, and is silent after its release

  @D-086 @ST-70 @gate:unit @auto
  Scenario: FMini plays the owner's 66 picks, four notes at most a track
    Given each of the 66 FMini sounds the owner picked
    When a note is held and let go
    Then it sounds below the mix's clip and ends after its release, and a fifth note on LUSH STRINGS takes the oldest

Feature: The effects bus (ST-69)

  @D-102 @ST-69 @auto @gate:unit
  Scenario: Delay tone softens the echo
    Given the delay receives a short impulse
    When its tone is changed from brightest to darkest
    Then the first echo has a lower peak at the same delay time
    And the dry impulse keeps its level

  @D-072 @ST-69 @auto
  Scenario: FX opens a live performance page and delay
    Given a synth track is ready to play
    When FX opens PERFORM and another tap opens DELAY
    And K3 raises the delay send
    Then the track's note and its echo sound
    And holding FX returns to PERFORM
    And HOME returns to the track

  @D-103 @ST-69 @auto @gate:unit
  Scenario: Room pre-delay delays only the wet onset
    Given a dry sound and the same sound with a room send
    When the room pre-delay is set to 100 ms
    Then the wet onset moves by 100 ms and the dry sound starts as before

  @D-072 @ST-69 @auto
  Scenario: FX opens a live reverb page
    Given a synth track is ready to play
    When FX steps through PERFORM and DELAY to REVERB
    And the room send and pre-delay are raised
    Then a note has an audible room tail

  @D-104 @ST-69 @auto @gate:unit
  Scenario: Dirt crushes on one track with a dry bypass
    Given DIRT crush is raised on one track
    When samples pass through DIRT at full mix
    Then the output is quantized
    And zero mix passes the same samples unchanged

  @D-072 @ST-69 @auto
  Scenario: FX opens the track dirt page
    Given a synth track is ready to play
    When FX steps through PERFORM, DELAY and REVERB to DIRT
    And the crush amount is raised
    Then the DIRT screen shows the altered transfer curve
    And the track still sounds with full mix and with zero mix

  @D-072 @ST-69 @auto
  Scenario: PERFORM switches live effect pairs
    Given PERFORM is open with DUB selected
    When X and Y are turned and a note is played
    Then the note echoes
    And SELECT steps through SPACE and DIRT

  @D-105 @ST-69 @auto @gate:unit
  Scenario: Performance pairs preserve the track settings
    Given a track with its original effects
    When DUB, SPACE, and DIRT are applied in turn
    Then their audible effects are added while the base settings stay available

  @D-106 @ST-69 @auto @gate:unit
  Scenario: WAH emphasizes its sweep band
    Given WAH sweeps near 1 kHz
    When 100 Hz and 1 kHz tones play at equal input levels
    Then WAH attenuates the 100 Hz tone more strongly

  @D-072 @ST-69 @auto
  Scenario: PERFORM opens WAH one step before DUB
    Given PERFORM shows DUB
    When SELECT moves back one step
    Then WAH shows its SWEEP and SHARPNESS controls

  @D-107 @ST-69 @auto @gate:unit
  Scenario: Wobble sweeps cutoff on one track
    Given WOBBLE is active on the selected track
    When SPEED and DEPTH are changed
    Then the cutoff cycles at that speed and bypasses at zero depth
    And the other track keeps its original samples

  @D-072 @ST-69 @auto
  Scenario: PERFORM opens WOBBLE after DIRT
    Given PERFORM shows DIRT
    When SELECT moves forward one step
    Then WOBBLE shows its SPEED and DEPTH controls

  @D-108 @ST-69 @auto @gate:unit
  Scenario: FMini cutoff edits affect a held note
    Given an FMini sound with a note held
    When its cutoff is closed
    Then the held note becomes quieter without releasing
    And reselecting the sound restores its original filter settings

  @D-072 @ST-69 @auto
  Scenario: PERFORM FILTER edits the current FMini sound
    Given an FMini sound is selected and PERFORM FILTER is open
    When K3 and K4 are turned
    Then FILTER shows the edited cutoff and resonance

  @D-072 @D-110 @ST-46 @auto
  Scenario: EDIT opens the native FMini oscillator and filter
    Given an FMini sound is selected
    When EDIT opens OSC and SELECT moves to FILTER
    Then K1 sets the same native cutoff shown in PERFORM FILTER
    And HOME returns to playing

  @D-110 @ST-46 @auto @gate:unit
  Scenario: FMini oscillator edits affect a held note
    Given oscillator 1 of an FMini sound is playing a note
    When its waveform changes
    Then the held note uses the new waveform

  @D-072 @D-111 @ST-46 @auto
  Scenario: ENV opens the native FMini contours
    Given a Producer track plays an FMini sound
    When ENV is pressed twice
    Then the first press opens F.ENV and the second opens A.ENV
    And the amp release is editable

  @D-111 @ST-46 @auto @gate:unit
  Scenario: FMini envelope edits change the amp release
    Given two notes play with the same FMini attack and sustain
    When the second note has a longer release
    Then the second note keeps sounding longer after it is let go

  @D-112 @ST-46 @auto
  Scenario: VOICE changes the track bend range
    Given a Basic sound is playing
    When VOICE sets BEND RANGE to 12 semitones
    Then full upward pitch bend raises the note by an octave

  @D-072 @D-113 @ST-46 @auto
  Scenario: LFO opens the editable FMini modulation
    Given a Producer track with an FMini sound selected
    When LFO is pressed
    Then the rate, depth, target and waveform can be edited

  @D-113 @ST-46 @auto @gate:unit
  Scenario: FMini LFO changes a held sound
    Given an FMini note held with a square LFO
    When the editable route targets volume
    Then the held note cycles between two levels

  @D-114 @ST-46 @auto
  Scenario: MOD applies a signed cutoff route
    Given FMini MOD routes the wheel to cutoff with a negative amount
    When the modulation wheel is raised during a note
    Then the held note becomes darker
    And clearing the wheel restores the original cutoff

  @D-114 @ST-46 @auto @gate:unit
  Scenario: Wheel offsets preserve base FX sends
    Given the track has base delay and reverb sends
    When signed wheel offsets change those sends
    Then the audible send changes and the base settings remain available

  @D-115 @ST-46 @auto
  Scenario: FILTER shows the FMini spectrum while playing
    Given FMini FILTER is open
    When a note plays on the selected track
    Then its spectrum shows behind the cutoff curve

  @D-115 @ST-46 @auto @gate:unit
  Scenario: Spectrum measures the selected FMini track
    Given a C4 sine plays on the selected track
    When the spectrum window is read
    Then it peaks near the note's fundamental
    And sound from another track does not appear in the selected spectrum

  @D-116 @ST-46 @auto
  Scenario: Sound Edit plots the sounding note's markers
    Given an FMini sound is selected in Sound Edit
    When a note is played across F.ENV, A.ENV and LFO
    Then their graphs show live markers while the voice sounds

  @D-116 @ST-46 @auto @gate:unit
  Scenario: FMini motion reports live contour and phase
    Given a note plays on the selected FMini track
    When the note is released
    Then the contour levels fall as the LFO phase advances


  @D-093 @ST-69 @gate:unit @auto
  Scenario: The effects bus delays, drives and choruses a track's sound by its sends, and ends
    Given a track that sends to the delay, the drive or the chorus
    When it plays a click, a loud square or a tone, then stops
    Then the click repeats after the delay's time, quieter, the square is clipped softly, the chorus adds a moving copy, and all end in silence

Feature: FM-106's Juno-106 engine (ST-71b)
  @D-090 @ST-71b @auto
  Scenario: A Juno-106 sound plays its note in tune and stops when released
    Given hello on Juno-106 Flutes
    When C4 is held and let go
    Then C4 sounds in tune, and is silent after its release

  @D-090 @ST-71b @gate:unit @auto
  Scenario: The Juno-106 plays the owner's 28 picks, five notes at most a track
    Given each of the 28 Juno-106 sounds the owner picked
    When a note is held and let go
    Then it sounds below the mix's clip and ends after its release, and six notes on Synth Pad sound five voices

Feature: The Sound Browser (ST-68)
  @D-087 @ST-68 @auto
  Scenario: PRESETS opens the sound list on the track's group, K1 jumps a group while it shows, and it closes 2 s after
    Given the CHORDS track on PLAY
    When PRESETS is turned, then turned two more, and K1 is turned
    Then the list shows Keys at CLAV SOFT, then HARPSICHORD, then Organ at FULL ORGAN
    When a key is held for 2.4 s and let go
    Then the list stays open while it is held and closes 2 s after, and K1 is then the page's own

Feature: ARP on every track (ST-77, ST-78)
  @D-088 @ST-77 @approved:owner @auto
  Scenario: ARP plays the notes held up in 16ths, and latch keeps it going
    Given MELODY with ARP on, MODE up, RATE 1/16
    When C4, E4 and G4 are held
    Then C4, E4, G4 play in turn in 16ths
    When ARP is tapped twice and the keys let go
    Then the pattern goes on

  @D-088 @ST-78 @approved:owner @auto
  Scenario: ARP on the drums repeats a hat at the rate
    Given DRUMS with ARP on, RATE 1/32 and RAMP crescendo
    When the hat key is held
    Then the hat repeats in 32nds, getting louder

Feature: Bass Something 2 (ST-92)

  @D-095 @ST-92 @auto @gate:unit
  Scenario: The Sound Browser selects each picked Bass Something 2 patch
    Given the owner's eleven Bass Something 2 picks
    When each sound is chosen in the Sound Browser
    Then the track plays the matching upstream factory patch

  @D-095 @ST-92 @auto @gate:unit
  Scenario: Bass Something 2 plays one voice per track
    Given two tracks with Bass Something 2 patches
    When two keys overlap on one track while the other holds a note
    Then the first track sounds one voice and the other keeps its note

  @D-095 @ST-92 @auto
  Scenario: Square Solo plays the selected note and releases it
    Given Square Solo is selected with PRESETS
    When C4 is pressed and released
    Then C4 sounds within 15 cents and its release ends in silence

Feature: OberVave (ST-72)

  @D-096 @ST-72 @auto
  Scenario: OberVave Sync Lead sounds when played
    Given Sync Lead is selected with PRESETS
    When C4 is pressed
    Then the selected preset sounds

  @D-097 @ST-72 @auto
  Scenario: Selecting Shadow Arp enables the track arpeggiator
    Given the Sound Browser is open on the track's sounds
    When Shadow Arp is selected and C4 is held
    Then the track plays its factory arpeggio

Feature: CoralFlower VA (ST-73)

  @D-098 @ST-73 @auto
  Scenario: CoralFlower Punch Bass sounds when played
    Given Punch Bass is selected with PRESETS
    When C4 is pressed
    Then the selected preset sounds

  @D-098 @ST-73 @auto @gate:unit
  Scenario: The Sound Browser selects each picked CoralFlower VA patch
    Given the nine selected CoralFlower sounds in the browser
    When a selected sound is loaded
    Then the matching CoralFlower factory patch is used

Feature: FMini factory arpeggios (ST-77)

  @D-099 @ST-77 @auto @gate:unit
  Scenario: FMini Pluck Arp plays through the track arpeggiator
    Given Pluck Arp is selected for the melody track
    When C4 is held
    Then the track plays the factory's up-down pattern across its three octaves

  @D-117 @ST-46 @auto
  Scenario: Juno EDIT FILTER shares the active cutoff
    Given a Juno-106 sound on the selected track
    When I turn cutoff on EDIT FILTER with a note held
    Then the cutoff shown on PERFORM FILTER changes

  @D-117 @ST-46 @auto @gate:unit
  Scenario: Juno native filter edits affect a held note
    Given the selected track plays a held Juno-106 note
    When I sample the selected track spectrum
    Then the bands show the held note's energy

  @D-118 @ST-46 @auto
  Scenario: Juno envelope edits appear on both pages
    Given a Juno-106 sound on the selected track
    When I edit release on F.ENV
    Then A.ENV shows the same release

  @D-118 @ST-46 @auto @gate:unit
  Scenario: Juno reports its shared native envelope
    Given a selected Juno-106 voice is releasing
    When I read its shared native envelope
    Then both envelope levels follow the voice

  @D-119 @auto
  Scenario: Sound Edit opens the preset save panel
    Given a pitched sound on the selected Producer track and Sound Edit open
    When I press SAVE
    Then the preset save options appear with Cancel selected
    When I choose Save As and confirm a name
    Then the saved preset is offered an explicit overwrite action
