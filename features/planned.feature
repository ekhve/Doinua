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

Feature: The DRUMS track as a mini MPC (ST-95 to ST-101, approved by the owner 2026-11-26, from the Sampler Lab v11)






  @ST-99 @UC-21 @planned @approved:owner
  Scenario: TAKE records from the input on sound and stops on REC
    Given MY KIT 2, LFO tapped, the input silent
    When REC is tapped and the input plays a kick 1 s later
    Then the recording starts on the kick, and REC tapped stops it
    And a take that reaches 11 s stops by itself
    When REC is tapped again before the take was put on a key
    Then the FM-1 asks before replacing it

  @ST-99 @UC-21 @planned @approved:owner
  Scenario: CHOP cuts the take, splits and joins slices, and the keys play them
    Given a take of a two-bar break on CHOP
    When K4 is set to hits
    Then the slices start at the hits, and F3 plays slice 1, F#3 slice 2
    When slice 3 plays and OCT+ is pressed
    Then slice 3 is split at the play line
    When OCT- is pressed
    Then it is joined to the slice before it
    And SELECT chooses the next slice and plays it

  @ST-99 @UC-21 @planned @approved:owner
  Scenario: A slice goes on a key with two presses, or every slice from a key up
    Given CHOP on MY KIT 2 with one slice
    When REC is tapped and F3 is pressed
    Then nothing sounds and the screen says F3 · KICK: press again to store
    When F3 is pressed again
    Then F3 plays the slice and the screen says Stored on F3
    When K4 PUT is on each and F4 is pressed twice
    Then each slice is on the keys from F4 up
    And on a built-in kit the FM-1 asks which MY KIT, and a kit past 7 s stays as it was and says it is full

  @ST-99 @UC-21 @planned @approved:owner
  Scenario: TRIM shortens a sample and makes it as loud as it goes
    Given a quiet hat sample on EDIT TRIM
    When K4 GAIN turns up
    Then it stops at MAX with the hat's peak at 0 dBFS
    When K2 END turns down
    Then the hat is shorter, and once its take is gone END cannot go past what was kept

  @ST-100 @UC-22 @planned @approved:owner
  Scenario: ENV plays a pad on 16 levels, or as notes in the song's scale
    Given a sung A3 on F4, the song in C minor, ENV pressed on the FX page
    Then NOTES shows 16 LEVELS, and the white keys play it from 6 % to 100 %
    When K2 is tune
    Then F3 plays it 7 semitones down and G5 8 up
    When K1 is KEY
    Then C4 plays C4 with the A3 sample, the keys of C minor are lit and the Cs in full
    When K1 is WHITES
    Then the white keys play C minor from C4

  @ST-100 @UC-22 @planned @approved:owner
  Scenario: ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads
    Given the sung note's ENV, ENV pressed again, the song in C minor, progression i VI III VII
    When C4 is played
    Then it plays C minor with the sample
    When C#4 is held and D4 is played
    Then it plays the D chord with its 7th
    When F5, G5 and F#5 are played
    Then F5 plays the next chord, G5 the last one again, and F#5 keeps the colour
    And with REC on, each note of the chords is recorded on the drums track with its pitch

  @ST-101 @UC-22 @planned @approved:owner
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
