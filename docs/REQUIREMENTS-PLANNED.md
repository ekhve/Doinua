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

**ST-92, Bass Something 2 (Draft, work order of 2026-11-23).**

| ID | Requirement | Source | Mode | Status | Verified by |
|---|---|---|---|---|---|
| R-316 | When a Bass Something 2 sound is chosen, the companion app shall select its matching factory patch. | ST-92, UC-07 | A | Draft | The Sound Browser selects each picked Bass Something 2 patch |
| R-317 | While a track plays a Bass Something 2 sound, the firmware shall sound at most one of the track's notes at once. | ST-92, UC-07 | A | Draft | Bass Something 2 plays one voice per track |
