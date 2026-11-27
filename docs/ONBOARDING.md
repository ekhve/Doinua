# Onboarding: working on Doinua the way the team does

For a new agent (or person) joining. Read in this order: this file, `CLAUDE.md` (binding rules), `docs/HANDOFF.md`
(state and backlog), `docs/WORK_ORDER.md` (what to do until Thursday), then `docs/WAY_OF_WORKING.md` and `DONE.md`.
The owner (`ekhve`) is short of time and tokens: **communication is short, concrete, and batched** (section 6).

## 1. Roles

- **Owner** (`ekhve`): product owner. Decides what a child sees, hears and learns; screens; safety; licences;
  publishing; gate thresholds; priorities. Plays with the firmware on a real FM-1 and in the browser emulator.
- **Factory lead** (you): writes the code, keeps the backlog (issue **#1 "Factory status"**), opens and **merges its
  own PRs** once `factory/check.sh` passes locally and CI (`gates`) is green on the PR's head. You do not need the
  owner to merge, except for the cases in section 5.

## 2. The loop for every piece of work

1. **Issue.** Take the top item of issue #1 (or of `docs/WORK_ORDER.md`). Use the issue forms
   (`.github/ISSUE_TEMPLATE/`): Story, Problem report, Decision.
2. **Branch.** One branch and one PR per story (`git checkout -b <short-name> origin/main`). Never push code to
   `main` directly.
3. **Requirements first.** New behaviour gets requirements in **EARS** (one `shall` each) in `docs/REQUIREMENTS.md`,
   written by `docs/REQUIREMENTS-PLAYBOOK.md`, with Status **Draft** (the owner approves them later). Next free
   numbers: **R-316**, **D-095**, **ST-92**, **API 37** (check before use: `grep -o "R-[0-9]*" docs/REQUIREMENTS.md`).
4. **Scenario before or with the code.** Each `@auto` requirement gets a Gherkin scenario in
   `features/fm1-scales.feature` (tags `@D-nnn @ST-nn @auto`, or `@gate:unit`) and an executable check: a
   `factory/scenarios/<app>_<name>.json` whose `"scenario"` is the Gherkin title, or a unit test
   `factory/unit/test_<module>_<name>.c`. `python3 factory/trace.py --write` regenerates `docs/TRACEABILITY.md`;
   `python3 factory/reqlint.py` checks the wording.
5. **Code.** Apps use only `fm1kit.h`. A change to `fm1kit.h` bumps `FM1KIT_API` and adds a D-nnn row in
   `docs/PLATFORM.md` (both backends implement every function).
6. **Gates.** `factory/check.sh` (about 1 h locally with the emulator; CI about 45 min). Report its result as it is.
7. **Docs in the same PR.** D-nnn row (decision, who decided, why), MANUAL section if a user sees it, README licence
   line for anything copied.
8. **PR** filling `.github/pull_request_template.md` (What / Acceptance criteria table / Gates / Decisions / Needs
   from the owner). `Closes #n`.
9. **Merge** when local check.sh and CI are green (merge commit, stack order). Then update issue #1.

## 3. The gates and what they mean

| Gate | Command | Notes |
|---|---|---|
| reqlint, trace | `factory/reqlint.py`, `factory/trace.py` | wording of requirements; every requirement has a check |
| build | `tools/build.py <app>` | prints image / RAM / pool sizes against their limits |
| unit tests | in check.sh: `test_<module>[_x].c` linked with `fm1kit/src/<module>.c` | engines tests also link `juno.c fxbus.c`, synth tests `fxbus.c` |
| music oracle | `factory/oracle/check.py` | music theory must equal Sketchpad's (never edit golden.json by hand) |
| safety | `factory/safety.py` | image within **90 % of the app area = 523,407 B**; apps use only fm1kit; the update path works |
| usb midi | `factory/usb_midi.py` | |
| **cpu** | `factory/cpu.py companion` and `hello` | every sound, 8 keys; four tracks; twelve voices; interrupts **< 80 %** (cycle-modelled, `FM1_STRICT`, USB cable in) |
| scenarios | `factory/run.py --no-build [files]` | host simulator AND emulated firmware; screens vs `factory/approved/` |

**CPU, in practice.** Today the heaviest sound is 79 %. Before adding work in the audio path:
- measure one sound: `FM1_STRICT=1 FM1_BENCH_KEYS="900:e6:+N,<keys after 900+N*8 ms>" fm1-live --usb --fwsc build/hello/hello.fwsc --shot <ms>` (see `factory/cpu.py` `load()`);
- profile: `FM1_TPROF=prof.txt,FROM,TO` with `--shot`, map PCs with `objdump -t build/<app>/<app>.elf`;
- a new engine's cost goes into the voice budget: `VOICE_COST[]` (tenths of % per voice) and `pcost()` (per track)
  in `fm1kit/src/synth.c`, measured with 0, 1, 2, 4, 5 keys on its heaviest preset (D-091);
- the song view of the CPU: `python3 factory/cpu_timeline.py companion` (a tool, about 20 min).

**Flash, in practice.** `tools/build.py companion` prints the image size; keep it under 523,407 B (today about
432 KB). Big tables go in flash as `static const`; large RAM buffers in the pool with `KIT_BIG`
(`__attribute__((section(".pool")))`).

## 4. Recipes for the next tasks

### 4a. Porting an engine (as FMini D-086 and the Juno D-090 were)

1. Copy the engine source into `fm1kit/src/<engine>/` with its SPDX line and a header naming the upstream repo and
   commit. Mark every change "fm1-scales". Check the licence is compatible (GPL-3.0-only project; MPL-2.0 and
   GPL-3.0 are fine). **Unclear licence or copied factory patches of a commercial synth: ask the owner** (section 5).
2. A generator `tools/gen_<engine>.py` reads the owner's picks `assets/sounds/<engine>.txt` (one line: index and
   name, names checked) and writes `fm1kit/src/<engine>/<engine>_bank.h`. Picks come from `assets/sounds/browser.json`
   (the 208 the owner chose; `engine` field).
