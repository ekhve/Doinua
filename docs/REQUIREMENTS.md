# fm1-scales — Requirements

**Companions:** `REQUIREMENTS-PLAYBOOK.md` (how requirements are written and checked), `USE_CASES.md` (what people
do), `HAZARDS.md` (what could hurt a child or an FM-1), `../features/fm1-scales.feature` (how we check),
`PLATFORM.md` (decisions D-nnn: why), `ARCHITECTURE.md` and `INTERFACES.md` (how it is built),
`TRACEABILITY.md` (generated: every link in one table), `../DONE.md` (when it's finished).

**How to read this.** Each requirement is one EARS statement with one `shall`, either true or false of the
product. **Source** is what it came from: a use case (UC-nn), a hazard (HAZ-nn) or a decision (D-nnn, which
carries the rationale). **Mode** is how it is verified. **Status** is who approved it. **Verified by** is the exact
title of a scenario in `features/fm1-scales.feature`, or a note in parentheses for inspection.

Two gates keep this table honest. `factory/reqlint.py` checks the wording against the playbook's rules.
`factory/trace.py` checks the links: every title exists, every scenario is cited, every automated scenario is run,
and every source exists.

**Verification modes:** **A** automated (Test: the factory, on the host simulator and the emulated firmware, or a
named gate); **H** on a real FM-1 (Demonstration); **K** a play test with a child (Demonstration); **I** inspection;
**J** judgement (a question a person answers, `DONE.md`).

**Approval** (playbook section 2): platform and technical requirements are approved by the factory lead (the agent)
on the owner's delegation of 2026-11-06. A requirement that changes what a child sees, hears or learns is approved
by the owner. A requirement stays **Draft** until then.

---

## 1. Playing and seeing

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-001 | When a piano key is pressed, the FM-1 shall sound the key's note within 15 cents of its equal-tempered pitch. | D-004, HAZ-08 | A | Approved (agent) | A key plays its note in tune and the screen names it |
| R-021 | When a piano key is released, the FM-1 shall silence the key's note within 200 ms. | D-004, HAZ-02 | A | Approved (agent) | A key plays its note in tune and the screen names it |
| R-002 | The firmware shall draw the same pixels as the host simulator for the same input script, compared at the same millisecond. | D-004 | A | Approved (agent) | A key plays its note in tune and the screen names it |
| R-003 | While a scale is selected, the hello app shall light the keys of that scale at the dim level. | D-004 | A | Approved (agent) | The keys of the scale glow, the tonic breathes, and they follow the key |
| R-022 | While a scale is selected, the hello app shall light the keys of its tonic at the breathing level. | D-004 | A | Approved (agent) | The keys of the scale glow, the tonic breathes, and they follow the key |
| R-023 | While a piano key is held, the hello app shall light that key at the full level. | D-004 | A | Approved (agent) | The keys of the scale glow, the tonic breathes, and they follow the key |
| R-024 | When the key changes, the hello app shall light the keys of the new scale within 300 ms. | D-004 | A | Approved (agent) | The keys of the scale glow, the tonic breathes, and they follow the key |
| R-004 | While an engine preset is selected, when a piano key is pressed, the FM-1 shall sound the pitch listed for that preset and key in `docs/PLATFORM.md` within 25 cents. | D-014 | A | Approved (agent) | Every engine's first preset plays its note |
| R-054 | The FM-1 shall sound notes on four tracks at once, each track with its own engine and preset. | D-029 | A | Approved (agent) | Four tracks sound at once with their own sounds |
| R-055 | When the eight voices are in use, the FM-1 shall take a voice of the chords track for a new bass, melody or drums note. | D-029 | A | Approved (agent) | The four tracks share eight voices, chords first |
| R-056 | When the eight voices are in use, the FM-1 shall keep the voices of the bass, melody and drums tracks for them. | D-029 | A | Approved (agent) | The four tracks share eight voices, chords first |
| R-057 | When a note arrives on the drums track, the FM-1 shall play a kick, a snare or a hi-hat by the note's pitch class. | D-029 | A | Approved (agent) | Four tracks sound at once with their own sounds |
| R-063 | The song shall keep each recorded note's start, length and velocity as played. | D-031 | A | Approved (agent) | A recorded performance plays back as it was played |
| R-064 | While a track is armed and the song plays, the recorder shall add each note played on that track to it. | D-031 | A | Approved (agent) | A recorded performance plays back as it was played |
| R-065 | When a chord is step-entered at a bar, the song shall hold the chord's notes for that whole bar. | D-031 | A | Approved (agent) | A recorded performance plays back as it was played |
| R-066 | While the song plays, the song player shall play the notes of the four tracks at their ticks, at the song's tempo, looping at the song's length. | D-031 | A | Approved (agent) | A song plays its four tracks, looping, and stops |
| R-067 | While the song loops, the song player shall start loop n within one main-loop pass of n loop lengths after the song started. | D-031 | A | Approved (agent) | A recorded performance plays back as it was played |
| R-068 | The song shall keep its key as AUTO or locked, its tempo, and its length of 1 to 8 bars of four beats. | D-031, D-027 | A | Approved (agent) | A recorded performance plays back as it was played |
| R-069 | When the song player stops, the song player shall release the notes sounding. | D-031, HAZ-02 | A | Approved (agent) | A song plays its four tracks, looping, and stops |
| R-070 | While the song's key is AUTO, the song shall report the key that the notes of its bass, melody and chords tracks rank first, and whether that ranking is ambiguous. | D-032, D-030 | A | Approved (agent) | The song's settings never change what was played |
| R-071 | When the song's key is locked or set back to AUTO, the song shall keep its notes unchanged. | D-032 | A | Approved (agent) | The song's settings never change what was played |
| R-072 | When the song is transposed, the song shall move the notes of its chords, bass and melody tracks and its locked key by the same interval. | D-032 | A | Approved (agent) | The song's settings never change what was played |
| R-073 | When the tempo changes while the song plays, the song player shall continue from the same point of the loop at the new tempo. | D-032 | A | Approved (agent) | The song's settings never change what was played |
| R-074 | When the song is shortened, the song shall keep the notes beyond its new end without playing them. | D-032 | A | Approved (agent) | The song's settings never change what was played |
| R-044 | When the player starts a sequence, the player shall start each note one step after the previous one, within 2 ms. | D-025 | A | Approved (agent) | The player keeps time and note length |
| R-045 | The player shall sound each note for 92 % of a step. | D-025 | A | Approved (agent) | The player keeps time and note length |
| R-046 | When PLAY is pressed, the hello app shall play its scale over one octave in the selected pattern, looping, at the selected tempo. | D-025 | A | Approved (agent) | PLAY plays the scale up at the tempo |
| R-047 | When PLAY is pressed during playback, the hello app shall stop the playback within 100 ms. | D-025, HAZ-02 | A | Approved (agent) | PLAY again stops the playing |
| R-048 | While the hello app plays, the hello app shall light the key of the sounding note at the full level. | D-025 | A | Approved (agent) | PLAY plays the scale up at the tempo |
| R-049 | When OCT- or OCT+ is pressed, the hello app shall move the keys down or up one octave, at most two octaves from the start. | D-025 | A | Approved (agent) | OCT- and OCT+ move the keys at most two octaves |
| R-050 | When KNOB1 is turned, the hello app shall select the next pattern in the turn's direction. | D-025 | A | Approved (agent) | The patterns change the order |
| R-051 | When KNOB2 is turned, the hello app shall change the tempo by 5 BPM per click, within 40 to 240 BPM. | D-025, D-048 | A | Approved (agent) | The patterns change the order |
| R-052 | When an app draws an image from its assets, the firmware shall draw the image's pixels at the given position and scale, leaving its transparent pixels unchanged. | D-028 | A | Approved (agent) | The firmware draws the concept's game and study screens |
| R-053 | The firmware shall draw an image only inside the screen, whatever its position. | D-028 | A | Approved (agent) | The firmware draws images inside the screen only |
| R-234 | While a synth track's sound has a glide time above zero, when a key is pressed while another key on that track is held, the FM-1 shall slide the pitch from the held note to the new one over the glide time. | D-073, UC-07, ST-46 | A | Approved (owner) | A key pressed while another is held slides there over the glide time |
| R-235 | While a synth track's sound is set to mono, the FM-1 shall sound at most one note at a time on that track: the last key pressed. | D-073, UC-07, ST-46 | A | Approved (owner) | A mono track sounds the last key, and goes back to the key still held |
| R-236 | When a held key is released on a mono track while another key is still held, the FM-1 shall go back to the note still held. | D-073, UC-07, ST-46 | A | Approved (owner) | A mono track sounds the last key, and goes back to the key still held |

## 2. Input

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-005 | When a key in the range F3 to C5 is pressed, the FM-1 shall set the level of its note in proportion to the press velocity, within 10 %. | D-017 | A | Approved (agent) | Velocity comes from how hard a key is pressed |
| R-006 | When a key in the range F3 to C5 is pressed harder, the FM-1 shall play its note at a higher level. | D-017 | H | Approved (agent) | Velocity feels right on a real FM-1 |
| R-007 | When a note-on message arrives on the TRS MIDI input, the FM-1 shall sound that note within 15 cents of its equal-tempered pitch. | D-020 | A | Approved (agent) | Notes from a MIDI keyboard on the TRS jack play and stop |
| R-025 | When a note-off message arrives on the TRS MIDI input, the FM-1 shall silence that note within 200 ms. | D-020, HAZ-02 | A | Approved (agent) | Notes from a MIDI keyboard on the TRS jack play and stop |
| R-026 | The FM-1 shall apply MIDI running status to data bytes that arrive without a status byte. | D-020 | A | Approved (agent) | Notes from a MIDI keyboard on the TRS jack play and stop |
| R-027 | If a byte in the range 0xF8 to 0xFF arrives between the bytes of a MIDI message, then the FM-1 shall complete the MIDI message as if that byte were absent. | D-020 | A | Approved (agent) | Notes from a MIDI keyboard on the TRS jack play and stop |
| R-008 | When a piano key is pressed, the FM-1 shall send a note-on message carrying the press velocity over USB-MIDI within 20 ms. | D-020 | A | Approved (agent) | Keys go out over USB-MIDI and notes from a computer come in |
| R-028 | When a piano key is released, the FM-1 shall send a note-off message over USB-MIDI within 20 ms. | D-020 | A | Approved (agent) | Keys go out over USB-MIDI and notes from a computer come in |
| R-029 | When a note-on message arrives on the USB-MIDI input, the firmware shall deliver a MIDI note event to the app. | D-020 | A | Approved (agent) | Keys go out over USB-MIDI and notes from a computer come in |

## 3. Remembering

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-009 | When the FM-1 starts, the store shall return the record that the app saved last. | D-019, HAZ-06 | A | Approved (agent) | Settings survive a power-off |
| R-010 | If the power fails during a save, then the store shall keep a whole record. | D-019, HAZ-06 | A | Approved (agent) | A power cut during a save keeps a whole record |
| R-020 | The store shall erase only blank sectors and sectors that start with its marker. | D-022, HAZ-05 | A | Approved (agent) | The store never erases another firmware's data |
| R-030 | If no candidate sector is free, then the store shall refuse the save without changing the flash. | D-022, HAZ-05 | A | Approved (agent) | The store never erases another firmware's data |
| R-039 | When an app saves a record, the store shall write it to flash no sooner than 2 s after that app's last save. | HAZ-06, D-023 | A | Approved (agent) | The store paces its flash writes |
| R-040 | The store shall leave at least 60 s between two flash writes. | HAZ-06, D-023 | A | Approved (agent) | The store paces its flash writes |
| R-041 | While a key is held or a voice sounds, the store shall postpone its flash write. | HAZ-07, D-023 | A | Approved (agent) | The store paces its flash writes |
| R-042 | If a saved record equals the stored record, then the store shall skip the flash write. | HAZ-06, D-023 | A | Approved (agent) | The store paces its flash writes |

## 4. Staying updatable

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-011 | When the installer's handshake arrives over USB-MIDI, the firmware shall answer it within 20 ms. | D-006, HAZ-04 | A | Approved (agent) | The installer can always replace the firmware |
| R-031 | When the UBOOT soft key arrives over USB-MIDI, the firmware shall enter update mode within 200 ms. | D-006, HAZ-04 | A | Approved (agent) | The installer can always replace the firmware |
| R-012 | When OCT- and OCT+ have been held together for 5 s, the firmware shall enter update mode. | D-018, HAZ-04 | A | Approved (agent) | Holding OCT- and OCT+ for 5 seconds enters update mode, and not sooner |
| R-032 | While OCT- and OCT+ have been held together for less than 5 s, the firmware shall keep running the app. | D-018, HAZ-04 | A | Approved (agent) | Holding OCT- and OCT+ for 5 seconds enters update mode, and not sooner |
| R-013 | Each package shall contain the update loader whose SHA-256 is pinned in `factory/loader.sha256`. | D-011, HAZ-04 | A | Approved (agent) | Every build carries the pinned update loader |
| R-014 | When the package is installed on a real FM-1 with the felucca-bsp installer, the firmware shall start. | D-006 | H | Approved (agent) | It installs on a real FM-1 and goes back to stock |
| R-033 | When the official firmware is installed over the firmware, the FM-1 shall run the official firmware. | D-006 | H | Approved (agent) | It installs on a real FM-1 and goes back to stock |

## 5. Fitting the hardware

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-015 | While eight voices sound, the firmware shall spend less than 80 % of the CPU time in interrupts. | D-016, D-091, HAZ-01 | A | Approved (agent) | Every sound with eight voices fits the CPU |
| R-058 | While four tracks sound eight voices together, the firmware shall spend less than 80 % of the CPU time in interrupts. | D-029, D-016, D-091 | A | Approved (agent) | Every sound with eight voices fits the CPU |
| R-016 | The firmware image shall occupy at most 90 % of the app area. | D-001 | A | Approved (agent) | The firmware fits the flash and the RAM |
| R-035 | The firmware shall keep its audio output at or below −6 dBFS. | HAZ-01, D-008 | A | Approved (agent) | The output stays at or below −6 dBFS |
| R-036 | If two boots in a row end before the firmware has run for 2 s, then the firmware shall enter update mode. | HAZ-04, D-006 | A | Approved (agent) | A firmware that crashes at boot drops into update mode |
| R-037 | While the flash is busy, the firmware shall output silence. | HAZ-07, D-019 | I | Approved (agent) | *(audio_silence before every erase and write, fm1kit/src/device.c)* |
| R-038 | Each app shall flash the whole screen fewer than 3 times in a 1 s interval. | HAZ-03 | I | Approved (agent) | *(review of each app; WCAG 2.3.1 three flashes)* |
| R-034 | The firmware's RAM sections shall fit their memory regions. | D-001 | A | Approved (agent) | The firmware fits the flash and the RAM |
| R-019 | The firmware shall write flash only in the update staging area and in the store's region. | D-012, D-022, HAZ-04, HAZ-05 | I | Approved (agent) | *(FM1_FLASH_WRITE_REGIONS in fm1kit/src/device.c; the BSP's flash driver refuses the rest)* |
| R-018 | Each app shall reach the hardware only through `fm1kit.h`. | D-001 | I | Approved (agent) | *(factory/safety.py checks the files in apps/)* |

