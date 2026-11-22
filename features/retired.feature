# Scenarios of retired requirements (docs/REQUIREMENTS-RETIRED.md); not run, not traced.

Feature: Manual chords
  (Since D-042 the manual page is the CHORDS track's IDEAS page: SCL steps PLAY, IDEAS, AUTO.)
  The CHORDS track's MANUAL page (concept v0.4 §2, docs/MANUAL.md): the knobs build a chord, the keys and the screen's
  piano show it, you play it; SELECT walks the ideas, ARP arpeggiates, and what is recorded breathes as it plays.

  # MC-1, MC-2: build a chord and play it, in the key or outside it
  @D-036 @retired
  Scenario: The knobs build a chord that the keys and the screen show
    Given the companion app is on CHORDS, IDEAS (the manual page), in C major
    When K3 is turned to 7
    Then C7's keys light full (C4, E4, G4, Bb4), the key's other notes glow dim, and the piano shows them
    When K1 is turned to ii
    Then Dm7's keys light full
    When D4, F4, A4 and C5 are held
    Then the screen names them Dm7, with a tick
    When K1 is turned on past vii
    Then the roots outside the key come, keeping vii's type: Dbm7b5 lights, marked as outside the key
    And the device's screens are the host simulator's, pixel for pixel

  # MC-2: the key is GLO's, or AUTO's from what is recorded
  @D-036 @retired
  Scenario: The manual page takes its key from what is recorded
    Given the song is empty, its key AUTO, so the manual page is in C major
    When Am is built and recorded as played (REC, PLAY, A3 C4 E4 held)
    And the song is stopped and K1 turned back to its first chord
    Then the key is A minor and K1's first chord is Am: A3, C4 and E4 light full

  # MC-3: play without help
  @D-036 @retired
  Scenario: EDIT switches the lights off and back on
    Given the manual page with C built
    When EDIT is pressed
    Then the keys' lights are off
    When EDIT is pressed again
    Then C's keys light full and the key's notes dim again

  # MC-6: walk the ideas
  @D-036 @retired
  Scenario: SELECT walks the ideas of the chord pad's progression
    Given the chord pad's progression is Neo soul ii-V-I
    When SELECT is turned right on the manual page
    Then the knobs take Dm9 and its keys light
    When SELECT is turned right again, then left
    Then G13, then Dm9 again
    When Pop is chosen on the chord pad and SELECT is turned right on the manual page
    Then the knobs take its first chord, C

  # MC-7: arpeggiate
  @D-036 @retired
  Scenario: ARP plays the held notes one at a time at the song's tempo
    Given the manual page with ARP on
    When C4, E4 and G4 are held
    Then C4, E4, G4 and C4 again play one at a time, an eighth note each at 90 BPM

  # MC-4, MC-5: layer live; see chords made on the pad
  @D-036 @retired
  Scenario: Chords made on the pad breathe on the manual page as the song plays
    Given Dm9 recorded on the chord pad as the song played
    When SCL goes to the manual page and the song comes round
    Then Dm9's keys breathe beside the chord built, which stays full
    And the screen says now Dm9, and the loop's line at the top shows it where it was played, on the manual page as on the pad


Feature: The chord pad on AUTO (retired by ST-83)
  @D-034 @retired
  Scenario: One finger plays a style's progression with its colours
    Given the companion app has started and SCL has opened the chord pad
    And the style is Neo soul with its usual progression, ii-V-I, in C major
    Then the ii pads (G3, G4) breathe as the next chord, the other pads glow dim and AUTO (F#5) is lit
    When F5 (NEXT) is pressed
    Then Dm9 sounds, its pads light full and the V pads (C4, C5) breathe
    When F5 is pressed again
    Then G13 sounds, the V pads light full and the I pads (F3, F4) breathe
    When F5 is pressed again
    Then Cmaj7 sounds, the I pads light full and the ii pads breathe
    And the device's screens are the host simulator's, pixel for pixel
    # CP-4: K4 changes how the chord is played
    When K4 is turned to Arp and F5 is held
    Then Dm9's notes play one at a time, up, an eighth note each at 90 BPM: D3, F3, A3, C4

  @D-034 @retired
  Scenario: A held black key adds its colour and AUTO locks it
    Given the chord pad, Neo soul ii-V-I, in C major
    When 13 (G#4) is held and the iii pad (A3) is pressed
    Then Em13 sounds and G#4 lights while it is held
    When 9 (D#4) is held and AUTO (F#5) is tapped
    Then 9 is locked: D#4 stays lit when it is let go and AUTO breathes
    When the IV pad (B3) is pressed
    Then F9 sounds
    When AUTO is tapped again
    Then the lock is off: D#4 goes dark and AUTO is lit

  @D-034 @retired
  Scenario: Chords played on the pad are recorded as played and light their pads as the song plays
    Given the chord pad, and the song of 4 bars at 90 BPM, empty
    When REC and PLAY are pressed, Dm9 is played with NEXT and held about a second, and REC is pressed again
    And then the iii pad is played, not recorded
    Then when the song comes round, Dm9 sounds with no key pressed
    And the ii pads light full while it sounds, beside the iii pad just played
    And it is silent before it comes round
