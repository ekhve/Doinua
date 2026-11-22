# fm1-scales — Hazard log

What could hurt a child, damage the owner's FM-1 or lose someone's data because of this firmware, and what the
firmware does about it. Each hazard traces to the requirements that mitigate it (`REQUIREMENTS.md`, Source column
"HAZ-nn"), and through them to the scenarios and gates that verify them (`TRACEABILITY.md`).

Scope: the firmware. The FM-1 itself (its plastics, battery, small parts, electrical safety) is M-VAVE's product and
outside it. Changing or removing a mitigation is the owner's decision (playbook §3, rail 3).

**Severity:** **High**: harm to a person (hearing, seizure). **Medium**: the FM-1 can't be updated or used, or
someone's data is lost. **Low**: an annoyance with a workaround.
**Status:** **Mitigated**: every mitigation is a requirement with an automated or inspected verification. **Open**:
a mitigation is missing or verified only partly; the action says what closes it.

| ID | Hazard | Harm | Cause | Severity | Mitigations | Status |
|---|---|---|---|---|---|---|
| HAZ-01 | Sound too loud | Hearing (headphones on a child) | Many loud voices summed; MASTER at full; a bug in mixing | High | R-035 output ceiling −6 dBFS; `kit_volume_limit` (apps set a ceiling: hello 85 %); R-015 no CPU overload glitches | Open: a child-safe level through headphones is not specified yet (needs the owner: #4, a K-mode requirement) |
| HAZ-02 | A note that never stops | Distress, loud drone | A MIDI note-off that never comes; an app bug | Medium | R-021, R-025 notes stop on release; `kit_all_notes_off` on sound change and before saves | Open: no platform rule for a note-off that never arrives (needs an app requirement with the unwanted-case prompt, playbook §4.4) |
| HAZ-03 | Lights or screen flashing | Photosensitive seizure | An app flashing the screen or the key lights | High | R-038 fewer than 3 full-screen flashes a second; the breathing light is one cycle per 1.13 s | Mitigated by inspection (R-038, Mode I) |
| HAZ-04 | The FM-1 can no longer be updated | The device is stuck with our firmware | A crash at boot; the update path broken; a bad loader | Medium | R-011, R-031 installer and UBOOT key; R-012, R-032 OCT combo; R-036 boot guard; R-013 pinned loader; R-019 no writes to the boot area | Mitigated (#9: the boot guard is now checked by the safety gate with a build that crashes at boot) |
| HAZ-05 | Another firmware's data destroyed | The owner loses projects or samples made with another firmware | The store erasing shared flash | Medium | R-020, R-030 only blank or own sectors; R-019 write regions | Mitigated |
| HAZ-06 | A child's saved progress lost | Lost settings or progress | Power cut during a save; flash wear | Low | R-010 power-cut safe record; R-009; R-039..R-042 the store paces its writes (at most once a minute, unchanged records not written) | Mitigated (#10: the store writes at most once a minute, only when the record changed, R-039..R-042) |
| HAZ-07 | A loud buzz while saving | Startle, discomfort | The flash stops interrupts; the audio DMA repeats its last buffer | Medium | R-037 silence while the flash is busy (the buffer is zeroed first); R-041 no write while a key is held or a voice sounds | Mitigated by inspection (R-037, Mode I) |
| HAZ-08 | Wrong music taught | A child learns a wrong note name or scale | A bug in note naming or scales | Medium | R-017 the music functions match Sketchpad in every key; R-001 notes in tune | Mitigated |
