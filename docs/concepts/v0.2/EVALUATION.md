# Evaluation of concept v0.2

**By:** the factory lead (agent), 2026-11-06. **Of:** [`CONCEPT_v0.2.md`](CONCEPT_v0.2.md), the owner's revision.
**Checked against:** the FM-1 as fm1kit drives it (`docs/PLATFORM.md`), the decisions so far (D-001 to D-026), the
hazard log, Sketchpad's code, and other FM-1 firmwares (Felucca, fm1-chord, ChoralRoot, fm1-x0x, SLOOP).

## Verdict

**Adopt v0.2 as the direction.** It is clearer than v0.1 in the ways that matter:

- **One song engine, three experiences.** Producer, Guided and Game differ only in who chooses the next action and
  how things are shown. There's one engine to build and test, and songs move between the experiences.
- **Any track can start a song**, and the key can be chosen *or* inferred. Starting from a bassline becomes as
  natural as starting from chords.
- **The recorded performance is the truth.** Analysis suggests; it never rewrites notes or secretly changes the key.
  This is the rule a composition assistant needs, and it can be tested exactly.
- **Key and tempo on GLO**, so the four knobs stay for music. **ALGORITHM selects the track**, as in Felucca, so
  FM-1 owners feel at home.
- **Kids or grown-up is a presentation, not a difficulty.** Both views run the same note matching, so it's tested
  once.
- **Song packs are original works in a style, plus public-domain songs.** This matches v0.1 section 4.4 and keeps
  the firmware publishable.

The points below are corrections and risks to settle before the requirements. None of them changes the direction.

## Corrections and risks