## 6. Music theory

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-017 | The music functions shall return Sketchpad's note names, key spellings and scales for the 24 keys and the 17 scales. | D-005, HAZ-08 | A | Approved (agent) | Note names and scales match Sketchpad in every key |
| R-043 | The music functions shall return Sketchpad's run order for up, down, up and down, and random with a seed. | D-024 | A | Approved (agent) | Run orders and tempo match Sketchpad |
| R-059 | The music functions shall return Sketchpad's triads, with their roots, qualities and Roman numerals, for the seven-note scales in the 12 keys. | D-030 | A | Approved (agent) | Chords, voicings and key analysis match Sketchpad |
| R-060 | The music functions shall return Sketchpad's root position and two inversions of each triad. | D-030 | A | Approved (agent) | Chords, voicings and key analysis match Sketchpad |
| R-061 | The music functions shall return Sketchpad's judgement of whether a progression's chords belong to a key, and of the key that holds more of them. | D-030 | A | Approved (agent) | Chords, voicings and key analysis match Sketchpad |
| R-062 | The music functions shall return Sketchpad's ranking of the keys that played notes could be in, with its verdict on whether they are ambiguous. | D-030, D-027 | A | Approved (agent) | Chords, voicings and key analysis match Sketchpad |
| R-083 | The music functions shall return Sketchpad's notes and chord symbol for each colour chord of a triad in five voicings. | D-033 | A | Approved (agent) | Colour chords, voicings and the next chord match Sketchpad |
| R-084 | The music functions shall return Sketchpad's likely next chord after each chord of a key. | D-033 | A | Approved (agent) | Colour chords, voicings and the next chord match Sketchpad |
| R-095 | The music functions shall return Sketchpad's name for the chord that played notes form. | D-035 | A | Approved (agent) | Names of played chords match Sketchpad |

## 7. Product (the learning app)

