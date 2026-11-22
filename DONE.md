# Definition of Done

**Status:** v2.1; the way of working agreed with the owner on 2026-11-08 ([`docs/WAY_OF_WORKING.md`](docs/WAY_OF_WORKING.md)):
mixed requirements (approved Gherkin for behaviour, EARS for rules), models for states and interactions, mock-ups first,
the increment and release checklists below. v2.0: the owner's answers of 2026-11-06 (play tests, margins). Redefined from Sketchpad's DONE.md (v1.0, 2026-09-05)
for a firmware built by a factory of agents, for children.
**Applies to:** every change that reaches `fm1kit/`, `apps/`, `factory/`, `tools/`, `docs/`, `features/` or CI.

A change is **done** when all four are true. Not one, not three.

1. `factory/check.sh` passes, and CI is green on the PR's head.
2. Every gate below that the change touches passes, or has a recorded, dated exception (§ Exceptions).
3. The manual gates it touches (a real FM-1, a play test) have been run by the owner, or the PR lists them under
   "Needs from the owner" and the issue carries `needs-you`.
4. The PR says what it changed, which criteria it meets and how they are verified (`.github/pull_request_template.md`).

Anything less is in progress, however well it plays.

---

## Why this exists

Two bugs here looked like working code and were caught only because a gate was there. The OCT- + OCT+ combo could
enter update mode at once instead of after 5 s (D-018). The store wrote a record before the child's later changes
reached it (D-023). Both came from a time stored "in the future" by one millisecond, and the second one showed only on
the real firmware in the emulator, never on the PC. Sketchpad's version of this file came from the same lesson: a
process gap passes every check that exists.

## The gates

**Gate** is checked by a machine in `factory/check.sh` (its part in brackets). **Review** is checked by the approver of the
PR. **Owner** needs the owner.

### A. Requirements and traceability

| Gate | Requirement | How it's judged |
|---|---|---|
| **DoD-1** | Every new or changed behaviour is agreed: a Gherkin scenario approved by the owner (`@approved:owner`, with its use case `@UC-nn`) that states it completely, or a requirement in `docs/REQUIREMENTS.md` in EARS with one `shall` (playbook §5, §6) for a rule that must always hold (safety, timing, limits, theory, never-change) | Gate [trace], [reqlint]; review for the rules marked "review" |
| **DoD-2** | Every requirement names its source: a use case, a hazard or a decision | Gate [trace] |
| **DoD-3** | Every requirement names how it is verified, and every automated one has an executable scenario or gate that runs | Gate [trace] |
| **DoD-4** | A requirement that changes what a child sees, hears or learns is approved by the owner before merge | Review (`Status: Approved (owner)`) |
| **DoD-5** | Requirements made obsolete are struck through, not deleted | Review |

### B. Documentation

