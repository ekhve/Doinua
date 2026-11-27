# Kursaal Pulse: the visualizer's artistic target (from the owner, 2026-11-27)

`kursaal-pulse-target.gif` is the owner's sketch: the Kursaal seen from the María Cristina bridge in Donostia at
night, its facade a moving light show, the river mirroring it, the bridge lamps glowing. It is the **artistic target,
not the implementation**: the firmware adapts its visual language in a few cheap layers. A lab comes first (the lab is
the contract for the screen, D-084); this file holds the brief the owner sent with the sketch.

## What to keep

- **The Kursaal is the star**: a projection screen, a glowing instrument, a living canvas. Not on/off pulses but
  moving bands, blooming colour fields, curved wave sweeps, an occasional geometric rise and fall, rare motif reveals.
- **The reflections are part of the show**: the water mirrors the facade's colours, smeared vertically, wobbling a
  little, brighter with the music's intensity.
- **The lamp posts pulse warmly**: steady most of the time, blooming softly on the kick, never flashing hard.
- **The sky hosts small motifs**: sparse dots, tiny constellations, little wave forms, short streaks.
- What makes it beautiful is **expressive light movement**: large readable shapes, clear colour transitions, flow,
  contrast against the dark night. That matters more than detail.

## How to build it (layers)

- **Static background**: the sky painting, the bridge, the buildings' silhouettes, the Kursaal's shape, the espigón,
  the lamp posts' bodies.
- **Animated overlays**: the facade's light mask and patterns, the water's reflection strips, the lamp globes'
  brightness, the sky particles, a few windows.

## Facade modes, first

1. Warm watercolour bloom: gold, amber and violet clouds moving slowly.
2. Blue-cyan wave sweep: a curved band crossing the facade.
3. Ribbon (infinity-like path): melodic movement.
4. Vertical rise bars: energetic parts.
5. Soft abstract mix: a quiet idle state.

## The music, mapped

| Part | What reacts |
|---|---|
| Kick | the lamp posts bloom, a low pulse on the Kursaal, a small shake in the reflections |
| Bass | the water thickens and vibrates, the facade's glow deepens, the low bands move more |
| Chords | the palette and mood: warm (amber, peach, gold) or cool (cyan, blue, violet); minor darker, major more open |
| Melody | ribbons, sky dots, windows: directional movement |
| Live playing | the strongest sweeps, motifs, special facade moments |

## Stages

- **V1**: the facade modes, the water reflections, the lamp pulses.
- **V2**: sky dots and small motifs, windows reacting to the melody, foam behind the waves.
- **V3**: harmony-driven palettes, phrase-level events, live-performance flourishes.

## FM-1 notes (the factory lead)

- The screen is drawn in the main loop, not the audio interrupt: the visualizer does not take from the 80 % audio
  budget, but a heavy frame makes the screen less smooth; each layer is measured.
- A 240x240 painted background costs roughly 15–40 KB of flash even compressed; the app has about 30 KB left (493 of
  523 KB, the 90 % gate of `safety.py`). The visualizer will need the owner's flash decision (the gate at 95 %, or
  images in a data area as Felucca keeps samples), with measured numbers.
- Order (the owner, 2026-11-27, D-126): the first flashable build, the owner's test and a first release come first; the
  guided mode next; this visualizer and the InfeKt-FM1 audition after.
