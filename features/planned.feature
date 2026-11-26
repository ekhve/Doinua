# fm1-scales — Planned scenarios (agreed, not built yet)
# These are the acceptance scenarios of stories ST-26 to ST-50 (docs/STORIES.md), from the melody and drum labs and the
# owner's reviews of 2026-11-12 and 13. When a story is built its scenarios move to fm1-scales.feature with @auto and an
# executable check; until then traceability does not read this file. Requirements: docs/REQUIREMENTS-PLANNED.md.

Feature: The drums (planned)
Feature: Ideas from MIDI files and the styles (planned)
  @ST-45 @UC-15 @planned @approved:owner
  Scenario: MIDI files become ideas and AUTO patterns in any key
    Given ideas/melody/reggae/one-drop.mid in A minor and a LICENSE file
    When the firmware is built and the song is in D minor
    Then the reggae idea plays in D minor on IDEAS and on AUTO

Feature: The chords track (ST-79 to ST-83, drafts for the owner)









Feature: One pattern for the tracks (ST-84, draft for the owner)

Feature: The sequencer (ST-85 to ST-91, drafts for the owner)



  @ST-88 @planned @approved:owner
  Scenario: A knob turned while recording becomes a MOTION lane
    Given REC on while sequence 1 plays
    When CUTOFF is turned on a sound page
    Then MOTION lists CUTOFF with its values in Hz, and EDIT tapped on it clears it

  @ST-90 @planned @approved:owner
  Scenario: Stutter on the melody while the drums keep playing
    Given a beat with drums and melody playing
    When FX is held on PERFORM, K1 TARGET set to MELODY, and the stutter key held
    Then the melody stutters and the drums play on

  @ST-91 @planned @approved:owner
  Scenario: GLO held explains the page, and the top line names the sequence
    Given the MELODY track's PLAY page with REC armed on sequence 2
    Then the top line shows SEQ 2
    When GLO is held
    Then the page's actions show