| Gate | Requirement | How it's judged |
|---|---|---|
| **DoD-6** | A behaviour that changed because of something learned (a bug, a measurement, prior art) is recorded as a decision `D-nnn`, with what caused it | Review |
| **DoD-7** | A change to `fm1kit.h` bumps `FM1KIT_API` and updates `docs/PLATFORM.md` and `docs/INTERFACES.md` IF-01 | Review |
| **DoD-8** | A change at a boundary (hardware, flash, USB, MIDI, audio, the factory's tools) updates `docs/ARCHITECTURE.md` or `docs/INTERFACES.md`, and their diagrams still render | Review (mermaid-cli) |
| **DoD-9** | A change to anything a hazard's mitigation relies on updates `docs/HAZARDS.md` | Review |
| **DoD-10** | `docs/TRACEABILITY.md` is current | Gate [trace] |
| **DoD-11** | A reversed decision is struck through and replaced, never deleted | Review |
| **DoD-34** | A new visual feature had a mock-up (frames, or the HTML lab for anything played) the owner saw before code | Review (linked in the PR) |
| **DoD-35** | The user manual (`docs/MANUAL.md`) describes what changed for a user, in the same PR | Review |
| **DoD-36** | A new or changed page, mode or interaction updates the state and interaction models (`docs/ARCHITECTURE.md` §3) | Review |
| **DoD-37** | A story's use case is in `docs/USE_CASES.md` and the story in `docs/STORIES.md`, extracted from the concept as agreed | Review |

### C. Tests

| Gate | Requirement | How it's judged |
|---|---|---|
| **DoD-12** | Every automated scenario passes on the host simulator **and** on the real firmware in the emulator, and the device's screen equals the host's | Gate [scenarios] |
| **DoD-13** | The portable code's unit tests pass | Gate [unit] |
| **DoD-14** | A bug fix ships with a test that fails against the old code, shown in the PR | Review |
| **DoD-15** | A new check is shown to fail: a mutant of the code it guards fails it, stated in the PR | Review |
| **DoD-16** | Anything that varies (key, scale, sound, velocity, timing) is varied in at least one test | Review |
| **DoD-17** | No gate is weakened to get green: no wider tolerance, no deleted check, no skipped scenario (CLAUDE.md rule 5) | Review |

### D. Code and the hardware's limits

| Gate | Requirement | How it's judged |
|---|---|---|
| **DoD-18** | Apps reach the hardware only through `fm1kit.h` | Gate [safety] |
| **DoD-19** | The image fits the app area with 10 % to spare; RAM and pool fit their regions | Gate [build], [safety] |
| **DoD-20** | Every sound with 8 voices stays under 75 % interrupt load | Gate [cpu] |
| **DoD-21** | The FM-1 stays updatable: pinned loader, installer handshake, UBOOT key, OCT combo (both ways), boot guard | Gate [safety] |
| **DoD-22** | Flash is written only in the update staging area and the store's region; the store never erases another firmware's data | Gate [unit]; review of `FM1_FLASH_WRITE_REGIONS` |
| **DoD-23** | Music theory matches Sketchpad in every key | Gate [oracle] |
| **DoD-24** | No 64-bit multiplies in device code; time differences that can be "in the future" are signed | Review |
| **DoD-25** | A low-level or hardware choice follows the most careful prior art among FM-1 firmwares, cited in its decision | Review |

### E. On a real FM-1 (the owner)

Run what the change touches. These three always run before a release, because they degrade silently:

| Gate | Scenario | Why always |
|---|---|---|
| **DoD-26** | It installs on a real FM-1 and goes back to stock | The emulator has no ROM update mode; only hardware proves the way back |
| **DoD-27** | Loudness through the speaker and through headphones | No gate hears a child's ears (HAZ-01) |
| **DoD-28** | Nothing sticks: notes stop, lights follow, after five minutes of play | Stuck notes and stuck lights show up only in real play (HAZ-02) |

Also when touched: velocity feel (R-006), MIDI with a real keyboard, the screen in daylight.

### F. Play test with a child (the owner)

A short session with a child of the target age (UC-00), noting what they did without help, where they got stuck and
what they enjoyed. Required **before every release**, and **before merging a change that adds or changes a lesson or
a game's flow**. Smaller changes to what a child sees or hears (a colour, a sound, a word) wait for the release's play
test. Findings become decisions or requirements, never silent tweaks. (Decided by the factory lead on the owner's
answer, 2026-11-06.)

### G. Judgement

The gates a machine cannot reach. Ask them out loud before calling something done.

| Gate | Question |
|---|---|
| **DoD-29** | Does this make UC-00 better, or only more featureful? |
| **DoD-30** | Would a child of the target age understand it without a grown-up explaining, or without reading if they don't read yet? |
| **DoD-31** | Does it sound good, or merely correct? An in-tune note in an ugly sound is a failure for a child |
| **DoD-32** | What did I *not* test that I could have? Write it down even if you don't fix it |
| **DoD-33** | If a problem came back a second time, is the fix reaching the cause or the symptom? A repeat means the layer is wrong, not the value (Sketchpad D-031; here D-018 and D-023 were the same cause) |

---

## The increment and the release (the owner, 2026-11-08)

**An increment** is a story or a fix, one PR. It is done when:
- the story and its use case are agreed and written down (DoD-37); a visual feature had its mock-up (DoD-34);
- its behaviour is in approved Gherkin scenarios or EARS rules (DoD-1), each `@auto` one running on both targets;
- `factory/check.sh` passes and CI is green; the manual (DoD-35), the models (DoD-36) and the decisions (DoD-6) are
  updated;
- each new screen was put next to its lab or mock-up in the same state and matches it (D-084); what cannot match is
  listed in the PR;
- the PR lists what needs the owner: screens to approve, tests on a real FM-1, a play test.

**A release** is an increment ready to ship: new features or fixes, merged on `main`. It is done when, in addition:
- every screen it shows is approved (`factory/approved/`);
- the standing gates on a real FM-1 pass (DoD-26 to DoD-28), with the `@manual` scenarios it touches;
- a play test with a child, when what a child sees, hears or learns changed (F);
- the hazards and known gaps are reviewed;
- it is tagged `vMAJOR.MINOR.PATCH` on `main`, its package (`companion.fwsc`) attached to the GitHub release, and
  `CHANGELOG.md` says what changed for a user.

## Metrics and thresholds

Read from `factory/check.sh`. A threshold is a floor, not a target to sit on.

| Metric | Threshold | Current (2026-11-06) | Part |
|---|---|---|---|
| Requirement wording violations | 0 (waivers need a reason) | 0, 42 requirements | reqlint |
| Broken links (requirements, scenarios, checks, sources, hazards) | 0 | 0; 20 scenarios, 18 automated | trace |
| Scenario checks passing, host and emulator | 100 % | 58 / 58 | scenarios |
| Device screen vs host | ≤ 0.5 % pixels differ | 0.00 % | scenarios |
| Pitch | within the scenario's cents (15 or 25) | +0 to +6 cents | scenarios |
| Music theory vs Sketchpad | 0 differences | 0 of 840 | oracle |
| Interrupt load, every sound, 8 voices | < 75 % | 52–56 % max (varies by run) | cpu |
| Image size | ≤ 90 % of the app area | 30 % (171 KB) | build, safety |

The margins (75 % CPU, 10 % flash) are kept on purpose: an app's own work runs on top of the platform's, and the cycle model is close to the chip but not exact. Decided by the factory lead on the owner's answer, 2026-11-06.
| Output peak | ≤ −6 dBFS | −6.0 dBFS | scenarios |
| Safety checks | all | all pass | safety |
| Hazards without a mitigating requirement | 0 | 0; 2 of 8 open (HAZ-01, HAZ-02, waiting on #4) | trace |

## Scaling the gates to the change

Not every change needs every gate. What it never gets is a pass on a whole category.

| Change | Requirements | Docs | Tests | Manual |
|---|---|---|---|---|
| **Platform feature** (fm1kit) | New requirements, approved by the factory lead | Decision; PLATFORM and INTERFACES; API bump | Scenarios on both targets; unit tests; a mutant | DoD-26 at the next release |
| **App feature** (what a child gets) | New requirements, **approved by the owner** | Decision; use case | Scenarios; screens to approve | Play test (F); E as touched |
| **Bug fix** | Amend the requirement it broke, or add the missing one | Decision if the cause was structural | A test that fails on the old code | The affected scenario on hardware if it was found there |
| **Refactor** | None | None | No new tests; every gate passes unchanged. If a metric moves, it wasn't a refactor | None |
| **Safety part** (update path, loader, boot guard, flash regions, gates) | As needed | HAZARDS | A mutant for every new check | **The owner's say-so before merge** (CLAUDE.md rule 3) |
| **Documents only** | As applicable | — | trace, reqlint | None |

## Who may change a gate

The decision tiers in `CLAUDE.md` apply to the gates themselves:

| Change | Who |
|---|---|
| Add a check, tighten a threshold, add a scenario | The factory lead (and say so in the PR) |
| Loosen a threshold, remove a check, skip a scenario, add a reqlint waiver | The owner |
| Approve reference screens | The owner (`factory/run.py --approve`) |
| Change the loader pin, the update path, the boot guard, the flash write regions | The owner |
| Change this file | The owner approves; the factory lead proposes |

## Exceptions

A gate may be skipped. It may not be skipped silently. Record it with a date, the gate, what was skipped, why, and what
would close it. An exception with no closing condition is a decision, and belongs in `docs/PLATFORM.md` as one.

| Date | Gate | What was skipped | Why | Closes when |
|---|---|---|---|---|
| 2026-11-06 | DoD-26, 27, 28 | Nothing has run on a real FM-1 yet | No install has happened; the owner holds the hardware | #2 is done |
| 2026-11-06 | DoD-12 (screens) | No reference screens approved: scenarios check device = host, not that the screen is right | Approving is the owner's | The owner approves the current screens |
| 2026-11-06 | R-006, R-038 and the top-row velocity | Verified by inspection or not at all (velocity on C#5..G5 is fixed at 100) | The emulator doesn't model the top row's probe | #3, after #2 |

## What "done" does not mean

- **Not "released".** A release is an increment ready to ship: tagged, its package attached, the release checklist
  above done, and the three standing E gates passed on a real FM-1.
- **Not "finished".** The platform is done and the product hasn't started. Done means *this change is complete and
  honest*.
- **Not "bug-free".** D-018 and D-023 passed every gate that existed when they were written. Gates catch what we have
  already learned to look for, which is why DoD-32 asks what you didn't test.
