# Your own kit

Put your sounds here and build the firmware: they become the fifth kit, **USER** (PRESETS on a DRUMS track).

- Name each file `NN-name.wav`: `NN` is the key, `01` for F3 (the lowest) to `27` for G5; `name` is what the
  screen shows (up to 10 letters). `01-kick1.wav`, `05-snare1.wav`.
- WAV, PCM, 8, 16 or 24 bits, mono or stereo, any rate: the build makes each mono, 22 050 Hz, 16 bits, a second at
  most; 240 KB of sound in all.
- Put a `LICENSE` file (or `LICENSE.txt`, `LICENSE.md`) here saying whose the sounds are and on what terms they may
  be used. Without one the build stops: the firmware only carries sounds it may carry.
- A key without a file plays the Dusty kit's sound for that key.

D-056 in `docs/PLATFORM.md`; `factory/userkit.py` checks it.
