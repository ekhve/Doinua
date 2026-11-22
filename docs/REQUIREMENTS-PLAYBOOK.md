# Requirements playbook

How requirements are written, checked and approved in fm1-scales. It is a light version of the owner's
requirements engineering method (INCOSE-based, with EARS patterns), which is the owner's and is not in this
repository. This file takes from it what a small product built by a factory needs. Section references like "core
rules §7" point into that method. The term lists `factory/reqlint.py` checks are the project's own copy, in
`factory/reqlint-terms.json`.

## 1. What a requirement is here

One statement in `docs/REQUIREMENTS.md`, true or false of the product, with one `shall`, written in an EARS
pattern. Behaviour is specified by Gherkin scenarios in `features/fm1-scales.feature`. A requirement names the
scenario that verifies it, or is verified by inspection or judgement. Use cases (`USE_CASES.md`) say *why*, and
hazards (`HAZARDS.md`) say what must not happen. Decisions (`PLATFORM.md`, D-nnn) carry the rationale.

## 2. Roles and approval

| Who | Does |
|---|---|
| **Owner** (product owner, stakeholder) | Says what the product is for: use cases, learning content, what a child sees and hears. Approves the requirements that change those |
| **Factory lead** (the agent, on the owner's delegation of 2026-11-06) | Writes and maintains requirements, approves platform and technical ones, keeps the gates green, raises what needs the owner as issues labelled `needs-you` |
| **Requirements agent** (this playbook, the agent in that role) | Drafts from a story, asks the readiness questions, checks wording. Never approves |

A requirement is **Draft** until approved, then **Approved (agent)** or **Approved (owner)** (`trace.py` checks the
value). Changing an approved requirement is a new draft of it, approved again by the same authority.

## 3. Safety rails (from the agent's behaviour §2)

1. **Never invent domain facts.** Learning content, ages, thresholds the product promises a child, states, and
   interfaces nobody named all come from the owner or from evidence (prior art, measurements). If they are missing,
   ask.
2. **Never auto-approve.** Only the approval authority in §2 sets Approved.
3. **Never weaken a safety attribute** (hazard-derived requirements, the update path, flash, volume) without saying
   so and naming what was removed. That is the owner's decision.
4. **Rewrites are suggestions** to the authority that approves the requirement. A violation of a mandatory rule
   (R18, R6) is not optional, though: it gets fixed.
5. **Preserve intent.** Where the intent is unclear, ask before rewriting.

**Decision policy** (behaviour §3): high confidence, unambiguous fix → **suggest**; intent unclear → **ask** one
question; needs the author's judgement → **flag**; would mean inventing a fact or approving → **refuse** and say who
supplies it.

## 4. From story to requirements

1. **Readiness test** (readiness §4). Before drafting, these must be answerable, in this order: *who* does it (the
   subject), in *what state*, triggered by *what event*, with *what measurable outcome*, and *why*. Put the questions
   that can't be answered to the owner, at most five, and draft nothing until they are answered. An unanswerable
   question is a finding: it names the analysis not yet done (a glossary term, the app's state model, an interface
   event list).
2. **Classify** (classification §3): functional, quality, physical or interface, and whether it is also a
   constraint.
3. **Draft one requirement per response**, choosing the pattern by core rules §6:
   ubiquitous if always true, `While` for a state, `When` for a planned event, `If … then` for an unwanted one.
4. **The unwanted case** (core rules §6 step 7, functional FN-08). After each `When` requirement, ask what happens if
   the event is late, wrong, repeated or missing, and write the `If … then` requirements that answer. For a child on
   an FM-1 that means: two keys at once, a key held forever, every key mashed, a knob spun hard, power-off
   mid-lesson, the MASTER knob at full.
5. **Attributes**: §7.
6. **Check**: `factory/reqlint.py` (wording) and `factory/trace.py` (links), both in `factory/check.sh`.
7. **Verify**: each Mode A requirement names an `@auto` scenario that is executable (a `factory/scenarios/*.json` or
   a gate). H and K requirements name `@manual` scenarios that produce a "needs you" item when the code is merged.
8. **Approve** by §2.

## 5. EARS (core rules §5)

```
Ubiquitous:  The <subject> shall <response>
State:       While <state>, the <subject> shall <response>
Event:       When <trigger>, the <subject> shall <response>
Unwanted:    If <unwanted event>, then the <subject> shall <response>
Complex:     While <state>, when <trigger>, the <subject> shall <response>
```

Exactly one `shall`, exactly one subject from the glossary, at most one `When` / `If`, any number of `While`. The
optional-feature pattern is not used. More than three preconditions call for a decision table (R23).

## 6. The rules we apply

Of the method's R1–R42, these are enforced here. **Gate** = `reqlint` fails the build on a violation. **Warn** =
`reqlint` lists it for the reviewer. **Review** = checked by the approver. The rest of R1–R42 applies by review, as
in the method.

| Rule | What | Here |
|---|---|---|
| R1 | An EARS pattern | Gate |
| R2 | Active voice: no `shall be <verb>ed` | Gate (irregular passives like "shall be lit" by review) |
| R3 / R4 | The subject is a glossary subject; terms are defined in the glossary | Gate for the subject; review for terms |
| R6 | Units of measure, consistent (mandatory) | Review |
| R7, R34 | No vague terms (the method's lists: quantifiers, adjectives, adverbs, performance words) | Gate; conditional entries warn |
| R8 | No escape clauses ("as appropriate", "where possible") | Gate |
| R9 | No open-ended clauses ("etc.", "including but not limited to") | Gate |
| R10 | No superfluous infinitives ("shall be able to") | Gate |
| R16 | Use of "not" | Warn (the method: "evaluate case by case") |
| R17 | No "/" or "and/or" | Gate |
| R18 | One thought: exactly one `shall` (mandatory) | Gate |
| R19 | One trigger; one response ("and" / "or" after `shall`) | Gate for triggers; warn for "and"/"or" |
| R26 | No unachievable absolutes ("all", "always", "never", "100%") | Gate |
| R33, R34 | Values have ranges or tolerances; performance is measurable | Review (and the scenario must measure it) |
| R35 | No indefinite temporal terms ("immediately", "soon", "simultaneously") | Gate; conditional entries warn |

The term lists are the project's own, in `factory/reqlint-terms.json`, which `reqlint` reads (D-058).

**Waivers.** A violation that is right in context goes in `factory/reqlint-waivers.json`
(`{"R-0xx": [{"rule": "R7", "reason": "..."}]}`). A waiver needs a reason, shows in the gate's output, and is
reviewed in the PR like any change.

## 7. Attributes (attributes §3, trimmed)

| Field | Where |
|---|---|
| ID | `R-nnn`, never reused |
| Statement | The EARS sentence |
| Source | UC-nn, HAZ-nn, D-nnn (the rationale lives in the decision) |
| Verification method | Mode: A = Test, H / K = Demonstration, I = Inspection, J = judgement |
| Status | Draft, Approved (agent), Approved (owner) |
| Verified by | The scenario title, or a note for I |
| Work item | The story issue, in the PR that adds it |

## 8. Findings

Written as the method does (behaviour §4): the rule ID, the state, the evidence (the exact words, or what is
missing), then a fix or a question. For example: `[R35] VIOLATION "immediately" → state the time: "within 20 ms"`.
States: PASS, VIOLATION, POSSIBLE (a question), NOT ASSESSABLE (name what is missing).

## 9. Analysis this needs (readiness §2, the light set)

| Artefact | Where | Feeds |
|---|---|---|
| Glossary | `REQUIREMENTS.md`, Glossary | Subjects and terms (R3, R4) |
| States and modes of the FM-1 and each app | `ARCHITECTURE.md` | `While` clauses |
| Interface events, nominal and off-nominal | `INTERFACES.md` | `When` and `If` clauses |
| Hazards | `HAZARDS.md` | `If … then` requirements and safety requirements |
