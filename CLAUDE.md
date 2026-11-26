# Working in fm1-scales (rules for agents)

This repository is built as a software factory. You write code; the gates decide whether it works.

## Before you call anything done

Run `factory/check.sh` and report the result as it is. It runs the build, the music oracle, the
safety gate and every scenario on the host simulator and the emulated firmware. If the toolchain or
emulator is missing, run `tools/setup.sh` first.

## Rules

1. **Apps use `fm1kit.h` and nothing else.** No BSP headers, no register addresses (`safety.py` checks this).
2. **Every new behaviour gets a scenario** in `factory/scenarios/` before or with the code. A behaviour
   without a scenario is one nobody agreed to.
3. **Never touch the safety parts without a person's say-so:** `factory/loader.sha256`, the update and
   UBOOT paths and the boot guard in `fm1kit/src/device.c`, `factory/safety.py`, `tools/build.py`'s
   checks. A change there can make an FM-1 impossible to update.
4. **Never approve screens yourself** (`factory/run.py --approve`, `factory/approved/`). That is a
   person's call. New screens show as "new" until then. The owner approves them in the conversation (D-123: the owner does not run
   workflows); on that say-so you run the **Approve screens** workflow (D-078), record the owner's words on the PR, and
   fast-forward the branch to the `screens-approved-<run>` it pushes.
5. **Never weaken a gate to get green**: no wider tolerances, no deleted checks, no skipped scenarios.
   If a check is wrong, say so and why.
6. **The API is versioned.** A change to `fm1kit.h` bumps `FM1KIT_API` and is recorded as a decision in
   `docs/PLATFORM.md`. Both backends (`device.c`, `host.c`) implement every function.
7. **Music theory follows Sketchpad.** If `factory/oracle/check.py` disagrees, the C port is wrong. Regenerate
   `golden.json` only from Sketchpad (`node factory/oracle/gen_golden.mjs /path/to/Sketchpad`).
8. **No 64-bit multiplies in device code** (felucca-bsp `docs/CPU.md`: this CPU gets them subtly wrong).
9. `build/` and `work/` are outputs: never commit them.
10. **The lab is the contract for the screen** (D-084): build a screen as its lab draws it, and before asking for
   approval put each new screen next to the lab in the same state and fix what differs.

## How work is organised

The way of working (agreed with the owner, 2026-11-08) is in `docs/WAY_OF_WORKING.md`: concepts and mock-ups explore;
stories (`docs/STORIES.md`), use cases (`docs/USE_CASES.md`), approved Gherkin scenarios and EARS rules are what is
agreed; `DONE.md` holds the increment and release checklists.

**The factory lead** (the agent, on the owner's delegation of 2026-11-06) runs the backlog: it orders the work in the
**Factory status** issue (the ordered backlog and what needs the owner), works top first, and labels anything that
needs the owner `needs-you`: decisions, screens to approve, tests on a real FM-1, play tests. The owner
follows it on the project board and is consulted as a stakeholder for design.

**Merging** is the factory lead's (the owner, 2026-11-06: "I rather you do the updates and merges"): it merges its own
PRs, in stack order, once `factory/check.sh` and CI pass on the PR's head. Still the owner's before merge: anything
under rule 3 (safety parts) and rule 4 (screen approvals), and a requirement that changes what a child sees, hears or
learns (DONE.md DoD-4).

**Issues** (forms in `.github/ISSUE_TEMPLATE/`), on the project board:
- **Story**: one increment of the software, as a user story with acceptance criteria (requirements `R-nnn` and/or
  Gherkin scenarios), taken from the top of the Factory status backlog.
- **Problem report**: something that doesn't do what it should, with severity S1–S3, steps (a key script when
  possible) and the requirement it breaks. An S1 (update, boot, data, safety) goes before any story.
- **Decision**: a choice that belongs to the product owner, assigned to `ekhve`.

**A story, step by step**
1. Its requirements and scenarios come from the story (written by the requirements agent, approved by the owner). Add
   the requirements to `docs/REQUIREMENTS.md` and the scenarios to `features/fm1-scales.feature` exactly as approved.
   **Never change an approved requirement**: if one is wrong or impossible, raise a Decision.
2. Implement each `@auto` scenario as an executable check (`factory/scenarios/*.json` with `"scenario"` set to its
   title, or a gate), then the code. `factory/trace.py` checks the links.
3. One branch and one PR per story; the PR says `Closes #n` and fills in `.github/pull_request_template.md`.
4. Done means `factory/check.sh` passes and the PR lists what still needs the owner (screens, hardware, play test);
   then the factory lead merges it.

**Which decisions come to the owner**
- **Ask first**: the dependent work stops until answered. Learning content and approach; what the child sees and
  touches; safety (update path, loader, flash, volume); breaking API changes, dependencies, licence, version pins;
  scope and priority; anything that leaves the repo (release, publish, upstream, install on hardware); the gates
  themselves (thresholds, approvals). Ask in the conversation, with the options, their pros and cons, and a
  recommendation; record the question and the answer as a Decision issue and the result as a `D-nnn`.
- **Low-level and hardware questions** (flash layout, drivers, timing, formats, coexistence with other firmwares):
  first look at how other FM-1 firmwares solve it (Felucca, felucca-bsp, Optimist / fm1-base, SLOOP, Melodee, X0X,
  ...; clone and read them), follow the most careful prior art, decide, and tell: record a `D-nnn` that cites
  what they do. Ask only if the prior art conflicts on something that changes what the child gets or the risk to
  the owner's FM-1.
- **Decide, then tell**: reversible technical choices within the approved spec. Record a `D-nnn` (decided by:
  agent) and list it in the PR.
- **Just do it**: implementation details, refactors that pass the gates unchanged, fixes that restore the spec.

## Where things are

- The API: `fm1kit/include/fm1kit.h`, explained in `docs/PLATFORM.md` (also the decision log, D-nnn).
- Requirements `docs/REQUIREMENTS.md` (EARS, one `shall`), written by `docs/REQUIREMENTS-PLAYBOOK.md`; use cases
  `docs/USE_CASES.md`; hazards `docs/HAZARDS.md`; scenarios `features/fm1-scales.feature`; architecture and
  interfaces `docs/ARCHITECTURE.md`, `docs/INTERFACES.md`; the generated `docs/TRACEABILITY.md`.
- Scenario format: the docstring at the top of `factory/run.py`.
- Key script syntax (scenarios, `app-host --keys`): `T:C4@100` press, `T:C4^` release, `T:PLAY`,
  `T:e0:+2` (encoder 0 = SELECT, 1 ALGORITHM, 2..5 KNOB1..4, 6 PRESETS), `T:pot:900`. Times in ms from
  boot; keep the first event at 800 ms or later.
