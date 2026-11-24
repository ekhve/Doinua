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