Feature: The DRUMS track as a mini MPC (ST-95 to ST-101, drafts for the owner, from the Sampler Lab v11)

  @ST-95 @UC-21 @planned @draft
  Scenario: The drum map puts AUTO's sounds on the first octave
    Given a drums track on Dusty
    When F3, F#3 and G3 are played, then F4
    Then they sound the kick, the hat and the snare, and the screen names them KICK, HAT, SNARE, then F4's sound name
    And AUTO's beat plays its kick on F3, its snare on G3 and its hats on F#3

  @ST-96 @UC-10 @planned @draft
  Scenario: PLAY sets the key's level and reverb, and the track's grid and feel
    Given a drums track's PLAY page, the snare played last
    When K1 and K2 turn
    Then the snare is quieter and sends to the reverb, and the kick is unchanged
    When K3 sets GRID 1/16 and K4 sets FEEL MPC 66
    Then the top of the page shows GRID 1/16 and FEEL MPC 66

  @ST-96 @UC-13 @planned @draft
  Scenario: GRID places notes as they are recorded, and FEEL moves them afterwards
    Given GRID 1/16, REC on and the song playing
    When the kick is played 20 ms after a sixteenth, then the snare and the hat
    Then the kick is recorded on that sixteenth, and the bar draws a row for each key in that order
    When FEEL is set to Swing 62
    Then the hats on the off-sixteenths play later, and the kick on the beat does not move

  @ST-96 @UC-13 @planned @draft
  Scenario: Erasing needs REC on, and SAVE undoes and redoes
    Given a recorded beat playing with REC off
    When EDIT and the hat key are held
    Then the hats stay and the screen says REC on to erase
    When REC is on and EDIT and the hat key are held for a bar
    Then the hats are gone
    When SAVE is held
    Then the hats are back
    When SAVE is tapped twice within 400 ms
    Then the hats are gone again, and a single tap shows HOLD: UNDO

  @ST-96 @UC-14 @planned @draft
  Scenario: EDIT builds the kit from every sound, grouped by kit
    Given MY KIT 2, F4 played last, EDIT tapped
    Then the KIT page shows
    When PRESETS turns
    Then F4 plays the next sound of the list, and its settings are the defaults
    When K3 turns
    Then F4 plays the first sound of the next kit in the list
    When EDIT is tapped three times
    Then SOUND, TRIM and PAD FX show in turn

  @ST-96 @UC-21 @planned @draft
  Scenario: SOUND shapes a key with tune, attack, hold and decay, and mute groups
    Given EDIT SOUND on the snare
    When K3 HOLD and K4 DECAY turn down
    Then the snare is shorter, and no sound plays while the knobs turn
    When SELECT sets the open hat and the hat to group A, and the open hat then the hat are played
    Then the hat stops the open hat within 10 ms
    When HOLD is set to key on a sung note and its key is held for 2 s
    Then it sounds for 2 s and then fades over its DECAY

  @ST-96 @UC-14 @planned @draft
  Scenario: A kit is saved as MY KIT 1, 2 or 3 and kept
    Given Dusty with the snare's sound changed
    When SAVE is held on EDIT, K1 picks MY KIT 3 and SAVE is held again
    Then PRESETS reaches MY KIT 3 after Latin, with the changed snare
    And after power-off MY KIT 3 still has it

  @ST-97 @UC-21 @planned @draft
  Scenario: The kit's effects run once, and each pad sends to them
    Given FX PUNCH with COMP 6, FX COLOR with TAPE 5 and LOFI 3, FX SPACE with REVERB plate
    When the hats' COLOR send is 0 and the snare's REVERB send is 7 on EDIT PAD FX
    Then the kick and snare are compressed and coloured, the hats are clean, and only the snare rings in the plate
    When COMP and CLIP are 0
    Then PAD FX shows PUNCH off
    And the CPU gate stays under 80 % with eight keys sending to the four effects

  @ST-98 @UC-21 @manual @planned @draft
  Scenario: The FM-1 is a USB audio device both ways
    Given the FM-1 connected to a phone by USB
    When a beat plays
    Then the phone records it in stereo
    When the phone plays and GLO USB AUDIO's MONITOR is when sampling
    Then the FM-1 plays the phone only on TAKE and CHOP, and K3 sets its level from -12 dB to +24 dB

  @ST-99 @UC-21 @planned @draft
  Scenario: TAKE records from the input on sound and stops on REC
    Given MY KIT 2, LFO tapped, the input silent
    When REC is tapped and the input plays a kick 1 s later
    Then the recording starts on the kick, and REC tapped stops it
    And a take that reaches 11 s stops by itself
    When REC is tapped again before the take was put on a key
    Then the FM-1 asks before replacing it

  @ST-99 @UC-21 @planned @draft
  Scenario: CHOP cuts the take, splits and joins slices, and the keys play them
    Given a take of a two-bar break on CHOP
    When K4 is set to hits
    Then the slices start at the hits, and F3 plays slice 1, F#3 slice 2
    When slice 3 plays and OCT+ is pressed
    Then slice 3 is split at the play line
    When OCT- is pressed
    Then it is joined to the slice before it
    And SELECT chooses the next slice and plays it

  @ST-99 @UC-21 @planned @draft
  Scenario: A slice goes on a key with two presses, or every slice from a key up
    Given CHOP on MY KIT 2 with one slice
    When REC is tapped and F3 is pressed
    Then nothing sounds and the screen says F3 · KICK: press again to store
    When F3 is pressed again
    Then F3 plays the slice and the screen says Stored on F3
    When K4 PUT is on each and F4 is pressed twice
    Then each slice is on the keys from F4 up
    And on a built-in kit the FM-1 asks which MY KIT, and a kit past 7 s stays as it was and says it is full

  @ST-99 @UC-21 @planned @draft
  Scenario: TRIM shortens a sample and makes it as loud as it goes
    Given a quiet hat sample on EDIT TRIM
    When K4 GAIN turns up
    Then it stops at MAX with the hat's peak at 0 dBFS
    When K2 END turns down
    Then the hat is shorter, and once its take is gone END cannot go past what was kept

  @ST-100 @UC-22 @planned @draft
  Scenario: ENV plays a pad on 16 levels, or as notes in the song's scale
    Given a sung A3 on F4, the song in C minor, ENV pressed on the FX page
    Then NOTES shows 16 LEVELS, and the white keys play it from 6 % to 100 %
    When K2 is tune
    Then F3 plays it 7 semitones down and G5 8 up
    When K1 is KEY
    Then C4 plays C4 with the A3 sample, the keys of C minor are lit and the Cs in full
    When K1 is WHITES
    Then the white keys play C minor from C4

  @ST-100 @UC-22 @planned @draft
  Scenario: ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads
    Given the sung note's ENV, ENV pressed again, the song in C minor, progression i VI III VII
    When C4 is played
    Then it plays C minor with the sample
    When C#4 is held and D4 is played
    Then it plays the D chord with its 7th
    When F5, G5 and F#5 are played
    Then F5 plays the next chord, G5 the last one again, and F#5 keeps the colour
    And with REC on, each note of the chords is recorded on the drums track with its pitch

  @ST-101 @UC-22 @planned @draft
  Scenario: A take shows its note, key and tempo, and fits the song
    Given a piano C#3 8 cents sharp recorded on TAKE
    Then TAKE shows NOTE C#3 and its cents
    When it is on a key and K1 TUNE turns down on EDIT SOUND twice
    Then it plays C#3, then C3
    Given a chord loop in A minor at 95 BPM recorded on TAKE
    Then TAKE shows KEY A minor and 95 BPM, and SELECT asks to make A minor the song's key
    When FIT is speed and the take is put on a key
    Then it plays at the song's 90 BPM, lower
    When FIT is slices and each slice is put on keys
    Then the FM-1 offers to write the first bar as notes at 90 BPM