Requirements that change what a child sees, hears or learns, approved by the owner, story by story (#4).

**The chord pad** (concept v0.4, `concepts/v0.4/CHORD_PAD.md`, approved by the owner on 2026-11-07 as CP-R1 to CP-R6,
CP-R10 and CP-R11; how to use it: `MANUAL.md`).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-081 | When GLO is held, the firmware shall show help for the current screen. | D-034 | A | Approved (owner) | Holding GLO shows help for the screen |

**Manual chords** (concept v0.4, `concepts/v0.4/CHORD_PAD.md` §2, approved by the owner on 2026-11-07 as MC-R1 to MC-R10;
how to use it: `MANUAL.md`).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|

**The song page and the modes** (concept v0.5 §2.4–2.5, the owner's direction and answers of 2026-11-08 as CV-R4 to
CV-R11).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-096 | When GLO is tapped, the firmware shall show the song page with the key, the tempo, the length and the time signature on the four knobs. | D-038 | A | Approved (owner) | Tapping GLO opens the song page, where the key, tempo and length change and nothing played does |
| R-097 | When K1 turns on the song page, the firmware shall step through AUTO and the 24 keys. | D-038, D-032 | A | Approved (owner) | Tapping GLO opens the song page, where the key, tempo and length change and nothing played does |
| R-098 | When the key or the tempo changes on the song page, the song shall keep its recorded notes. | D-038, D-032 | A | Approved (owner) | Tapping GLO opens the song page, where the key, tempo and length change and nothing played does |
| R-099 | When HOME is held, the firmware shall show the mode picker with Producer, Guided and Game. | D-038 | A | Approved (owner) | Holding HOME opens the modes; a new mode asks first, shows its home and keeps the song |
| R-100 | When a mode is chosen, the firmware shall show that mode's home and colour the screen's top line in the mode's colour. | D-038 | A | Approved (owner) | Holding HOME opens the modes; a new mode asks first, shows its home and keeps the song |
| R-102 | When the mode changes, the firmware shall keep the song. | D-038 | A | Approved (owner) | Holding HOME opens the modes; a new mode asks first, shows its home and keeps the song |
| R-103 | When a different mode is chosen in the mode picker, the firmware shall ask for a confirmation before it switches. | D-038 | A | Approved (owner) | Holding HOME opens the modes; a new mode asks first, shows its home and keeps the song |
| R-240 | While the song page shows, when ALGORITHM, PRESETS, OCT- or OCT+ is used, the companion app shall close the song page before it acts. | D-071 | A | Approved (owner) | The song page closes when the track's sound or octave changes |
| R-241 | When HOME is tapped in Producer mode, the companion app shall close the song page, the REC layer and the open question. | D-071 | A | Approved (owner) | HOME tapped closes the song page, the REC layer and a question |
| R-242 | While a question waits for its answer, the companion app shall ignore the knobs and newly pressed keys. | D-071, HAZ-02 | A | Approved (owner) | A question takes the knobs and new keys until it is answered |
| R-243 | When SAVE is tapped on a track page, the companion app shall show HOLD, instead of undoing. | D-071, R-147 | A | Approved (owner) | SAVE tapped says how to undo and keeps the take |
| R-244 | When a key is let go while the mode picker shows, the companion app shall stop that key's note. | D-071, HAZ-02 | A | Approved (owner) | A key let go while the modes show stops its note |

**The bass track** (concept v0.5 BASS.md; its behaviour is in the approved Gherkin scenarios of "Feature: The bass
track", per the way of working; the rule that always holds is here).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-104 | The companion app shall keep each note of a bass idea or pattern within the bass's selected scale on the song's key. | D-040, D-039 | A | Approved (owner) | The bass keeps its ideas and patterns inside the scale |

**The melody track** (stories ST-26 to ST-29, ST-44, ST-49 and ST-50; D-050. Moved here from
`REQUIREMENTS-PLANNED.md` when they were built.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-105 | When a melody track opens, the companion app shall start on its PLAY page. | D-050, UC-11, ST-50 | A | Approved (owner) | Every template opens on PLAY |
| R-106 | The companion app shall offer the song key's pentatonic as the melody's first scale. | D-050, UC-11, ST-26 | A | Approved (owner) | The melody starts on the key's pentatonic |
| R-107 | While a melody track's HELP is Chord and a chord sounds, the companion app shall light the keys of that chord's notes at the full level. | D-050, UC-11, ST-26 | A | Approved (owner) | HELP Chord lights the chord now and colours the note played |
| R-108 | While a melody track's HELP is Chord and a chord sounds, the companion app shall light the keys of the scale's other notes at the dim level. | D-050, UC-11, ST-26 | A | Approved (owner) | HELP Chord lights the chord now and colours the note played |
| R-109 | While a melody track's HELP is Chord, the companion app shall show a played note in green when it is a note of the chord sounding. | D-050, UC-11, ST-26 | A | Approved (owner) | HELP Chord lights the chord now and colours the note played |
| R-110 | While a melody track's HELP is Chord, the companion app shall show a played note in amber when it is outside the scale. | D-050, UC-11, ST-26 | A | Approved (owner) | HELP Chord lights the chord now and colours the note played |
| R-111 | While notes are held on a melody track's PLAY page, the companion app shall show the names of the held notes without their octave. | D-050, UC-11, ST-26 | A | Approved (owner) | The melody page shows the notes held, and the chord they make |
| R-112 | While three or more held notes on a melody track form a chord, the companion app shall show the chord's name. | D-050, UC-11, ST-26 | A | Approved (owner) | The melody page shows the notes held, and the chord they make |
| R-113 | While a track's HELP is Free, the companion app shall show the SCALE and OUTSIDE settings greyed. | D-050, UC-00, ST-44 | A | Approved (owner) | With HELP Free, the scale's settings are greyed |
| R-114 | While a melody track's HELP is Free, the companion app shall light only the keys being played. | D-050, UC-11, ST-44 | A | Approved (owner) | With HELP Free, the scale's settings are greyed |
| R-115 | When PLUS is 3rd and a key is played, the companion app shall also play the note a third above it in the song's key. | D-050, UC-11, ST-29 | A | Approved (owner) | PLUS adds a voice in the key, or an echo |
| R-116 | When PLUS is 6th and a key is played, the companion app shall also play the note a sixth above it in the song's key. | D-050, UC-11, ST-29 | A | Approved (owner) | PLUS adds a voice in the key, or an echo |
| R-117 | When PLUS is echo and a key is played, the companion app shall repeat the note twice, softer, a dotted eighth apart. | D-050, UC-11, ST-29 | A | Approved (owner) | PLUS adds a voice in the key, or an echo |
| R-118 | When SCALE turns while chords are recorded, the companion app shall name, for each scale listed, the chord notes it lacks, or show that it fits. | D-050, UC-11, ST-49 | A | Approved (owner) | The scale list says which scales fit the chords and what the others lack |
| R-119 | While a melody track is on IDEAS, the companion app shall show the idea's notes falling onto the keys they belong to. | D-050, UC-12, ST-27 | A | Approved (owner) | A melody idea falls onto its keys, in versions |
| R-120 | When SELECT turns on a melody track's IDEAS page, the companion app shall play another version of the idea with the same rhythm. | D-050, UC-12, ST-27 | A | Approved (owner) | A melody idea falls onto its keys, in versions |
| R-121 | While a melody pad is held on AUTO, the companion app shall play the style's phrase over that pad's chord, its first bar and its answer in turn. | D-050, UC-11, ST-28 | A | Approved (owner) | Holding a pad plays a phrase in a style, call and answer |
| R-122 | The companion app shall keep each note of a melody phrase within the melody's selected scale. | D-050, UC-11, ST-28 | A | Approved (owner) | Holding a pad plays a phrase in a style, call and answer |

**Recording, the same on every track** (stories ST-40 to ST-42, and ST-31's feel; D-051. Moved here from
`REQUIREMENTS-PLANNED.md` when they were built.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-139 | When REC is held, the companion app shall open the track's REC layer with GRID, FEEL, CLICK and COUNT-IN on the knobs. | D-051, UC-13, ST-40 | A | Approved (owner) | Holding REC opens the track's REC layer |
| R-140 | While a track's GRID is set, the companion app shall play the track's notes at the nearest grid position. | D-051, UC-13, ST-40 | A | Approved (owner) | GRID and FEEL change the playback, never the recording |
| R-141 | The companion app shall keep a track's recorded notes as played when its GRID or FEEL changes. | D-051, UC-13, ST-40 | A | Approved (owner) | GRID and FEEL change the playback, never the recording |
| R-142 | While a track's FEEL is set and its GRID is on, the companion app shall move each note by that feel's swing and shift for the note's drum group or track. | D-051, UC-10, ST-31 | A | Approved (owner) | GRID and FEEL change the playback, never the recording |
| R-144 | When OCT+ is pressed in the REC layer, the companion app shall ask before it clears the track. | D-051, UC-13, ST-40 | A | Approved (owner) | OCT+ in the REC layer clears the track after asking |
| R-145 | When OCT+ is pressed again while the companion app asks to clear the track, the companion app shall remove the track's notes. | D-051, UC-13, ST-40 | A | Approved (owner) | OCT+ in the REC layer clears the track after asking |
| R-146 | While EDIT is held with a key held as the song plays, the companion app shall remove that key's notes of the track as the playhead passes them. | D-051, UC-13, ST-41 | A | Approved (owner) | EDIT and a key erase that key's notes as they play |
| R-147 | When SAVE is held, the companion app shall undo the last recording, erase or clear. | D-051, UC-13, ST-41 | A | Approved (owner) | SAVE held undoes the last change |
| R-148 | When REC is pressed while the song is stopped and COUNT-IN is on, the companion app shall play the count-in's bars of clicks before it records. | D-051, UC-13, ST-40 | A | Approved (owner) | Holding REC opens the track's REC layer |
| R-149 | While a track is on IDEAS with SPEED below 100 %, the companion app shall play the song at that share of its tempo. | D-051, UC-12, ST-42 | A | Approved (owner) | SPEED slows the whole song while practising |
| R-150 | The companion app shall record notes played at a practice speed at their place in the song's tempo. | D-051, UC-12, ST-42 | A | Approved (owner) | SPEED slows the whole song while practising |

**Explanations** (story ST-43; D-052. Moved here from `REQUIREMENTS-PLANNED.md` when built.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-151 | When a knob turns while explanations are on, the companion app shall show what the new value does. | D-052, UC-00, ST-43 | A | Approved (owner) | A knob explains its value, unless explanations are off |
| R-152 | While explanations are off, the companion app shall show no explanation when a knob turns. | D-052, UC-00, ST-43 | A | Approved (owner) | A knob explains its value, unless explanations are off |
| R-245 | When the companion app starts without kept settings, the companion app shall have explanations off. | D-072 | A | Approved (owner) | A knob explains its value, unless explanations are off |

**The drums: the kits and the pads** (stories ST-30, ST-38 and ST-39; D-053. Moved here from
`REQUIREMENTS-PLANNED.md` when built.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-124 | The companion app shall give each of the 27 keys its own sound in each drum kit. | D-053, UC-10, ST-30 | A | Approved (owner) | Every key is its own drum sound, named on the screen |
| R-125 | When a key is played on a drums track, the companion app shall show the name of its sound. | D-053, UC-10, ST-30 | A | Approved (owner) | Every key is its own drum sound, named on the screen |
| R-126 | When PRESETS turns on a drums track, the companion app shall change the track's kit. | D-053, UC-10, ST-30 | A | Approved (owner) | PRESETS changes the kit; a held key and PRESETS change that key's sound |
| R-127 | When PRESETS turns while a key is held on a drums track, the companion app shall change that key's sound to the next of the four kits' sounds. | D-053, UC-14, ST-39 | A | Approved (owner) | PRESETS changes the kit; a held key and PRESETS change that key's sound |
| R-128 | While a drums track has a key whose sound was changed, the companion app shall name the kit My kit on the top line. | D-053, UC-14, ST-39 | A | Approved (owner) | PRESETS changes the kit; a held key and PRESETS change that key's sound |
| R-129 | When K1, K2 or K3 turns on a drums track's PLAY page, the companion app shall change the repeat, the tuning or the decay of the sound played last. | D-053, UC-10, ST-38 | A | Approved (owner) | Each pad keeps its own repeat, tuning and decay |
| R-130 | While a key with a repeat is held on a drums track, the companion app shall play its sound again at the repeat's interval at the song's tempo. | D-053, UC-10, ST-38 | A | Approved (owner) | Each pad keeps its own repeat, tuning and decay |
| R-131 | When a pitched drum sound is tuned, the companion app shall show the note it sounds. | D-053, UC-14, ST-38 | A | Approved (owner) | Each pad keeps its own repeat, tuning and decay |

**The drums: AUTO, IDEAS and the feel drawn** (stories ST-31, ST-32 and ST-33; D-054.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-132 | While a drums AUTO pad is held, the companion app shall play its beat of the selected style: main, busy, fill, break or drop. | D-054, UC-10, ST-32 | A | Approved (owner) | Holding a pad plays a style's beat, with fills and ghost notes |
| R-133 | While FILLS is 4 or 8 bars and the main pad is held, the companion app shall play the style's fill in the last bar of each 4 or 8 bars. | D-054, UC-10, ST-32 | A | Approved (owner) | Holding a pad plays a style's beat, with fills and ghost notes |
| R-134 | While GHOSTS is off, the companion app shall leave the ghost notes out of a drum style's beats. | D-054, UC-10, ST-32 | A | Approved (owner) | Holding a pad plays a style's beat, with fills and ghost notes |
| R-135 | While a drums track is on IDEAS, the companion app shall show the beat's hits falling onto the keys of their sounds. | D-054, UC-12, ST-33 | A | Approved (owner) | A drum beat falls onto its keys to learn |
| R-143 | While a drums track plays with a feel, the companion app shall draw each hit where the feel places it, with a line from its grid position. | D-054, UC-10, ST-31 | A | Approved (owner) | The screen draws where the feel puts each hit |
| R-136 | While a drums track's FX is Punch, MPC, Lofi or Tape, the companion app shall process that track's sound through that effect. | D-055, UC-10, ST-34 | A | Approved (owner) | The drums go through Punch, MPC, Lofi or Tape |

**The user kit** (story ST-35; D-056.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-137 | When the firmware is built with sound files in `kits/user/`, the build shall put each file on the key its number names and show the rest of its name. | D-056, UC-14, ST-35 | A | Approved (owner) | The user kit comes from numbered sound files with a licence |
| R-138 | If `kits/user/` holds sound files without a `LICENSE` file, then the build shall stop. | D-056, UC-14, ST-35 | A | Approved (owner) | The user kit comes from numbered sound files with a licence |

**Styles on every template** (story ST-48; D-057.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-156 | The companion app shall offer the styles reggae, afrobeats, shatta, reggaeton and drum and bass on the drums, the chord pad, the bass and the melody. | D-057, UC-17, ST-48 | A | Approved (owner) | A style is on every template |

**The game: Unicorn Jump** (story ST-51; D-059. Approved by the owner, 2026-11-14: "For requirements and screens works good. Approved.")

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-157 | When the Game mode opens, the companion app shall show the unicorn waiting in a night meadow with the 27 keys breathing. | D-059, UC-18, ST-51 | A | Approved (owner) | The game waits for a key, then the unicorn runs to the first fence and waits there |
| R-158 | When a key is played while the game waits, the companion app shall start the unicorn running towards fences that each carry a note of the selected song. | D-059, UC-18, ST-51 | A | Approved (owner) | The game waits for a key, then the unicorn runs to the first fence and waits there |
| R-162 | While the next fence has not been played, the companion app shall stop the unicorn in front of it. | D-059, UC-18, ST-51 | A | Approved (owner) | The game waits for a key, then the unicorn runs to the first fence and waits there |
| R-169 | While K1 is Easy, the companion app shall take a key played as the next fence's note. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode any key jumps the fence and plays its note |
| R-160 | When the next fence's note is played while the fence is on the screen, the companion app shall make the unicorn jump the fence and play the fence's note. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode any key jumps the fence and plays its note |
| R-170 | When the next fence's note is played before the unicorn reaches the fence, the companion app shall make the unicorn dash to the fence. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode any key jumps the fence and plays its note |
| R-171 | When the unicorn jumps a fence, the companion app shall keep the unicorn's speed until it lands beyond the fence. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode any key jumps the fence and plays its note |
| R-159 | While K1 is Key, the companion app shall light the key of the next fence's note. | D-059, UC-18, ST-51 | A | Approved (owner) | In Key mode the next fence's key lights; a wrong key bumps and it waits; two wrong, a poop |
| R-161 | When a wrong key is played in Key mode, the companion app shall make the unicorn bump the fence and wait on. | D-059, UC-18, ST-51 | A | Approved (owner) | In Key mode the next fence's key lights; a wrong key bumps and it waits; two wrong, a poop |
| R-163 | When two wrong keys are played in a row, the companion app shall make the unicorn poop with a low buzz and a plop and count the poop on the top line. | D-059, UC-18, ST-51 | A | Approved (owner) | In Key mode the next fence's key lights; a wrong key bumps and it waits; two wrong, a poop |
| R-165 | When the unicorn clears eight fences in a row in Key mode, the companion app shall show WINGS! and keep the wings until the song ends. | D-059, UC-18, ST-51 | A | Approved (owner) | Eight good jumps in Key mode earn the wings, and the unicorn flies when the song ends |
| R-166 | While K1 is Key, the companion app shall show each fence's note as its letter in the note's colour. | D-059, UC-18, ST-51 | A | Approved (owner) | Eight good jumps in Key mode earn the wings, and the unicorn flies when the song ends |
| R-164 | When a poop leaves the screen behind the unicorn, the companion app shall bring it back ahead of the unicorn in the place of a fence. | D-059, UC-18, ST-51 | A | Approved (owner) | A poop comes round again and any key hops it |
| R-168 | When a key is played while a poop is the next thing ahead, the companion app shall make the unicorn hop it. | D-059, UC-18, ST-51 | A | Approved (owner) | A poop comes round again and any key hops it |
| R-167 | The companion app shall keep the game running with no end and no loss. | D-059, UC-18, ST-51 | A | Approved (owner) | The song never ends |
| R-172 | While K4 is on in the game, the companion app shall play a pad chord and a bass note that hold the last fence's note, with reverb. | D-059, D-060, UC-18, ST-51 | A | Approved (owner) | With the music on, a pad and a bass play under the bells, and K4 turns them off |
| R-173 | While K4 is off in the game, the companion app shall play the bells and the game's sounds without the pad, the bass and the reverb. | D-059, D-060, UC-18, ST-51 | A | Approved (owner) | With the music on, a pad and a bass play under the bells, and K4 turns them off |
| R-174 | While K1 is Easy, the companion app shall draw the fences without notes. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode the fences are plain and the unicorn flies when the song ends |
| R-175 | When the last fence of the song is jumped in Easy mode or with wings, the companion app shall fly the unicorn away to the moon, once round it and back, drawn smaller the further away it is, with a rainbow trail. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode the fences are plain and the unicorn flies when the song ends |
| R-190 | While the unicorn flies, the companion app shall hold the song's next round until the unicorn lands. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode the fences are plain and the unicorn flies when the song ends |
| R-191 | While the unicorn runs, the companion app shall draw no rainbow behind it. | D-059, UC-18, ST-51 | A | Approved (owner) | In Easy mode the fences are plain and the unicorn flies when the song ends |
| R-192 | When SELECT is turned in the Game mode, the companion app shall show the next game of Unicorn Jump, Hiru Damatxo, Aldapeko Sagarra and the Night Train. | D-062, D-064, UC-18, ST-53, ST-55 | A | Approved (owner) | SELECT turns from one game to the next |
| R-193 | When a key is played in Hiru Damatxo, the companion app shall play the song's next note and make its dancer jump. | D-062, UC-18, ST-53 | A | Approved (owner) | In Hiru Damatxo a key dances the next note |
| R-194 | When a wrong key is played in Key mode in Hiru Damatxo, the companion app shall show the note's dancer sitting down while the other two giggle. | D-062, UC-18, ST-53 | A | Approved (owner) | In Hiru Damatxo a wrong key sits a dancer down, and two bring the seagull |
| R-195 | When two wrong keys are played in a row in Hiru Damatxo, the companion app shall show a seagull pooping on the dancer's head. | D-062, UC-18, ST-53 | A | Approved (owner) | In Hiru Damatxo a wrong key sits a dancer down, and two bring the seagull |
| R-196 | When PLAY is pressed in Hiru Damatxo, the companion app shall play the song and dance it. | D-062, UC-18, ST-53 | A | Approved (owner) | In Hiru Damatxo PLAY plays the song, danced, and its end brings fireworks |
| R-197 | When the song's last note is played in Hiru Damatxo, the companion app shall show the three dancers turning with fireworks and ZORIONAK!. | D-062, UC-18, ST-53 | A | Approved (owner) | In Hiru Damatxo PLAY plays the song, danced, and its end brings fireworks |
| R-198 | When a key is played in Aldapeko Sagarra, the companion app shall play the song's next note an octave higher with the robin singing it. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra the robin sings each note an octave up, in the voice K2 picks |
| R-203 | When K2 is turned in Aldapeko Sagarra, the companion app shall change the robin's voice among a flute, a music box and a bird whistle. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra the robin sings each note an octave up, in the voice K2 picks |
| R-199 | When a wrong key is played in Key mode in Aldapeko Sagarra, the companion app shall drop an apple on a sheep's head. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra a wrong key drops an apple on a sheep, and two make it poop |
| R-200 | When two wrong keys are played in a row in Aldapeko Sagarra, the companion app shall show that sheep pooping with the robin hiding its eyes. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra a wrong key drops an apple on a sheep, and two make it poop |
| R-201 | When PLAY is pressed in Aldapeko Sagarra, the companion app shall play the song with the sheep dancing its last part. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra PLAY plays the song, the sheep dance its end, and TXIRULIRULI! |
| R-202 | When the song's last note is played in Aldapeko Sagarra, the companion app shall show TXIRULIRULI! with the baserri's windows lit. | D-063, UC-18, ST-54 | A | Approved (owner) | In Aldapeko Sagarra PLAY plays the song, the sheep dance its end, and TXIRULIRULI! |
| R-176 | When the unicorn has waited 8 s with no key, the companion app shall play a neigh. | D-059, D-060, UC-18, ST-51 | A | Approved (owner) | A unicorn left waiting neighs, then falls asleep until a key |
| R-177 | When the unicorn has waited 20 s with no key, the companion app shall show the unicorn asleep until a key is played. | D-059, UC-18, ST-51 | A | Approved (owner) | A unicorn left waiting neighs, then falls asleep until a key |
| R-178 | The companion app shall offer the Unicorn Song, written for the game, as a song of the game. | D-059, UC-18, ST-51 | A | Approved (owner) | The unicorn has its own song |
| R-179 | The companion app shall offer four taught songs, MY SONG 1 to MY SONG 4, after the built-in songs of the game. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-180 | When REC is pressed on a MY SONG, the companion app shall show the unicorn listening without fences until SAVE or REC is pressed. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-181 | While the unicorn listens, the companion app shall add each note played on the keys or received over MIDI to the song being taught. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-182 | While the unicorn listens, the companion app shall move a note outside F3 to G5 into F3 to G5 by octaves. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-183 | When a note is added while the unicorn listens, the companion app shall show the unicorn dancing a step and the note floating up to it. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-184 | When K1 is turned back while the unicorn listens, the companion app shall take back the last note taught. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-185 | When SAVE or REC is pressed while the unicorn listens, the companion app shall keep the song taught, up to 120 notes, over power-off. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-186 | While a MY SONG has no notes, the companion app shall show TEACH ME and run no fences. | D-061, UC-18, ST-52 | A | Approved (owner) | A parent teaches the unicorn a song, and it keeps it |
| R-187 | When REC is pressed on a built-in song, the companion app shall leave the song unchanged. | D-061, UC-18, ST-52 | A | Approved (owner) | The built-in songs cannot be taught over, and leaving while teaching keeps nothing |
| R-188 | When the game is left while the unicorn listens, the companion app shall keep the MY SONG as it was. | D-061, UC-18, ST-52 | A | Approved (owner) | The built-in songs cannot be taught over, and leaving while teaching keeps nothing |
| R-189 | While a MY SONG plays, the companion app shall play the pad and the bass in the song's key worked out from its notes. | D-061, UC-18, ST-52 | H | Approved (owner) | A taught song's pad and bass follow its key |

**The game: the Night Train** (story ST-55; D-064. Approved by the owner, 2026-11-15: "Approve all the train and implement in
firmware"; R-192 amended to reach it, Decision #90.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-204 | When the Night Train opens, the companion app shall show Old Number 7 waiting on the line in a desert night under a huge moon, its lamp lit. | D-064, UC-18, ST-55 | A | Approved (owner) | The train waits under the moon, and a key is a chuff |
| R-205 | When a key is played in the Night Train, the companion app shall play the song's next note and move the engine on with a chuff and a puff of smoke in the note's colour. | D-064, UC-18, ST-55 | A | Approved (owner) | The train waits under the moon, and a key is a chuff |
| R-206 | When K3 is turned in the Night Train, the companion app shall change the song between I've Been Working on the Railroad and Funiculì, funiculà. | D-064, UC-18, ST-55 | A | Approved (owner) | K3 changes the train's song to Funiculì, funiculà |
| R-207 | When a wrong key is played in Key mode in the Night Train, the companion app shall make the engine cough black smoke. | D-064, UC-18, ST-55 | A | Approved (owner) | In the train game a wrong key is a cough, and two in a row are a sneeze |
| R-208 | When two wrong keys are played in a row in the Night Train, the companion app shall make the engine sneeze with its front rearing up, the puppy barking and the driver cross. | D-064, UC-18, ST-55 | A | Approved (owner) | In the train game a wrong key is a cough, and two in a row are a sneeze |
| R-209 | When eight right notes are played in a row in Key mode in the Night Train, the companion app shall turn the engine into the golden super train for six seconds. | D-064, UC-18, ST-55 | A | Approved (owner) | Eight right notes in a row in Key mode make the golden super train |
| R-210 | When PLAY is pressed in the Night Train, the companion app shall play the song with the train running it. | D-064, UC-18, ST-55 | A | Approved (owner) | The song's end brings the train to Star Station, where it wins a star it keeps |
| R-211 | When the song's last note is played in the Night Train, the companion app shall run the golden super train and then stop the train at Star Station. | D-064, UC-18, ST-55 | A | Approved (owner) | The song's end brings the train to Star Station, where it wins a star it keeps |
| R-212 | When the train stops at Star Station, the companion app shall add a star to the train's tender. | D-064, UC-18, ST-55 | A | Approved (owner) | The song's end brings the train to Star Station, where it wins a star it keeps |
| R-213 | The companion app shall keep the train's stars over power-off. | D-064, UC-18, ST-55 | A | Approved (owner) | The song's end brings the train to Star Station, where it wins a star it keeps |
| R-214 | When a line of the song ends in the Night Train, the companion app shall blow the whistle and ring the engine's bell. | D-064, UC-18, ST-55 | K | Approved (owner) | The night line's life and sounds, in a play test |
| R-215 | While the train runs in the Night Train, the companion app shall show the line's life: telegraph poles and an owl, a ranch, a camp, a longhorn, prairie dogs, a jackrabbit and now and then a canyon on a trestle. | D-064, UC-18, ST-55 | K | Approved (owner) | The night line's life and sounds, in a play test |
| R-216 | While the last part of the song plays in the Night Train, the companion app shall open a flower on each cactus the lamp lights. | D-064, UC-18, ST-55 | K | Approved (owner) | The night line's life and sounds, in a play test |
| R-219 | While the companion app is in Guided mode, the guided mode shall show the lesson's number and title, the step's number, the step's words and what to do next. | D-068, UC-19, ST-57 | A | Approved (owner) | Guided opens on lesson 1, step 1, and says what to do |
| R-220 | When the action a step asks for is done, the guided mode shall show a green tick and sound a two-note chime. | D-068, UC-19, ST-57 | A | Approved (owner) | A step done shows a green tick and sounds a chime |
| R-221 | When SELECT is turned one click clockwise, the guided mode shall go to the next step. | D-068, UC-19, ST-57 | A | Approved (owner) | SELECT turns the guided mode's steps |
| R-222 | When SELECT is turned one click anticlockwise, the guided mode shall go back one step. | D-068, UC-19, ST-57 | A | Approved (owner) | SELECT turns the guided mode's steps |
| R-223 | When HOME is tapped in Guided mode, the guided mode shall show the list of the lessons with each lesson's number of steps. | D-068, UC-19, ST-57 | A | Approved (owner) | HOME tapped shows the lessons, and HOME opens the chosen one |
| R-224 | While the list of the lessons shows, when HOME is tapped, the guided mode shall open the chosen lesson at its first step. | D-068, UC-19, ST-57 | A | Approved (owner) | HOME tapped shows the lessons, and HOME opens the chosen one |
| R-225 | While a lesson's song plays, the guided mode shall set the levels of the drums, the bass, the chords and the melody with K1 to K4, from 0 to 10. | D-068, UC-19, ST-57 | A | Approved (owner) | K1 to K4 are the guided mode's mixer |
| R-226 | While the companion app is in Guided mode, the guided mode shall show a 16-step sequencer of the kick, the snare, the hats, the bass, the chords and the melody, with the step's rows lit and the others dimmed. | D-068, UC-19, ST-57 | A | Approved (owner) | Guided opens on lesson 1, step 1, and says what to do |
| R-227 | While a step names keys to play, the guided mode shall light those keys. | D-068, UC-19, ST-57 | A | Approved (owner) | A key outside the key is named and said not to be in the key |
| R-228 | When C4, D4 or E4 is pressed on the step that teaches the drums, the guided mode shall play the kick, the snare or the hat on its own. | D-068, UC-19, ST-57 | A | Approved (owner) | The three drum keys play the kick, the snare and the hat on their own |
| R-229 | If REC is pressed in lesson 1, then the guided mode shall show that REC comes in lesson 2. | D-068, UC-19, ST-57 | A | Approved (owner) | REC in lesson 1 says that REC comes in lesson 2 |
| R-230 | If a key outside the song's key is pressed on a step that lights the key's notes, then the guided mode shall name the note and show that it is not in the key. | D-068, UC-19, ST-57 | A | Approved (owner) | A key outside the key is named and said not to be in the key |
| R-231 | The guided mode shall give lesson 1 the 14 steps of ST-57. | D-068, UC-19, ST-57 | K | Approved (owner) | Lesson 1, played through by a grown-up who never made a beat |
| R-232 | When Guided mode opens after a power-off, the guided mode shall open the lesson and step it was on. | D-068, UC-19, ST-57 | A | Approved (owner) | The guided mode opens on the lesson and step it was on, over power-off |
| R-233 | While the companion app is in Guided mode, the guided mode shall show what to do next in the 24-pixel font on one line, or in the 16-pixel font on at most two lines. | D-068, UC-19, ST-57 | A | Approved (owner) | Guided opens on lesson 1, step 1, and says what to do |

**The games: PLAY lights the keys** (story ST-56; D-065. Draft: the owner approves before it merges, DoD-4.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-217 | When PLAY is pressed in Unicorn Jump, the companion app shall run the unicorn through the song by itself, jumping each fence as its note sounds. | D-065, UC-18, ST-56 | A | Approved (owner) | In Unicorn Jump PLAY runs the song by itself, each fence's key lit as its note sounds |
| R-218 | While a game plays its song on PLAY, the companion app shall light the key of each note as the note sounds. | D-065, UC-18, ST-56 | A | Approved (owner) | While a game plays its song on PLAY, each note's key lights as the note sounds |

---

**One pattern for the tracks (ST-84)** (D-074; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-271 | While the BASS track's IDEAS page shows, when K3 turns, the companion app shall set the song's speed from half speed to full speed. | D-074, ST-84 | A | Approved (owner) | The bass and the drums follow the tracks' pattern |
| R-272 | While the BASS track's PLAY page shows, the companion app shall add the octave or the fifth to each note that K4 PLUS names. | D-074, ST-84 | A | Approved (owner) | The bass and the drums follow the tracks' pattern |
| R-273 | While the DRUMS track's AUTO page shows, the companion app shall set the feel with K3 and the ghost notes with K4. | D-074, ST-84 | A | Approved (owner) | The bass and the drums follow the tracks' pattern |

**The chords' HELP steps (ST-79)** (D-075; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-246 | While the CHORDS track's PLAY page shows, when K1 turns, the companion app shall step HELP through Free, Key, Shapes and Pads. | D-075, ST-79 | A | Approved (owner) | The chords' HELP steps from Free to Pads |
| R-247 | While HELP is Key, the companion app shall light the key's notes dim and its tonic at half light. | D-075, ST-79 | A | Approved (owner) | One note held on Key lights a chord of the key around it |
| R-248 | While HELP is Key, when one key is held, the companion app shall light the other notes of a chord of the key that holds the held note, with the colour set on K3. | D-075, ST-79 | A | Approved (owner) | One note held on Key lights a chord of the key around it |
| R-249 | While HELP is Key and a chord is suggested, when SELECT turns, the companion app shall suggest the next chord of the key that holds the notes held. | D-075, ST-79 | A | Approved (owner) | One note held on Key lights a chord of the key around it |
| R-250 | While HELP is Key, the companion app shall show the key's seven chords and light the chord held. | D-075, ST-79 | A | Approved (owner) | One note held on Key lights a chord of the key around it |
| R-251 | While HELP is Free or Key, when K2 TRANSPOSE is set to whites, the companion app shall shift the keys so that the white keys play the notes of the song's key. | D-075, ST-79 | A | Approved (owner) | TRANSPOSE whites plays the key on the white keys |
| R-252 | While HELP is Shapes, the companion app shall light the keys of the progression's chord at full light and the keys of its next chord at half light. | D-075, ST-79 | A | Approved (owner) | Shapes lights the chord to play and the next one |
| R-253 | While HELP is Shapes, when SELECT turns, the companion app shall move to the next chord of the progression in the turn's direction. | D-075, ST-79 | A | Approved (owner) | Shapes lights the chord to play and the next one |

**The chord pads and the progressions (ST-80)** (D-076; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-254 | While HELP is Pads, the companion app shall play on the white keys C to B the chords of the key's degrees I to vii, in each octave of the keyboard. | D-076, ST-80 | A | Approved (owner) | Pads: C plays I and D plays ii in every key |
| R-255 | While HELP is Pads, when a black key is held with a pad, the companion app shall add the black key's colour to the chord. | D-076, ST-80 | A | Approved (owner) | Pads: C plays I and D plays ii in every key |
| R-256 | While HELP is Pads, the companion app shall draw the FM-1's 27 keys with the chord or the colour each key plays. | D-076, ST-80 | A | Approved (owner) | Pads: C plays I and D plays ii in every key |
| R-257 | When PROGRESSION turns, the companion app shall show the numbered list of progressions with each one's style and numerals. | D-076, ST-80 | A | Approved (owner) | PROGRESSION opens a numbered menu |
| R-258 | While HELP is Pads, when F5 is pressed, the companion app shall play the progression's next chord in the octave of the last pad. | D-076, ST-80 | A | Approved (owner) | Pads: C plays I and D plays ii in every key |
| R-259 | The companion app shall list None, Mine, Follow bass and Follow melody before the styles' progressions. | D-076, ST-80 | A | Approved (owner) | PROGRESSION opens a numbered menu |

**How the pads play a chord (ST-81)** (D-077; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-260 | While HELP is Pads, the companion app shall play each chord with the note timing and velocities of the FEEL chosen on K4. | D-077, ST-81 | A | Approved (owner) | FEEL pianist plays the bass first and the inner notes softer |

**Chord ideas (ST-82)** (D-079; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-261 | The companion app shall make each chord idea 2 bars or 4 bars long. | D-079, ST-82 | A | Approved (owner) | A chord idea lights its next chord a bar ahead and fills 4 bars |
| R-262 | While an idea plays, the companion app shall light the keys of the chord now at full light and the keys of the next chord at half light from the bar before it. | D-079, ST-82 | A | Approved (owner) | A chord idea lights its next chord a bar ahead and fills 4 bars |
| R-263 | While IDEAS shows, when SELECT turns, the companion app shall move to the idea's next chord in the turn's direction and light its keys. | D-079, ST-82 | A | Approved (owner) | A chord idea lights its next chord a bar ahead and fills 4 bars |
| R-264 | When the IDEAS page closes, the companion app shall set SPEED back to full speed. | D-079, ST-82 | A | Approved (owner) | A chord idea lights its next chord a bar ahead and fills 4 bars |

**Chord auto: song parts (ST-83)** (D-080; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-265 | While the CHORDS track's AUTO page shows, the companion app shall play the style's intro, verse, pre-chorus, chorus, bridge, break and outro calm on the white keys F3 to E4 and with energy on the white keys F4 to E5. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |
| R-266 | While a part plays on AUTO, when another part's key is pressed, the companion app shall start that part on the next bar. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |
| R-267 | While nothing plays, when a part's key is pressed on AUTO, the companion app shall play that part on its own. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |
| R-268 | While a black key is held on AUTO, the companion app shall apply the black key's move to the part playing. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |
| R-269 | The companion app shall lay out each AUTO part in 2 bars or 4 bars. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |
| R-270 | While FOLLOW is set to bass or melody, the companion app shall play the AUTO part on the chord of the key that fits the recorded bass or melody at that moment. | D-080, ST-83 | A | Approved (owner) | AUTO plays the style's verse, then the chorus from the next bar |

**The sequencer: SONG (ST-85)** (D-081; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-274 | The companion app shall hold 16 sequences, each a section of the four tracks. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-275 | While SONG shows and the song is stopped, when a lit sequence key is pressed, the companion app shall add that sequence to the song after the cursor's block. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-276 | While the song plays on SONG, when a lit sequence key is pressed, the companion app shall play that sequence from the next bar. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-277 | While a lit sequence key is held, when a dark sequence key is pressed, the companion app shall copy the held sequence to the dark key. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-278 | When EDIT and a lit sequence key are pressed, the companion app shall ask whether to delete that sequence. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-279 | While the delete question shows, when EDIT and the same sequence key are pressed within 3 s, the companion app shall delete the sequence and its blocks. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-280 | While SONG shows, the companion app shall light the keys of the sequences that hold notes and leave the keys of the empty ones dark. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-281 | While SONG shows, the companion app shall draw a repeated block as the sequence followed by a dashed copy for each repeat. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |
| R-282 | When a new song is made, the companion app shall start it with sequence 1, empty, as its only block. | D-081, ST-85 | A | Approved (owner) | SONG adds a sequence after the cursor, and asks before deleting one |

**The sequencer: SEQUENCE (ST-86)** (D-082; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-283 | While REC records on SEQUENCE, the companion app shall keep each note's start, length and velocity as played. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |
| R-284 | When K1 QUANTIZE turns, the companion app shall play the track's notes moved toward the chosen grid by the K3 STRENGTH amount, without changing the recorded notes. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |
| R-285 | When SAVE is held after a take, the companion app shall remove that take. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |
| R-286 | When EDIT and REC are pressed, the companion app shall clear the track's notes and motion in the sequence. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |
| R-287 | When SAVE is held after a track was cleared, the companion app shall restore the track. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |
| R-288 | If a note is played less than half a 16th before the loop ends, then the companion app shall record the note on the loop's first step. | D-082, ST-86 | A | Approved (owner) | A take is undone with SAVE held, and quantize keeps what was played |

**The sequencer: STEPS (ST-87)** (D-083; approved by the owner on 2026-11-19.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-289 | While STEPS shows, the companion app shall show one bar with its four beats numbered and the cursor's beat divided by K1 GRID. | D-083, ST-87 | A | Approved (owner) | STEPS writes only with REC on and removes with EDIT |
| R-290 | While STEPS shows and REC is off, the companion app shall play the keys without writing notes. | D-083, ST-87 | A | Approved (owner) | STEPS writes only with REC on and removes with EDIT |
| R-291 | While STEPS shows and REC is on, when keys are pressed, the companion app shall write their notes at the cursor. | D-083, ST-87 | A | Approved (owner) | STEPS writes only with REC on and removes with EDIT |
| R-292 | While STEPS shows, when EDIT and a key are pressed, the companion app shall remove that key's note at the cursor. | D-083, ST-87 | A | Approved (owner) | STEPS writes only with REC on and removes with EDIT |
| R-293 | While STEPS shows, the companion app shall light the keys of the notes at the cursor. | D-083, ST-87 | A | Approved (owner) | STEPS writes only with REC on and removes with EDIT |

**fmini-1's Minimoog engine (ST-70)** (D-086; drafts for the owner: the owner asked for the sounds on 2026-11-21, "So you can
implement all sounds".)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-307 | When an FMini sound plays a note, the firmware shall sound the note within 15 cents of its pitch. | D-086, ST-70 | A | Draft | An FMini sound plays its note in tune and stops when released |
| R-308 | While a track plays an FMini sound, the firmware shall sound at most four of the track's notes at once. | D-086, ST-70 | A | Draft | FMini plays the owner's 66 picks, four notes at most a track |

**ST-92, Bass Something 2 (Draft, work order of 2026-11-23).**

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-316 | When a Bass Something 2 sound is chosen, the companion app shall select its matching factory patch. | D-095, ST-92, UC-07 | A | Draft | The Sound Browser selects each picked Bass Something 2 patch |
| R-317 | While a track plays a Bass Something 2 sound, the firmware shall sound at most one of the track's notes at once. | D-095, ST-92, UC-07 | A | Draft | Bass Something 2 plays one voice per track |
| R-318 | When Square Solo plays C4, the firmware shall sound the note within 15 cents of its pitch. | D-095, ST-92, UC-07 | A | Draft | Square Solo plays the selected note and releases it |
| R-319 | When a note is played with Sync Lead selected, the firmware shall sound OberVave Sync Lead. | D-096, ST-72, UC-07 | A | Draft | OberVave Sync Lead sounds when played |
| R-320 | When Shadow Arp is selected, the firmware shall enable the track arpeggiator with the factory settings. | D-097, ST-72, UC-07 | A | Draft | Selecting Shadow Arp enables the track arpeggiator |

**CoralFlower VA (ST-73, Draft)**

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-321 | When a note is played with Punch Bass selected, the firmware shall sound CoralFlower Punch Bass. | D-098, ST-73, UC-07 | A | Draft | CoralFlower Punch Bass sounds when played |
| R-322 | When a CoralFlower VA sound is chosen, the companion app shall select its matching factory patch. | D-098, ST-73, UC-07 | A | Draft | The Sound Browser selects each picked CoralFlower VA patch |

**FMini factory arpeggios (ST-77, Draft)**

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-323 | When a chosen FMini patch has its factory arpeggiator enabled, the companion app shall play that pattern using the track's arpeggiator. | D-099, ST-77, UC-07 | A | Draft | FMini Pluck Arp plays through the track arpeggiator |

**Softer game sounds (ST-93, Draft)**

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-324 | When Unicorn Jump plays the first three notes of Twinkle Twinkle in Listen mode, the companion app shall keep the mix peak at or below −10 dBFS. | D-100, UC-18, ST-93 | A | Draft | In Unicorn Jump PLAY runs the song by itself, each fence's key lit as its note sounds |
| R-325 | When the Night Train whistles and plays the first note of Funiculì with music off, the companion app shall keep the mix peak at or below −20 dBFS. | D-100, UC-18, ST-93 | A | Draft | K3 changes the train's song to Funiculì, funiculà |

**FM-106's Juno-106 engine (ST-71b)** (D-090; drafts for the owner, from "Let's go with the other engines!", 2026-11-22.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-312 | When a Juno-106 sound plays a note, the firmware shall sound the note within 15 cents of its pitch. | D-090, ST-71b | A | Draft | A Juno-106 sound plays its note in tune and stops when released |
| R-313 | While a track plays a Juno-106 sound, the firmware shall sound at most five of the track's notes at once. | D-090, ST-71b | A | Draft | The Juno-106 plays the owner's 28 picks, five notes at most a track |

**The effects bus (ST-69)** (D-093; drafts for the owner, from "implement ... the fx in our fx so they sound the same", 2026-11-22.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-314 | When a track sends to the delay, the firmware shall repeat the track's sound after the delay's time. | D-093, ST-69 | A | Draft | The effects bus delays, drives and choruses a track's sound by its sends, and ends |
| R-315 | When an FMini sound is chosen for a track, the firmware shall set the track's effect sends from the sound's patch. | D-093, ST-69 | A | Draft | An FMini sound plays its note in tune and stops when released |
| R-328 | When DELAY TONE is lowered, the firmware shall soften a short impulse in the delayed signal. | D-102, ST-69, UC-07 | A | Draft | Delay tone softens the echo |
| R-329 | When FX is tapped on a synth track's playing page, the companion app shall show the PERFORM page. | D-072, ST-69, UC-07 | A | Draft | FX opens a live performance page and delay |
| R-330 | While DELAY shows, when K3 is turned, the companion app shall change the selected track's delay send. | D-072, ST-69, UC-07 | A | Draft | FX opens a live performance page and delay |
| R-331 | When room pre-delay is set to 100 ms, the firmware shall delay the wet signal by an additional 100 ms. | D-103, ST-69, UC-07 | A | Draft | Room pre-delay delays only the wet onset |
| R-332 | While REVERB shows, when K2 is turned, the companion app shall change the selected track's reverb send. | D-072, ST-69, UC-07 | A | Draft | FX opens a live reverb page |
| R-333 | While DIRT MIX is zero, the firmware shall pass the track's samples through the dirt processor unchanged. | D-104, ST-69, UC-07 | A | Draft | Dirt crushes on one track with a dry bypass |
| R-334 | While DIRT shows, when K2 is turned, the companion app shall change crush for the selected track. | D-072, ST-69, UC-07 | A | Draft | FX opens the track dirt page |
| R-335 | While PERFORM shows, when SELECT is turned, the companion app shall move to the next or previous performance pair. | D-072, ST-69, UC-07 | A | Draft | PERFORM switches live effect pairs |
| R-336 | While a DUB performance overlay is applied, the firmware shall retain the base delay settings. | D-105, ST-69, UC-07 | A | Draft | Performance pairs preserve the track settings |
| R-337 | When WAH is set near 1 kHz, the firmware shall attenuate a 100 Hz test tone more than a 1 kHz tone. | D-106, ST-69, UC-07 | A | Draft | WAH emphasizes its sweep band |
| R-338 | While the DUB pair shows, when SELECT is turned one step back, the companion app shall show WAH. | D-072, ST-69, UC-07 | A | Draft | PERFORM opens WAH one step before DUB |
| R-339 | While WOBBLE DEPTH is above minimum, the firmware shall sweep the track's low-pass cutoff at the selected SPEED. | D-107, ST-69, UC-07 | A | Draft | Wobble sweeps cutoff on one track |
| R-340 | While WOBBLE DEPTH is zero, the firmware shall pass the track's samples through the Wobble overlay unchanged. | D-107, ST-69, UC-07 | A | Draft | Wobble sweeps cutoff on one track |
| R-341 | While the PERFORM DIRT pair shows, when SELECT is turned one step forward, the companion app shall show WOBBLE. | D-072, ST-69, UC-07 | A | Draft | PERFORM opens WOBBLE after DIRT |
| R-342 | When an FMini sound's cutoff is edited during a held note, the firmware shall apply the edited cutoff to that note. | D-108, ST-69, UC-07 | A | Draft | FMini cutoff edits affect a held note |
| R-343 | While PERFORM FILTER shows for an FMini sound, when K3 or K4 is turned, the companion app shall adjust the sound's cutoff or resonance respectively. | D-072, ST-69, UC-07 | A | Draft | PERFORM FILTER edits the current FMini sound |
| R-344 | While an FMini track is on its playing page, when EDIT is tapped, the companion app shall show OSC. | D-072, ST-46, UC-07 | A | Draft | EDIT opens the native FMini oscillator and filter |
| R-345 | While the FMini FILTER page shows, when K1 is turned, the companion app shall update the same cutoff shown by PERFORM FILTER. | D-110, ST-46, UC-07 | A | Draft | EDIT opens the native FMini oscillator and filter |
| R-346 | While an FMini oscillator sounds, when its waveform is edited, the firmware shall apply the waveform to the held note. | D-110, ST-46, UC-07 | A | Draft | FMini oscillator edits affect a held note |
| R-347 | When ENV is pressed from F.ENV on a Producer sound track, the companion app shall show A.ENV. | D-072, ST-46, UC-07 | A | Draft | ENV opens the native FMini contours |
| R-348 | When the release of an FMini amp envelope is edited, the firmware shall apply its selected time to the note's release. | D-111, ST-46, UC-07 | A | Draft | FMini envelope edits change the amp release |
| R-349 | While VOICE shows for a supported sound, when K4 is turned, the companion app shall change the track's pitch bend range. | D-112, ST-46, UC-07 | A | Draft | VOICE changes the track bend range |
| R-350 | While a supported track's bend range is 12 semitones, when pitch bend reaches its upper endpoint, the firmware shall raise the sounding note by one octave. | D-112, ST-46, UC-07 | A | Draft | VOICE changes the track bend range |
| R-351 | When LFO is pressed on a Producer sound track, the companion app shall open LFO. | D-072, ST-46, UC-07 | A | Draft | LFO opens the editable FMini modulation |
| R-352 | While a note is held on an FMini track, when the LFO adds volume modulation, the firmware shall vary the note's level. | D-113, ST-46, UC-07 | A | Draft | FMini LFO changes a held sound |
| R-353 | While an FMini MOD route targets cutoff with negative amount, when the modulation wheel is raised, the companion app shall lower the active cutoff. | D-114, ST-46, UC-07 | A | Draft | MOD applies a signed cutoff route |
| R-354 | While the modulation wheel offsets the track's effects, the firmware shall retain the base effect sends. | D-114, ST-46, UC-07 | A | Draft | Wheel offsets preserve base FX sends |
| R-355 | While FMini FILTER shows, the companion app shall display the selected track's live spectrum. | D-115, ST-46, UC-07 | A | Draft | FILTER shows the FMini spectrum while playing |
| R-356 | While a C4 sine sounds on the selected FMini track, the firmware shall report the largest spectrum value in the band nearest its fundamental. | D-115, ST-46, UC-07 | A | Draft | Spectrum measures the selected FMini track |
| R-357 | While an FMini note sounds on the selected track, the companion app shall display its envelope marker on the selected envelope page. | D-116, ST-46, UC-07 | A | Draft | Sound Edit plots the sounding note's markers |
| R-358 | While an FMini note is releasing, the firmware shall report the falling envelope level. | D-116, ST-46, UC-07 | A | Draft | FMini motion reports live contour and phase |
| R-359 | While EDIT FILTER shows on a Juno-106 track, when K1 is turned, the companion app shall change the same cutoff shown on PERFORM FILTER. | D-117, ST-46, UC-07 | A | Draft | Juno EDIT FILTER shares the active cutoff |
| R-360 | While a selected Juno-106 note sounds, the firmware shall publish its energy in the selected track spectrum. | D-117, ST-46, UC-07 | A | Draft | Juno native filter edits affect a held note |
| R-361 | While an envelope page shows on a Juno-106 track, when an envelope knob is turned, the companion app shall edit the native ADSR shared by both envelope pages. | D-118, ST-46, UC-07 | A | Draft | Juno envelope edits appear on both pages |
| R-362 | While the selected Juno-106 voice is releasing, the firmware shall report its native envelope level. | D-118, ST-46, UC-07 | A | Draft | Juno reports its shared native envelope |

**The Sound Browser (ST-68)** (D-087; drafts for the owner, from the Sound Browser Lab approved on 2026-11-21.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-309 | When PRESETS is turned on a synth track's page, the companion app shall show the sound list on the group of the track's sound. | D-087, ST-68 | A | Draft | PRESETS opens the sound list on the track's group, K1 jumps a group while it shows, and it closes 2 s after |
| R-310 | While the sound list shows, when K1 is turned, the companion app shall choose the next group's sound. | D-087, ST-68 | A | Draft | PRESETS opens the sound list on the track's group, K1 jumps a group while it shows, and it closes 2 s after |
| R-311 | When 2 s pass with no turn and no key held, the companion app shall close the sound list. | D-087, ST-68 | A | Draft | PRESETS opens the sound list on the track's group, K1 jumps a group while it shows, and it closes 2 s after |

**ARP on every track (ST-77, ST-78)** (D-088; approved by the owner on 2026-11-19, from the Arp Lab.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-302 | When ARP is tapped, the companion app shall switch the track's arpeggiator on or off. | D-088, ST-77 | A | Approved (owner) | ARP plays the notes held up in 16ths, and latch keeps it going |
| R-303 | When ARP is tapped twice within 350 ms, the companion app shall switch latch on or off. | D-088, ST-77 | A | Approved (owner) | ARP plays the notes held up in 16ths, and latch keeps it going |
| R-304 | While ARP is on, the companion app shall play the notes held in the order K1 MODE sets, at K2 RATE, over K3 OCTAVES, with K4 GATE, in the rhythm SELECT chooses. | D-088, ST-77 | A | Approved (owner) | ARP plays the notes held up in 16ths, and latch keeps it going |
| R-305 | While ARP is on the DRUMS track, the companion app shall repeat the sound held at K2 RATE with the velocity K1 RAMP sets and the pitch K3 PITCH sets. | D-088, ST-78 | A | Approved (owner) | ARP on the drums repeats a hat at the rate |
| R-306 | While the ARP page shows, when SELECT turns, the companion app shall show the numbered list of rhythms, each drawn as its 16 steps with their accents, notes, ghosts and rests. | D-088, ST-77 | A | Approved (owner) | ARP plays the notes held up in 16ths, and latch keeps it going |

**The song kept over power-off (ST-89)** (D-085; approved by the owner on 2026-11-19, where it lives on 2026-11-20 in Decision #115.)

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-297 | When the FM-1 is switched on, the companion app shall load the song it held when it was switched off. | D-085, ST-89 | A | Approved (owner) | The song is there after a power-off |

## Glossary

Terms used in requirements. A **subject** may stand before `shall` (reqlint R3); the others are defined terms.

| Term | Kind | Meaning |
|---|---|---|
| **the FM-1** | subject | An M-VAVE FM-1 running the firmware: what a person hears and sees |
| **the firmware** | subject | This repository's firmware: fm1kit and an app, as one image |
| **the hello app** | subject | `apps/hello`, the platform's demonstration app |
| **the companion app** | subject | `apps/companion`, the product firmware (D-034) |
| **the chord pad** | subject | The CHORDS track's AUTO page (called PAD until D-042) of the companion app (`apps/companion`, D-034): the key's chords on the white keys, colours on the black keys |
| **the manual page** | subject | (Retired by ST-82, #108; its requirements are in REQUIREMENTS-RETIRED.md.) The CHORDS track's IDEAS page (called MANUAL until D-042) of the companion app (`apps/companion`, D-036): the knobs build a chord, the keys and the screen show it, you play it |
| **the guided mode** | subject | The companion app's Guided mode (`apps/companion/guided.c`, D-068): lessons for a grown-up who never made a beat, a step at a time, on layout E |
| **the chord play page** | subject | The CHORDS track's PLAY page of the companion app (`apps/companion`, D-042): it names the chord of the notes held |
| **the player** | subject | fm1kit's sequence player (`kit_play_*`, `fm1kit/src/player.c`) |
| **drum map** | term | The fixed place of each drum sound on the 27 keys, the same on every kit (F3 KICK 1 ... G5 CRASH; `fm1kit/src/drums.c`, D-053): AUTO and IDEAS play a kit through it, so a sample placed on a key plays as that key's drum (D-121) |
| **the song** | subject | fm1kit's song: key, tempo, bars and four tracks of notes (`kit_song_t`, `fm1kit/src/seq.c`) |
| **the recorder** | subject | fm1kit's recording into an armed track (`kit_seq_record`, `kit_seq_note_on/off`) |
| **the song player** | subject | fm1kit's looped playback of a song (`kit_seq_play`, `kit_seq_stop`) |
| **the store** | subject | fm1kit's record that survives power-off (`kit_store_*`) |
| **each package** | subject | Each `.fwsc` file the build produces |
| **the build** | subject | `tools/build.py` and the generators it runs (`tools/gen_*.py`), making an app's firmware from this repository (D-056) |
| **each app** | subject | Each folder in `apps/` |
| **the music functions** | subject | fm1kit's `kit_pc` … `kit_scale_degree` (`fm1kit/src/music.c`) |
| **the firmware image** | subject | The app binary placed in the app area |
| **the firmware's RAM sections** | subject | `.data`, `.bss` and `.pool` of the image |
| the 24 keys | term | The 12 major and the 12 natural minor keys, one on each pitch class |
| equal-tempered pitch | term | 440 Hz × 2^((n − 69) / 12) for MIDI note n |
| tick | term | A 24th of a beat (`KIT_TICKS_PER_BEAT`); a bar of 4/4 is 96 |
| step | term | The time from one note's start to the next one's: 60000 / tempo ms, the tempo in BPM |
| piano key | term | One of the 27 note keys, F3 to G5 with no octave shift |
| level | term | The note's amplitude; for lights, one of off, dim, breathing, full |
| note-off message | term | A MIDI note-off, or a note-on with velocity 0 |
| whole record | term | Exactly the bytes of one completed save |
| update mode | term | The FM-1 off USB-MIDI and in its update path (the ROM's UBOOT, or the installer's loader) |
| candidate sector | term | One of the store's four 4 KiB sectors at 0x93000–0x96FFF |
| app area | term | Flash 0x4120 to 0x920DC, where the image goes |
| the installer | term | felucca-bsp `tools/fm1_install.py`, or any installer that speaks M-VAVE's update protocol |
