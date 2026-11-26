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
its scenarios and executable checks, and puts each new screen next to the lab in the same state (D-084).

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-363 | The companion app shall play the 12 drum sounds AUTO and IDEAS use on the keys F3 to E4 of each kit, in the drum map's order: F3 kick, F#3 hat, G3 snare, G#3 open hat, A3 clap, A#3 shaker, B3 rim, C4 tom 1, C#4 ghost, D4 tom 2, D#4 perc, E4 tom 3. | D-121, ST-95 | A | Draft | The drum map puts AUTO's sounds on the first octave |
| R-364 | When a key from F3 to E4 is played on a drums track, the companion app shall show the key's drum map role. | D-121, ST-95 | A | Draft | The drum map puts AUTO's sounds on the first octave |
| R-365 | When a key from F4 to G5 is played on a drums track, the companion app shall show the name of its sound. | D-121, ST-95 | A | Draft | The drum map puts AUTO's sounds on the first octave |
| R-366 | While a drums track plays AUTO or IDEAS, the companion app shall play each drum part on its drum map key. | D-121, ST-95 | A | Draft | The drum map puts AUTO's sounds on the first octave |
| R-367 | While a drums track's PLAY page shows, when K1 turns, the companion app shall change the level of the key played last. | D-121, replaces R-129, ST-96 | A | Draft | PLAY sets the key's level and reverb, and the track's grid and feel |
| R-368 | While a drums track's PLAY page shows, when K2 turns, the companion app shall change the reverb send of the key played last. | D-121, ST-96 | A | Draft | PLAY sets the key's level and reverb, and the track's grid and feel |
| R-369 | While a drums track's PLAY page shows, when K3 turns, the companion app shall change the track's GRID. | D-121, ST-96 | A | Draft | PLAY sets the key's level and reverb, and the track's grid and feel |
| R-370 | While a drums track's PLAY page shows, when K4 turns, the companion app shall change the track's FEEL. | D-121, ST-96 | A | Draft | PLAY sets the key's level and reverb, and the track's grid and feel |
| R-371 | While REC records with a GRID set, the companion app shall record each note at the nearest position of that grid. | D-121, replaces R-140 and R-141, ST-96 | A | Draft | GRID places notes as they are recorded, and FEEL moves them afterwards |
| R-372 | While a track's FEEL is set, the companion app shall move each recorded note that lies on an off-sixteenth by that feel's swing. | D-121, replaces R-142, ST-96 | A | Draft | GRID places notes as they are recorded, and FEEL moves them afterwards |
| R-373 | While a drums track's PLAY page shows, the companion app shall draw one row for each key recorded, in the order the keys were first recorded, sharing the bar's height. | D-121, ST-96 | A | Draft | GRID places notes as they are recorded, and FEEL moves them afterwards |
| R-374 | While REC is on and the song plays, when EDIT and a key are held, the companion app shall remove that key's notes of the track as the playhead passes them. | D-121, replaces R-146, ST-96 | A | Draft | Erasing needs REC on, and SAVE undoes and redoes |
| R-375 | If EDIT and a key are held while REC is off, then the companion app shall show that erasing needs REC on. | D-121, ST-96 | A | Draft | Erasing needs REC on, and SAVE undoes and redoes |
| R-376 | When SAVE is tapped once on a page where holding it undoes, the companion app shall show HOLD: UNDO. | D-121, ST-96 | A | Draft | Erasing needs REC on, and SAVE undoes and redoes |
| R-377 | When SAVE is tapped twice within 400 ms after an undo, the companion app shall redo the change undone. | D-121, ST-96 | A | Draft | Erasing needs REC on, and SAVE undoes and redoes |
| R-378 | When EDIT is tapped on a drums track, the companion app shall show the KIT, SOUND, TRIM and PAD FX pages in turn. | D-121, ST-96 | A | Draft | EDIT builds the kit from every sound, grouped by kit |
| R-379 | While EDIT KIT shows, when PRESETS turns, the companion app shall give the key played last the next sound of the list of the kits' sounds. | D-121, ST-96 | A | Draft | EDIT builds the kit from every sound, grouped by kit |
| R-380 | While EDIT KIT shows, when K3 turns, the companion app shall give the key played last the first sound of the next kit in the list. | D-121, ST-96 | A | Draft | EDIT builds the kit from every sound, grouped by kit |
| R-381 | When a key gets a new sound, the companion app shall set that key's settings to their defaults. | D-121, ST-96 | A | Draft | EDIT builds the kit from every sound, grouped by kit |
| R-382 | While EDIT KIT shows, when K2 turns, the companion app shall set how much the key's level follows the velocity, from fixed to 10. | D-121, ST-96 | A | Draft | EDIT builds the kit from every sound, grouped by kit |
| R-383 | While EDIT SOUND shows, the companion app shall set the key's tune, attack, hold and decay with K1, K2, K3 and K4. | D-121, ST-96 | A | Draft | SOUND shapes a key with tune, attack, hold and decay, and mute groups |
| R-384 | While EDIT SOUND shows, when a knob turns, the companion app shall leave the key silent. | D-121, ST-96 | A | Draft | SOUND shapes a key with tune, attack, hold and decay, and mute groups |
| R-385 | While EDIT SOUND shows, when SELECT turns, the companion app shall set the key's mute group to off, A, B, C or D. | D-121, ST-96 | A | Draft | SOUND shapes a key with tune, attack, hold and decay, and mute groups |
| R-386 | When a key of a mute group sounds, the companion app shall stop the other keys of that group within 10 ms. | D-121, ST-96 | A | Draft | SOUND shapes a key with tune, attack, hold and decay, and mute groups |
| R-387 | While a key's HOLD is key, the companion app shall sound that key until it is released and then fade it over its DECAY. | D-121, ST-96 | A | Draft | SOUND shapes a key with tune, attack, hold and decay, and mute groups |
| R-388 | When SAVE is held on an EDIT page of a drums track, the companion app shall show MY KIT 1, 2 and 3 to save the kit to. | D-121, ST-96 | A | Draft | A kit is saved as MY KIT 1, 2 or 3 and kept |
| R-389 | When SAVE is held on the save panel, the companion app shall store the kit's keys in the MY KIT K1 chose. | D-121, ST-96 | A | Draft | A kit is saved as MY KIT 1, 2 or 3 and kept |
| R-390 | The companion app shall keep MY KIT 1, 2 and 3 over power-off. | D-121, ST-96 | A | Draft | A kit is saved as MY KIT 1, 2 or 3 and kept |
| R-391 | When PRESETS turns on a drums track's PLAY page, the companion app shall step through the built-in kits and then MY KIT 1, 2 and 3. | D-121, replaces R-126, ST-96 | A | Draft | A kit is saved as MY KIT 1, 2 or 3 and kept |
| R-392 | When FX is tapped on a drums track, the companion app shall show the PUNCH, COLOR and SPACE pages in turn. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-393 | While FX PUNCH shows, the companion app shall set the compressor amount, its speed, the clipper amount and its shape with K1, K2, K3 and K4. | D-121, replaces R-136, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-394 | While FX COLOR shows, the companion app shall set the tape, lofi, drive and crush amounts with K1, K2, K3 and K4. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-395 | While FX SPACE shows, the companion app shall set the delay time, the delay feedback, the reverb type and the reverb size with K1, K2, K3 and K4. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-396 | While EDIT PAD FX shows, the companion app shall set the key's sends to PUNCH, COLOR, DELAY and REVERB with K1, K2, K3 and K4. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-397 | The firmware shall run each of a drums track's four effects once, fed by the sends of its keys. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-398 | While an effect's amounts are 0 on FX, the companion app shall show that effect as off on EDIT PAD FX. | D-121, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-399 | While a drums track sends to its four effects with eight keys sounding, the firmware shall keep the audio work under 80 % of the CPU. | D-121, D-091, ST-97 | A | Draft | The kit's effects run once, and each pad sends to them |
| R-400 | While the FM-1 is connected by USB, the firmware shall send the FM-1's stereo output to the USB host as audio. | D-121, ST-98 | H | Draft | The FM-1 is a USB audio device both ways |
| R-401 | While the FM-1 is connected by USB, the firmware shall receive the USB host's audio as the input. | D-121, ST-98 | H | Draft | The FM-1 is a USB audio device both ways |
| R-402 | While GLO USB AUDIO's MONITOR is when sampling, the firmware shall play the input through the FM-1 only while TAKE or CHOP shows. | D-121, ST-98 | H | Draft | The FM-1 is a USB audio device both ways |
| R-403 | While GLO USB AUDIO's MONITOR is on, the firmware shall play the input through the FM-1. | D-121, ST-98 | H | Draft | The FM-1 is a USB audio device both ways |
| R-404 | While GLO USB AUDIO shows, when K3 turns, the companion app shall set the input level from -12 dB to +24 dB. | D-121, ST-98 | A | Draft | The FM-1 is a USB audio device both ways |
| R-405 | When LFO is tapped on a drums track, the companion app shall show the TAKE and CHOP pages in turn. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-406 | While TAKE shows, the companion app shall draw the input's level with its peak and two marks, at -6 dBFS and -1 dBFS. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-407 | While TAKE shows with START on sound, when REC is tapped, the companion app shall start recording when the input passes -34 dBFS. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-408 | While a take records, when REC is tapped, the companion app shall stop recording. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-409 | When a take reaches 11 s, the companion app shall stop recording. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-410 | If REC is tapped on TAKE while the take was not put on a key, then the companion app shall ask before replacing it. | D-121, ST-99 | A | Draft | TAKE records from the input on sound and stops on REC |
| R-411 | The firmware shall keep one take of up to 11 s in RAM as 4-bit IMA ADPCM at 22050 Hz. | D-121, ST-99 | I | Draft | TAKE records from the input on sound and stops on REC |
| R-412 | While CHOP shows, when K4 turns, the companion app shall cut the take into 1, 2, 4, 8 or 16 equal slices or at its hits. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-413 | While CHOP shows, when a key is played, the companion app shall play the slice whose number the key's position gives. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-414 | While CHOP shows, when SELECT turns, the companion app shall choose the next slice and play it. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-415 | While CHOP shows, when OCT+ is pressed, the companion app shall split the slice chosen at the play line, or in its middle while it is silent. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-416 | While CHOP shows, when OCT- is pressed, the companion app shall join the slice chosen to the slice before it. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-417 | The companion app shall cut a take into 27 slices at most. | D-121, ST-99 | A | Draft | CHOP cuts the take, splits and joins slices, and the keys play them |
| R-418 | While CHOP shows, when REC is tapped, the companion app shall switch the put mode on or off. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-419 | While CHOP's put mode is on, when a key is pressed twice, the companion app shall put the slice chosen on that key of the kit shown. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-420 | While CHOP's put mode is on, when a key is pressed, the companion app shall show the key and its role without playing it. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-421 | While CHOP's put mode is on with K4 PUT on each, when a key is pressed twice, the companion app shall put each slice on the keys from that key up. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-422 | If a slice is put on a key of a built-in kit, then the companion app shall ask which MY KIT to put it on. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-423 | If a slice would take a MY KIT past 7 s of sound, then the companion app shall keep the kit as it was and show that it is full. | D-121, ST-99 | A | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-424 | The firmware shall store MY KIT 1, 2 and 3 in the flash from 0xA0000 to 0xDC000, Felucca's three 80 KB sample slots. | D-121, ST-99 | I | Draft | A slice goes on a key with two presses, or every slice from a key up |
| R-425 | While EDIT TRIM shows a sample, the companion app shall set its start, end, reverse and gain with K1, K2, K3 and K4. | D-121, ST-99 | A | Draft | TRIM shortens a sample and makes it as loud as it goes |
| R-426 | While EDIT TRIM shows a sample, the companion app shall limit its GAIN to the gain that brings its peak to 0 dBFS. | D-121, ST-99 | A | Draft | TRIM shortens a sample and makes it as loud as it goes |
| R-427 | While a sample's take is gone, the companion app shall limit its START and END to the part kept. | D-121, ST-99 | A | Draft | TRIM shortens a sample and makes it as loud as it goes |
| R-428 | When ENV is pressed on a drums track page, the companion app shall show NOTES for the key played last. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-429 | While NOTES or CHORDS shows, when ENV is pressed, the companion app shall show the other of the two. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-430 | While NOTES shows 16 LEVELS on velocity, the companion app shall play the key's sound on the 16 white keys from 6 % to 100 % of its level. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-431 | While NOTES shows 16 LEVELS on tune, the companion app shall play the key's sound on the 16 white keys from 7 semitones down to 8 semitones up. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-432 | While NOTES shows FREE or KEY, the companion app shall play the key's sound at each key's own note. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-433 | While NOTES shows KEY, the companion app shall light the keys of the song's scale and its tonic in full. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-434 | While NOTES shows WHITES, the companion app shall play the song's scale on the white keys from its tonic. | D-121, ST-100 | A | Draft | ENV plays a pad on 16 levels, or as notes in the song's scale |
| R-435 | While CHORDS shows, when a white key is played, the companion app shall play the song key's chord on that key's letter with the key's sound. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-436 | While CHORDS shows and a black key is held, the companion app shall add that black key's colour to the chords played. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-437 | While CHORDS shows, when F5 is played, the companion app shall play the progression's next chord. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-438 | While CHORDS shows, when G5 is played, the companion app shall play the last chord again. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-439 | While CHORDS shows, when F#5 is played, the companion app shall keep the colour held on the chords played. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-440 | While REC records on NOTES or CHORDS, the companion app shall record each note on the drums track with its pitch. | D-121, ST-100 | A | Draft | ENV CHORDS plays the key's chords with a pad, as the CHORDS track's pads |
| R-441 | When a take with one pitch is recorded, the companion app shall show its note and its cents from that note. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-442 | When a take of 1.5 s or more without one pitch is recorded, the companion app shall show its key when one of the 24 keys fits it. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-443 | When a take of 1.5 s or more without one pitch is recorded, the companion app shall show its tempo when one from 70 BPM to 180 BPM fits it. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-444 | While TAKE shows a key, when SELECT turns, the companion app shall ask to make it the song's key. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-445 | While EDIT SOUND shows a pitched sound, when K1 turns, the companion app shall tune it to the nearest note first and then a semitone a step. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-446 | While FIT is speed, when a slice is put on a key, the companion app shall tune that key so the take's tempo matches the song's. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
| R-447 | While FIT is slices, when each slice is put on keys, the companion app shall offer to write the take's first bar as notes at the song's tempo. | D-121, ST-101 | A | Draft | A take shows its note, key and tempo, and fits the song |
