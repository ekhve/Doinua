# fm1-scales — Planned requirements

**What this is.** The requirements agreed for stories not built yet (ST-26 to ST-50, from the melody and drum labs and
the owner's reviews of 2026-11-12 and 13). Each is one EARS statement (`REQUIREMENTS-PLAYBOOK.md`); its "Verified by"
names a scenario in `../features/planned.feature`. When a story is built, its requirements move to `REQUIREMENTS.md`
and its scenarios to `fm1-scales.feature`, with their executable checks, and the traceability gate starts checking
them. Approved by the owner on 2026-11-13 ("confirmed"); `factory/reqlint.py` passes them already (checked by the agent).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-123 | When a drums track opens, the companion app shall start on its PLAY page. | UC-10, ST-50 | A | Approved (owner) | Every template opens on PLAY |
| R-153 | When the firmware is built with MIDI files in `ideas/<template>/<style>/`, the build shall make each one an idea and an AUTO pattern of that template and style. | UC-15, ST-45 | A | Approved (owner) | MIDI files become ideas and AUTO patterns in any key |
| R-154 | The companion app shall play a MIDI file's idea in the song's key, relative to the file's own key. | UC-15, ST-45 | A | Approved (owner) | MIDI files become ideas and AUTO patterns in any key |
| R-155 | If `ideas/` holds MIDI files without a `LICENSE` file, then the build shall stop. | UC-15, ST-45 | A | Approved (owner) | MIDI files become ideas and AUTO patterns in any key |

**Drafts for the owner (2026-11-19): the chords track, one pattern for the tracks, the sequencer, punch-in FX, help and
ARP** (stories ST-77, ST-78, ST-79 to ST-91; from the Chord Lab, the Track Modes page, the Sequencer Lab and the Arp Lab
the owner played and reviewed on 2026-11-18 and 19). Approved by the owner on 2026-11-19 ("approval for all, implement and test!"); each story's PR moves
its rows to `REQUIREMENTS.md` exactly as approved, with their scenarios and executable checks.

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-294 | While REC records and the song plays, when a knob turns, the companion app shall record the knob's value into the sequence playing. | ST-88 | A | Approved (owner) | A knob turned while recording becomes a MOTION lane |
| R-295 | While MOTION shows, the companion app shall list the track's recorded lanes with their values in their own units. | ST-88 | A | Approved (owner) | A knob turned while recording becomes a MOTION lane |
| R-296 | When EDIT is tapped on a MOTION lane, the companion app shall clear that lane. | ST-88 | A | Approved (owner) | A knob turned while recording becomes a MOTION lane |
| R-298 | While FX is held on PERFORM, when a white key is held, the companion app shall play that key's effect on the tracks K1 TARGET names (the four tracks, or one). | ST-90 | A | Approved (owner) | Stutter on the melody while the drums keep playing |
| R-299 | While REC records, the companion app shall record the effect punches into the sequence playing. | ST-90 | A | Approved (owner) | Stutter on the melody while the drums keep playing |
| R-300 | While GLO is held, the companion app shall show the actions of the page shown. | ST-91 | A | Approved (owner) | GLO held explains the page, and the top line names the sequence |
| R-301 | While REC is armed, the companion app shall show the number of the sequence REC records into on the top line of the page shown. | ST-91 | A | Approved (owner) | GLO held explains the page, and the top line names the sequence |

**Subjects** (as in `REQUIREMENTS.md`'s glossary): **the companion app**; **the build**, `tools/build.py` with the
user kit and the MIDI ideas it converts (to add to the glossary with the first of these stories).

**Drafts for the owner (2026-11-26): the DRUMS track as a mini MPC, from the Sampler Lab** (stories ST-95 to ST-101, use
cases UC-21 and UC-22; the lab `docs/labs/sampler-lab.html` v11 and its usage scenarios; decisions D-121). Rows that change an approved
requirement say which one they replace (Source). Each story's PR moves its rows to `REQUIREMENTS.md` exactly as approved, with
its scenarios and executable checks, and puts each new screen next to the lab in the same state (D-084). Approved by the
owner on 2026-11-26 ("Approve all, implement").

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-392 | When FX is tapped on a drums track, the companion app shall show the PUNCH, COLOR and SPACE pages in turn. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-393 | While FX PUNCH shows, the companion app shall set the compressor amount, its speed, the clipper amount and its shape with K1, K2, K3 and K4. | D-121, replaces R-136, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-394 | While FX COLOR shows, the companion app shall set the tape, lofi, drive and crush amounts with K1, K2, K3 and K4. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-395 | While FX SPACE shows, the companion app shall set the delay time, the delay feedback, the reverb type and the reverb size with K1, K2, K3 and K4. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-396 | While EDIT PAD FX shows, the companion app shall set the key's sends to PUNCH, COLOR, DELAY and REVERB with K1, K2, K3 and K4. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-397 | The firmware shall run each of a drums track's four effects once, fed by the sends of its keys. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-398 | While an effect's amounts are 0 on FX, the companion app shall show that effect as off on EDIT PAD FX. | D-121, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-399 | While a drums track sends to its four effects with eight keys sounding, the firmware shall keep the audio work under 80 % of the CPU. | D-121, D-091, ST-97 | A | Approved (owner) | The kit's effects run once, and each pad sends to them |
| R-400 | While the FM-1 is connected by USB, the firmware shall send the FM-1's stereo output to the USB host as audio. | D-121, ST-98 | H | Approved (owner) | The FM-1 is a USB audio device both ways |
| R-401 | While the FM-1 is connected by USB, the firmware shall receive the USB host's audio as the input. | D-121, ST-98 | H | Approved (owner) | The FM-1 is a USB audio device both ways |
| R-402 | While GLO USB AUDIO's MONITOR is when sampling, the firmware shall play the input through the FM-1 only while TAKE or CHOP shows. | D-121, ST-98 | H | Approved (owner) | The FM-1 is a USB audio device both ways |
| R-403 | While GLO USB AUDIO's MONITOR is on, the firmware shall play the input through the FM-1. | D-121, ST-98 | H | Approved (owner) | The FM-1 is a USB audio device both ways |
| R-404 | While GLO USB AUDIO shows, when K3 turns, the companion app shall set the input level from -12 dB to +24 dB. | D-121, ST-98 | A | Approved (owner) | The FM-1 is a USB audio device both ways |
| R-405 | When LFO is tapped on a drums track, the companion app shall show the TAKE and CHOP pages in turn. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-406 | While TAKE shows, the companion app shall draw the input's level with its peak and two marks, at -6 dBFS and -1 dBFS. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-407 | While TAKE shows with START on sound, when REC is tapped, the companion app shall start recording when the input passes -34 dBFS. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-408 | While a take records, when REC is tapped, the companion app shall stop recording. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-409 | When a take reaches 11 s, the companion app shall stop recording. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-410 | If REC is tapped on TAKE while the take was not put on a key, then the companion app shall ask before replacing it. | D-121, ST-99 | A | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-411 | The firmware shall keep one take of up to 11 s in RAM as 4-bit IMA ADPCM at 22050 Hz. | D-121, ST-99 | I | Approved (owner) | TAKE records from the input on sound and stops on REC |
| R-412 | While CHOP shows, when K4 turns, the companion app shall cut the take into 1, 2, 4, 8 or 16 equal slices or at its hits. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-413 | While CHOP shows, when a key is played, the companion app shall play the slice whose number the key's position gives. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-414 | While CHOP shows, when SELECT turns, the companion app shall choose the next slice and play it. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-415 | While CHOP shows, when OCT+ is pressed, the companion app shall split the slice chosen at the play line, or in its middle while it is silent. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-416 | While CHOP shows, when OCT- is pressed, the companion app shall join the slice chosen to the slice before it. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-417 | The companion app shall cut a take into 27 slices at most. | D-121, ST-99 | A | Approved (owner) | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-418 | While CHOP shows, when REC is tapped, the companion app shall switch the put mode on or off. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-419 | While CHOP's put mode is on, when a key is pressed twice, the companion app shall put the slice chosen on that key of the kit shown. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-420 | While CHOP's put mode is on, when a key is pressed, the companion app shall show the key and its role without playing it. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-421 | While CHOP's put mode is on with K4 PUT on each, when a key is pressed twice, the companion app shall put each slice on the keys from that key up. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-422 | If a slice is put on a key of a built-in kit, then the companion app shall ask which MY KIT to put it on. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-423 | If a slice would take a MY KIT past 7 s of sound, then the companion app shall keep the kit as it was and show that it is full. | D-121, ST-99 | A | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-424 | The firmware shall store MY KIT 1, 2 and 3 in the flash from 0xA0000 to 0xDC000, Felucca's three 80 KB sample slots. | D-121, ST-99 | I | Approved (owner) | A slice goes on a key with two presses, or every slice from a key up |
| R-425 | While EDIT TRIM shows a sample, the companion app shall set its start, end, reverse and gain with K1, K2, K3 and K4. | D-121, ST-99 | A | Approved (owner) | TRIM shortens a sample and makes it as loud as it goes |
| R-426 | While EDIT TRIM shows a sample, the companion app shall limit its GAIN to the gain that brings its peak to 0 dBFS. | D-121, ST-99 | A | Approved (owner) | TRIM shortens a sample and makes it as loud as it goes |
| R-427 | While a sample's take is gone, the companion app shall limit its START and END to the part kept. | D-121, ST-99 | A | Approved (owner) | TRIM shortens a sample and makes it as loud as it goes |
| R-428 | When ENV is pressed on a drums track page, the companion app shall show NOTES for the key played last. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-429 | While NOTES or CHORDS shows, when ENV is pressed, the companion app shall show the other of the two. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-430 | While NOTES shows 16 LEVELS on velocity, the companion app shall play the key's sound on the 16 white keys from 6 % to 100 % of its level. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-431 | While NOTES shows 16 LEVELS on tune, the companion app shall play the key's sound on the 16 white keys from 7 semitones down to 8 semitones up. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-432 | While NOTES shows FREE or KEY, the companion app shall play the key's sound at each key's own note. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-433 | While NOTES shows KEY, the companion app shall light the keys of the song's scale and its tonic in full. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-434 | While NOTES shows WHITES, the companion app shall play the song's scale on the white keys from its tonic. | D-121, ST-100 | A | Approved (owner) | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-435 | While CHORDS shows, when a white key is played, the companion app shall play the song key's chord on that key's letter with the key's sound. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-436 | While CHORDS shows and a black key is held, the companion app shall add that black key's colour to the chords played. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-437 | While CHORDS shows, when F5 is played, the companion app shall play the progression's next chord. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-438 | While CHORDS shows, when G5 is played, the companion app shall play the last chord again. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-439 | While CHORDS shows, when F#5 is played, the companion app shall keep the colour held on the chords played. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-440 | While REC records on NOTES or CHORDS, the companion app shall record each note on the drums track with its pitch. | D-121, ST-100 | A | Approved (owner) | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-441 | When a take with one pitch is recorded, the companion app shall show its note and its cents from that note. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-442 | When a take of 1.5 s or more without one pitch is recorded, the companion app shall show its key when one of the 24 keys fits it. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-443 | When a take of 1.5 s or more without one pitch is recorded, the companion app shall show its tempo when one from 70 BPM to 180 BPM fits it. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-444 | While TAKE shows a key, when SELECT turns, the companion app shall ask to make it the song's key. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-445 | While EDIT SOUND shows a pitched sound, when K1 turns, the companion app shall tune it to the nearest note first and then a semitone a step. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-446 | While FIT is speed, when a slice is put on a key, the companion app shall tune that key so the take's tempo matches the song's. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
| R-447 | While FIT is slices, when each slice is put on keys, the companion app shall offer to write the take's first bar as notes at the song's tempo. | D-121, ST-101 | A | Approved (owner) | A take shows its note, key and tempo, and fits the song |
