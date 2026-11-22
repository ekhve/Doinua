# fm1-scales — Architecture, arc42 index

The architecture in [arc42](https://arc42.org)'s twelve sections, pointing to where each already lives (the owner,
2026-11-08: an index over the files, not a rewrite). Content moves here only when a file grows too long.

| # | arc42 section | Where it is |
|---|---|---|
| 1 | Introduction and goals | [README](../README.md) (what the firmware is), [concept v0.5](concepts/v0.5/CONCEPT_v0.5.md) §1, [DONE.md](../DONE.md) (quality goals), [USE_CASES.md](USE_CASES.md) UC-00 |
| 2 | Constraints | [PLATFORM.md](PLATFORM.md) (the FM-1, felucca-bsp, licences, the API), CLAUDE.md rules 1–9 |
| 3 | Context and scope | [ARCHITECTURE.md](ARCHITECTURE.md) §1 |
| 4 | Solution strategy | [ARCHITECTURE.md](ARCHITECTURE.md) §2 (the fixed API, two backends, the factory of gates) |
| 5 | Building block view | [ARCHITECTURE.md](ARCHITECTURE.md) §2, [INTERFACES.md](INTERFACES.md) IF-01 to IF-09 |
| 6 | Runtime view | [ARCHITECTURE.md](ARCHITECTURE.md) §3: a key press, the FM-1's modes, the companion's states, a chord played while recording |
| 7 | Deployment view | [ARCHITECTURE.md](ARCHITECTURE.md) §4 (flash), §5 (build); the installer in PLATFORM |
| 8 | Crosscutting concepts | theory from Sketchpad (rule 7, D-030, D-033, D-035), the song engine (D-031, D-032), the screen design (D-037), the modes (D-038), what is recorded is kept as played |
| 9 | Architecture decisions | [PLATFORM.md](PLATFORM.md) D-001… |
| 10 | Quality requirements | [REQUIREMENTS.md](REQUIREMENTS.md) (the EARS rules: timing, CPU, safety, theory), [REQUIREMENTS-PLANNED.md](REQUIREMENTS-PLANNED.md) (agreed, not built yet; their scenarios in `features/planned.feature`), [DONE.md](../DONE.md) metrics |
| 11 | Risks and technical debt | [HAZARDS.md](HAZARDS.md), PLATFORM "Known gaps", DONE.md exceptions |
| 12 | Glossary | [REQUIREMENTS.md](REQUIREMENTS.md) glossary |
