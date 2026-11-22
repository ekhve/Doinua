# Concepts

**Concepts are exploratory** and not part of a release: the stories (`../STORIES.md`), use cases (`../USE_CASES.md`),
requirements and Gherkin scenarios are extracted from them when the owner agrees (`../WAY_OF_WORKING.md`).

How fm1-scales is meant to be used, before it becomes requirements. A concept is a proposal, versioned, with
pictures. Once the owner agrees on one, its walk-throughs become use cases (`../USE_CASES.md`) and requirements
(`../REQUIREMENTS.md`), story by story. A concept is never edited after it is superseded; the next version says what
changed.

| Version | By | Status | What it is |
|---|---|---|---|
| [v0.1](v0.1/CONCEPT_v0.1.md) | agent | Superseded by v0.2 | Three modes on ALGORITHM (PLAY, MAKE, GAME), a control map and mock-ups (`make_mockups.py`) |
| [v0.2](v0.2/CONCEPT_v0.2.md) | owner | **Current direction**, being evaluated | One four-track song engine, three experiences (Producer, Guided, Game), key and tempo on GLO, any track first, key AUTO by analysis |
| [v0.2 evaluation](v0.2/EVALUATION.md) | agent | For the owner | What holds, 13 corrections and risks, answers to v0.2's questions, 4 new questions |
| [Firmware screens](v0.2/firmware_game_screen.png) | firmware | Built (#22) | The game and study screens as the FM-1 draws them ([study](v0.2/firmware_study_screen.png)): proof that the firmware can draw this kind of screen (D-027) |
| v0.3 | owner | Workflow walkthrough (8 scenarios) | The owner's journeys P1–P3, G1–G2, M1–M2, X1 (files to be added) |
| [v0.4](v0.4/BRANCHING.md) | agent | Draft for the owner | The branching map (modes × what the song has × pages), chord PAD keys, chord lane, sounds; with [the simulator](v0.4/simulator.html) · [chord pad and manual chords](v0.4/CHORD_PAD.md): scenarios, controls, requirements. **Chord pad and manual chords approved** (2026-11-07): R-075–R-082 and R-085–R-094, built in `apps/companion` (D-034, D-036), how to use them in [`../MANUAL.md`](../MANUAL.md) |
| [v0.5](v0.5/CONCEPT_v0.5.md) | agent, from the owner's direction | Draft for the owner (2026-11-08) | **Start a song anywhere**: what the firmware is, the ladder of help on every track, branching, GLO, HOME and the modes · [user stories](v0.5/STORIES.md) · [usage scenarios](v0.5/SCENARIOS.md) · [styles](v0.4/STYLES.md) |

**Settled so far** (D-026): for children and grown-ups; the game waits at each note; a wrong key gives a soft bonk;
keys outside the scale play softly by default; triads first.
