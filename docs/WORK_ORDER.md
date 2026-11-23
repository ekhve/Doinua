# Work order: Monday 23 to Thursday 26 November 2026

From the factory lead (the agent so far) to the agent covering until Thursday, agreed with the owner. Read
`docs/ONBOARDING.md` first: it has the loop, the gates and the recipes this order refers to.

## The goal for the week

More of the owner's 208 picked sounds playable on the FM-1, with their effects and arpeggios, without breaking the CPU
(< 80 %) or the flash (< 523,407 B) gates. The owner tests on a real FM-1 at the end of the week.

## Tasks you may do on your own (merge when check.sh and CI are green)

Take them in this order. Each is one story, one branch, one PR. Update issue #1 when one merges.

| # | Task | Source | Notes |
|---|---|---|---|
| 1 | **Bass Something 2 engine**: the owner's 11 picks (new story **ST-92**) | https://github.com/imijes/bass-something-2 (4c8a82f), MPL-2.0, code and patches the author's own | A monosynth for bass: one voice a track. Recipe 4a. Picks: `assets/sounds/browser.json` entries with engine "Bass Something 2: …". Its effects: the patches' own sends where it has them |
| 2 | **OberVave engine**: 19 picks (**ST-72**) | https://github.com/zvenson/obervave (a1f660c), GPL-3.0 | Its programs include arpeggios: play them through our ARP (D-088), as task 4 does for FMini |
| 3 | **VA engine**: 9 picks (**ST-73**) | CoralFlower https://github.com/FicusFicusov/CoralFlower (46790b9), GPL-3.0 (ChoralRoot's VA) | Chord pads and basses: measure voices and CPU first |
| 4 | **FMini arpeggio patches through our ARP** (part of ST-77) | `fmini_x_t.arp[7]` in `fm1kit/src/fmini/` (mode, rate, octaves, gate, swing, probability) | When an FMini patch with its arp on is chosen, the track's ARP takes its settings and turns on, as fmini-1 does. The owner asked for exactly this ("implement the arp in our arp") |
| 5 | **K-WASUB audition: record** (owner asked) | https://github.com/Guizmoh/K-WASUB_Firmware_for_FM1, GPL-3.0; tools in `tools/audition/` (`docs/HANDOFF.md` has the commands) | Record all 121 presets, one clip each, check every name on the screen grabs, make one page (a clip per sound, a tick box each, the owner's picks exported as a list). **How the owner opens the page: ask** (GitHub Pages, or a file). Do not port K-WASUB until the owner has picked |
| 6 | **Softer game sounds** (the owner: unicorn and train "a bit too strident"; Hiru damatxo is good) | `apps/companion/` games | Lower their level and round their tone (low-pass, softer waves). Keep their tunes. Put a before/after recording in the PR |

For each engine: measure CPU (section 3 of ONBOARDING) **before** the PR. If the gate fails, cap the engine's voices
or make the code cheaper. **Never raise the 80 % limit**: that is the owner's call. Report the flash used after each
engine; if the image passes ~500 KB, stop adding engines and tell the owner.

## Tasks that need the owner first (prepare, then ask; do not merge without the answer)

| Task | What to ask |
|---|---|
| **Prophet engine** (27 picks, Melodee, GPL-3.0 code) | The patches are Sequential's factory sounds copied by Melodee: may we ship them? (The owner allowed Roland's Juno patches; ask the same for Sequential.) |
| **Sound editing and FX pages** (`docs/labs/sound-edit-lab.html`) | You may build them as the lab draws them (recipe 4b), but **the screens need the owner's approval** before merge: send side-by-side images (lab vs firmware) and ask them to run "Approve screens" |
| **Wet or dry per sound** | If a sound with its preset's effects sounds wrong in a test, ask before making it dry |
| Any change to the gates, the safety parts, the flash layout, a dependency pin | Ask |

## Not this week (owner's open questions; do not start)

Several songs (needs a lab first), the sampler (four open questions in its lab), a sampled piano (40–80 KB, owner to
decide), drum kits (wait for the owner's samples), the start-up logo (the owner checks it), the visual "just play" mode
(the owner will send a sketch of the Kursaal from the María Cristina bridge), K-WASUB port (after the picks).

## Reporting

- **Daily:** one comment on issue #1: what merged (PR links), what is next, any question (2–3 options and your
  recommendation). Nothing else unless you are blocked.
- **Thursday:** a summary for the owner's FM-1 test: what changed since 8e86f7c, which sounds to try, the latest
  `companion.fwsc` (the CI run's artifact link), and what you need from them.
- If CI or anything else blocks you, say so in one line with the cause and what you need.
