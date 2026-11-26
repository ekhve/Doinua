# Your own kit

Put your sounds here and build the firmware: they become the fifth kit, **USER** (PRESETS on a DRUMS track).

- Name each file `NN-name.wav`: `NN` is the key, `01` for F3 (the lowest) to `27` for G5; `name` is what the
  screen shows (up to 10 letters). `01-kick1.wav`, `03-snare1.wav`. The drum map (D-121): `01` kick, `02` hat, `03` snare,
  `04` open hat, `05` clap, `06` shaker, `07` rim, `08` tom 1, `09` ghost, `10` tom 2, `11` perc, `12` tom 3 (F3 to E4,
  the keys AUTO and IDEAS play); `13` to `27` free.
- WAV, PCM, 8, 16 or 24 bits, mono or stereo, any rate: the build makes each mono, 22 050 Hz, 16 bits, a second at
  most; 240 KB of sound in all.
- Put a `LICENSE` file (or `LICENSE.txt`, `LICENSE.md`) here saying whose the sounds are and on what terms they may
  be used. Without one the build stops: the firmware only carries sounds it may carry.
- A key without a file plays the Dusty kit's sound for that key.

D-056 in `docs/PLATFORM.md`; `factory/userkit.py` checks it.
