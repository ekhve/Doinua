# User stories — concept v0.5 (draft for the owner)

As a … I want … so that … Each story lists what it covers and its acceptance (the usage scenarios U-n and test scenarios T-n in `SCENARIOS.md`, and the
draft requirements CV-R* in `CONCEPT_v0.5.md` §8, or ones still to write). **Built**: already in the firmware.

| # | Story | Covers | Accepted by | State |
|---|---|---|---|---|
| ST-01 | As a **producer**, I want to start a song on any track (chords, bass, melody or drums), so that I begin with whatever idea I have. | start anywhere | U-1, U-2, U-3 · T-1, T-2, T-3 | partly: chords built; bass, melody, drums are plain keyboards |
| ST-02 | As a producer, I want the key to come from what I record first, or to lock it myself, so that the other tracks help me in the right key. | key AUTO / locked | U-1, U-4 · T-1, T-2, T-4 · CV-R1, CV-R2, CV-R5 | AUTO built (read when the song stops); GLO page to build |
| ST-03 | As a producer, I want to choose how much help each track gives me (free, in key, guide, auto), so that I play freely or let the FM-1 carry me. | the ladder | U-1, U-3, U-7 · T-1 to T-3 | chords built (manual, pad); bass, melody to design |
| ST-04 | As a producer, I want to record each track over the loop as I play it, and go back to any track, so that the song grows in any order. | sequencer, back and forth | U-4 · T-4 · CV-R3 | built (REC, PLAY, four tracks) |
| ST-05 | As a producer, I want to see on every page the song's key, tempo and what is recorded where, so that I always know where I am. | top line, loop line | all | built (D-037) |
| ST-06 | As anyone, I want a song page for the key, tempo, length and time signature, so that the song's settings are in one place and never change what I played. | GLO | U-4 · T-4 · R-096 to R-098 | built (#44) |
| ST-07 | As anyone, I want to choose between Producer, Guided and Game, so that the FM-1 fits what I want to do today. | HOME | U-5, U-6 · T-5 · R-099 to R-103 | built (#44; Guided and Game as placeholders) |
| ST-08 | As a **beginner** (an older child or a grown-up), I want the FM-1 to lead me step by step through making a song, from very easy to a bit harder, so that I learn producing without knowing where to start. | Guided | U-5 · T-6, T-7 | later |
| ST-09 | As a **child**, I want to play a melody with a unicorn or a dinosaur that jumps when I hit the right key, so that practising feels like a game. | Game | U-6 · T-8 | later |
| ST-10 | As a producer, I want chords that fit a bassline or melody I already recorded, so that I can harmonise it. | Harmony Assist | U-2 · T-2 step 4 | later (v0.2) |
| ST-11 | As a producer, I want bass help like the chords' (in key, the chord now, a style's ideas, one-finger basslines), so that my basslines groove and fit. | bass ladder | U-1, U-3 · T-1, T-3 | next (to explore) |