3. Wire it: either into `engines.c`'s `ENGS[]` (if it fits the Felucca `engine_t` interface), or as its own
   translation unit like `juno.c` (`kit__<engine>_*` hooks called from `engines.c` under the kit's lock). Add
   `KIT_ENGINE_<X>` to `fm1kit.h` (API bump), the mapping in `synth.c` `eng_of`, `kit__eng_count`, names and presets.
4. **No 64-bit multiplies**; floats only in a file built with the FPU flags (`tools/build.py` `FPU_FLAGS`).
5. Its effects: `kit__eng_fx` in `engines.c` returns the preset's sends (DRIVE CHORUS DELAY REVERB) (D-093/D-094).
6. Level: one note about as loud as the others (compare RMS with Analog/Piano on host), a 4:1 knee over 40000.
7. Voices and CPU: measure (section 3), set the voice cap and `VOICE_COST`/`pcost`, run `cpu.py` for both apps.
8. Tests: `factory/unit/test_engines_<engine>.c` (every pick sounds, stays under clip, ends after release, in tune,
   voice cap) and `factory/scenarios/hello_<engine>.json` (PRESETS to a clean preset: clicks = its index in
   `app-host --list-sounds` minus 1; keys after the clicks, about 8 ms each).
9. Sound Browser: add the engine to `BUILT` (and `AS` if the browser's name differs) in `tools/gen_browser.py`, run
   it. **The Sound Browser screens change: they need the owner's approval** (section 5), and scenarios that walk the
   browser by click counts may move (fix the counts, not the checks).
10. Docs: D-nnn, Draft R-nnn, Gherkin, README licence line, MANUAL sound list.

### 4b. A screen (sound edit pages, FX pages)

Build it **as its lab draws it** (`docs/labs/`, rule 10 / D-084): same layout, names, places, colours, messages. Put
each new screen next to the lab in the same state and fix what differs; list remaining differences in the PR. New
screens show as "new" in the scenarios until the **owner** approves them in the conversation; then you run Approve
screens on their say-so and fast-forward your branch to `screens-approved-<run>` (D-123). Never approve screens yourself.

### 4c. A scenario

`factory/run.py`'s docstring has the format. Keep the first event at 800 ms or later. The emulated device plays
notes about 25 ms after the host: check pitch on sounds with a quick attack, not right at a slow attack. Every sound
now has its preset's effects: a "silence" check after a note must wait for its echo and reverb (about 0.8 s for the
bass). Never widen a tolerance to get green; if a check is wrong, say why in the PR.

## 5. What you decide, and what you ask

**Just do it:** implementation details, refactors that pass the gates unchanged, fixes that restore the spec,
scenario timing that follows a sound change (same checks), tools.

**Decide, then tell** (record a D-nnn "decided by the agent", list it in the PR): reversible technical choices inside
an agreed story: data formats, voice caps within the CPU gate, levels, which preset a test uses, file layout.

**Ask first, and wait** (the work that depends on it stops):
- anything a child sees, hears or learns that was not agreed: new screens or changed ones (approval), sound changes
  to existing sounds, game content;
- **licences**: copying code or factory patches whose licence is unclear or commercial (Sequential, Roland, Casio);
- safety parts (loader, update/UBOOT path, boot guard, `safety.py`, `build.py` checks), flash layout, volume;
- the gates: thresholds (CPU 80 %, the 90 % flash limit), approvals, skipping a check;
- breaking API changes, new dependencies, version pins (submodules), scope and priority;
- anything that leaves the repo: releases, publishing, upstream PRs, installing on hardware.

## 6. How to talk to the owner

- **Batch.** One message (or one issue comment) a day at most, unless blocked. The owner reads on a phone.
- **Shape:** what is done (one line each, with the PR link), what is next, and the questions. Each question with
  2–3 options, their trade-off in one line, and your recommendation first.
- **Record** every owner answer: a **Decision issue** (form), then a D-nnn row in `docs/PLATFORM.md`.
- **Needs-you** items go on issue #1's checklist (label `needs-you` on separate issues if they need discussion).
- Say plainly what failed and why; never report green that is not green.
- Use the owner's words for features (PRESETS, ARP, the Sound Browser…), not internal names.