| # | v0.2 says | What we found | Proposal |
|---|---|---|---|
| C1 | "Reuse Felucca's four-track audio, sequencer and keyboard" | We don't run Felucca. We build on felucca-bsp and Felucca-Engines with our own kit (D-001), which today plays **one sound at a time**. Felucca proves four tracks with their own engines fit this CPU, and its code is GPL-3.0 like ours (D-002), so pieces can be ported | The first platform story: **four tracks, each with its own sound** (fm1kit API 7), a recorder, and drums. Port from Felucca where it fits; our gates check it |
| C2 | Voicings "within Felucca's polyphony" | The kit has **8 voices**. The CPU gate measures 8 voices of one sound at up to 56 %; four *different* engines at once are unmeasured. Triad (3) + bass (1) + melody (1–2) + drums ≈ 8, which fits. Sevenths and add9 (4–5 notes) are tight | The CPU gate grows to cover four engines at once. Voice stealing gets a rule: the melody and bass are never stolen by the chords |
| C3 | Key AUTO "analyzes material" | Sketchpad's `keyFit` ranks the 24 major and minor keys **from chords**: how many fit, plus the first and last chord's root. **Nothing in Sketchpad ranks keys from single notes**, such as a bass riff. Rule 7 says theory follows Sketchpad | Write the note-based ranking (weighted by duration, first, last and lowest note, and beat) **in Sketchpad first**, then port it with the oracle, as for scales. See question Q1 |
| C4 | "C–F–B♯ is heard as C–F–C" | Right, and simpler still: the FM-1 sends pitches, not spellings, so B♯ never arises. The firmware spells notes from the key (D-005). C–F–C fits at least C major, F major, C minor, F minor, B♭ major, C mixolydian and C dorian | The list says **"ambiguous"** until one candidate clearly leads. The tonic evidence (first, last, longest and lowest note: C here) orders the rest |
| C5 | PAD: "left seven white keys are I–vii; the next seven may repeat" | On the FM-1 the left seven white keys are F3–E4 (it has 16 white keys, F3–G5). Pads by position are right when the key may be unknown (AUTO). But they lose what v0.1 gave: seeing the chord sit on its root | **Light the chord's actual notes on the right-hand keys** and let them strum (fm1-chord's strumplate). The eye learns the shape, and the hand can still play it. Use this rather than a second bank of repeated pads |
| C6 | K2 COLOR: triad, 7th, sus, add9; Guided A uses Am7 and D9 | The owner chose **triads first** (D-026) | Version one shows TRIAD only; the rest comes with the "sevenths" story. Guided A waits for it, or starts with Am and Dm |
| C7 | OCT+ / OCT− accept and back in dialogs | Felucca does the same, so that's fine. Two things to watch: **never both together** (5 s of both is the way back to update mode, HAZ-04, D-006). And in BASS the same buttons shift the register | Accept and back apply only on a lesson's instruction card. While playing, OCT shifts the register |
| C8 | SAVE persists the project | The store keeps **one record of 1 KB** (D-019, D-022). A four-track song of 4–8 bars is a few KB, and Guided progress needs room too | A bigger store region, decided from prior art (Felucca's projects, SLOOP's, ChoralRoot's loops), keeping "never erase what you didn't write" |
| C9 | Guided tutorials are "recipes" | They need a **format**, not code: steps, each with an instruction, a demo, the part to play, a hint level and a why | Tutorials as data, with a lesson gate that checks every step is reachable, every demo plays, and every line fits 240 px |
| C10 | Kids view "simple large notes/keys" | Our actor list says a child **may not read yet** | The kids view works without reading: the breathing key, big note names in colour, sounds, the horse. Text is for grown-ups |
| C11 | Game assesses timing | A key reaches the synth after up to 12 ms (velocity, D-017), plus one audio block (2.9 ms) | Timing windows allow for this, measured in the emulator. Timing is judged against the key press, not the sound |
| C12 | Scratch takes in Game | Writing flash pauses the CPU for tens of ms (D-019); the store already waits for quiet (D-023) | Takes stay in RAM; SAVE writes when quiet. Nothing in Game writes flash while it plays |
| C13 | Images | They are v0.1-era illustrations with labels that v0.2 supersedes (as the concept says) | The agreed concept (v0.3) gets mock-ups drawn from its own control map, regenerated by script as v0.1's are |

## v0.2's own questions (section 9), answered

| Question | Recommendation |
|---|---|
| ALGORITHM only selects the track, even on the song overview? | **Yes** |
| The provisional pad key when GLO is AUTO with no clear answer? | **The top candidate by default.** SELECT changes it, OCT+ locks the pad palette without forcing the song's key |
| The right-hand white keys in PAD: repeat the degrees or a separate bank? | **Neither** (C5): they light the chord's notes and strum them |
| Do voicing and colour changes affect a held chord? | **Live audition. Never rewrite recorded events** |
| Quick entry timing? | **One chord per bar first**; durations in SEQ later |

## New questions for the owner

| # | Question | Options | Recommendation |
|---|---|---|---|
| Q1 | Where is the note-based key analysis written? | **(a) In Sketchpad first, then ported** with the oracle: one source of truth, and Sketchpad gains it too. (b) Here, with its own tests: faster, but two places where theory lives | **(a)** |
| Q2 | The time signature in version one? | 4/4 only; or 3/4 and 6/8 too | **4/4 only** (GLO shows it, fixed). Three-four is for later songs |
| Q3 | The first slice | v0.2 proposes: record a bassline, infer harmonies, pick one, chords over it, then a challenge | **Yes, in this order of stories:** (1) four-track engine and recorder; (2) GLO song settings; (3) harmony assist; (4) chord PAD and MANUAL, triads; (5) Game on your own song, study view first, then the horse; Guided after that |
| Q4 | Which Game view comes first? | Study (piano roll) or kids (horse) | **Study first.** It's the same engine, and the simplest picture makes the timing and matching easy to get right. The horse follows right after, on the same engine |

## What happens next

When the owner agrees, or changes the answers above, the factory writes **concept v0.3** (v0.2 with these
settled, and new mock-ups). It then raises the first stories, with use cases and requirements, from v0.3's walk-throughs.
