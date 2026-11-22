#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Screen mock-ups for docs/concepts/v0.1/CONCEPT_v0.1.md: python3 docs/concepts/v0.1/make_mockups.py
Drawn at the FM-1's 240x240 and saved at 2x. These are proposals to discuss, not the firmware's pixels."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent / "img"
OUT.mkdir(exist_ok=True)
F = "/usr/share/fonts/opentype/inter/Inter-%s.otf"
S = 2                                   # draw at 2x


def font(px, w="SemiBold"):
    return ImageFont.truetype(F % w, px * S)


BG = (16, 18, 28)
PANEL = (30, 34, 50)
FG = (255, 255, 255)
MUTED = (140, 150, 175)
ACCENT = (255, 196, 64)                 # yellow: what is selected
CHORD = (255, 110, 90)                  # coral: the chord
SCALE = (70, 110, 200)                  # blue: the scale
GOOD = (90, 200, 120)
TRACK = {"Chords": CHORD, "Bass": (170, 120, 255), "Melody": (80, 190, 240), "Drums": (150, 150, 150)}
NAMES = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]


class Screen:
    def __init__(self, w=240, h=240):
        self.im = Image.new("RGB", (w * S, h * S), BG)
        self.d = ImageDraw.Draw(self.im)

    def rect(self, x, y, w, h, c, r=0, outline=None, width=1):
        box = [x * S, y * S, (x + w) * S - 1, (y + h) * S - 1]
        if r:
            self.d.rounded_rectangle(box, r * S, fill=c, outline=outline, width=width * S)
        else:
            self.d.rectangle(box, fill=c, outline=outline, width=width * S)

    def text(self, x, y, s, px=12, c=FG, w="SemiBold", anchor="la"):
        self.d.text((x * S, y * S), s, font=font(px, w), fill=c, anchor=anchor)

    def poly(self, pts, c):
        self.d.polygon([(x * S, y * S) for x, y in pts], fill=c)

    def ellipse(self, x, y, w, h, c):
        self.d.ellipse([x * S, y * S, (x + w) * S, (y + h) * S], fill=c)

    def line(self, pts, c, width=1):
        self.d.line([(x * S, y * S) for x, y in pts], fill=c, width=width * S)

    def header(self, left, mode, right=""):
        self.rect(0, 0, 240, 22, PANEL)
        self.text(6, 4, left, 12, FG)
        mw = self.d.textlength(mode, font=font(11, "Bold")) / S + 12
        self.rect(240 - mw - 6 - (34 if right else 0), 3, mw, 16, ACCENT, r=4)
        self.text(240 - mw / 2 - 6 - (34 if right else 0), 5, mode, 11, BG, "Bold", "ma")
        if right:
            self.text(234, 4, right, 12, MUTED, anchor="ra")

    def legend(self, items):
        """the bottom line: what each knob or button does here"""
        self.rect(0, 222, 240, 18, PANEL)
        x = 4
        for k, v in items:
            kw = self.d.textlength(k, font=font(9, "Bold")) / S + 6
            self.rect(x, 225, kw, 12, (70, 76, 100), r=3)
            self.text(x + kw / 2, 226, k, 9, FG, "Bold", "ma")
            x += kw + 2
            self.text(x, 226, v, 9, MUTED, "Medium")
            x += self.d.textlength(v, font=font(9, "Medium")) / S + 6

    def piano(self, x, y, w, h, levels, labels=None):
        """F3..G5 (the FM-1's 27 keys); levels: {midi: 'dim'|'on'|'breath'|'chord'|'held'}"""
        whites = [m for m in range(53, 80) if NAMES[m % 12] in ("C", "D", "E", "F", "G", "A", "B")]
        ww = w / len(whites)
        col = {"dim": SCALE, "on": FG, "breath": (120, 170, 255), "chord": CHORD, "held": ACCENT, "root": (255, 160, 140)}
        for i, m in enumerate(whites):
            c = col.get(levels.get(m), (200, 204, 215))
            self.rect(x + i * ww, y, ww - 1, h, c, r=1)
            if labels and m in labels:
                self.text(x + i * ww + ww / 2, y + h - 11, labels[m], 7, BG, "Bold", "ma")
        for i, m in enumerate(whites):
            if NAMES[m % 12] in ("E", "B") or m + 1 > 79:
                continue
            b = m + 1
            c = col.get(levels.get(b), (40, 44, 58))
            self.rect(x + i * ww + ww * 0.62, y, ww * 0.72, h * 0.6, c, r=1)

    def save(self, name):
        self.im.save(OUT / name)


def chord_notes(root, minor=False, dim=False):
    return [root, root + (3 if minor or dim else 4), root + (6 if dim else 7)]


# ---------------------------------------------------------------- MAKE: key and scale (SCL)
s = Screen()
s.header("Key", "MAKE", "90")
s.text(120, 30, "C major", 26, ACCENT, "Bold", "ma")
s.text(120, 62, "Hold SCL and press a key to set the root", 9, MUTED, "Medium", "ma")
scales = [("Major", True), ("Natural minor", False), ("Pentatonic", False), ("Blues", False), ("Dorian", False)]
for i, (n, sel) in enumerate(scales):
    y = 80 + i * 20
    s.rect(14, y, 212, 18, (60, 50, 20) if sel else PANEL, r=4)
    s.text(22, y + 3, n, 11, ACCENT if sel else FG)
s.piano(8, 186, 224, 32, {m: ("root" if m % 12 == 0 else "dim") for m in range(53, 80) if m % 12 in (0, 2, 4, 5, 7, 9, 11)})
s.legend([("K1", "root"), ("K2", "scale"), ("SCL", "back")])
s.save("make_key.png")

# ---------------------------------------------------------------- MAKE: chords, manual (pick and read)
s = Screen()
s.header("C major", "CHORDS", "90")
deg = [("I", "C"), ("ii", "Dm"), ("iii", "Em"), ("IV", "F"), ("V", "G"), ("vi", "Am"), ("vii°", "Bdim")]
for i, (r, n) in enumerate(deg):
    x = 4 + i * 33
    sel = i == 5
    s.rect(x, 28, 31, 40, CHORD if sel else PANEL, r=5)
    s.text(x + 15.5, 32, r, 9, BG if sel else MUTED, "Bold", "ma")
    s.text(x + 15.5, 46, n, 11 if len(n) < 4 else 9, BG if sel else FG, "Bold", "ma")
s.text(120, 76, "Am", 40, CHORD, "Bold", "ma")
s.text(120, 122, "vi  ·  A  C  E  ·  root position", 10, MUTED, "Medium", "ma")
s.text(120, 138, "Play the lit keys", 11, FG, "SemiBold", "ma")
lv = {m: "dim" for m in range(53, 80) if m % 12 in (0, 2, 4, 5, 7, 9, 11)}
for m in (57, 60, 64):
    lv[m] = "chord"
s.piano(8, 156, 224, 44, lv)
for i in range(4):                       # the progression so far
    s.rect(8 + i * 57, 204, 54, 14, TRACK["Chords"] if i < 3 else PANEL, r=3)
    s.text(8 + i * 57 + 27, 205, ["C", "G", "Am", "+"][i], 9, BG if i < 3 else MUTED, "Bold", "ma")
s.legend([("K1", "chord"), ("K2", "voicing"), ("REC", "add"), ("EDIT", "1-key")])
s.save("make_chords_manual.png")

# ---------------------------------------------------------------- MAKE: chords, one key plays a chord
s = Screen()
s.header("C major", "1-KEY", "90")
s.text(120, 30, "G", 52, CHORD, "Bold", "ma")
s.text(120, 92, "V  ·  G  B  D", 11, MUTED, "Medium", "ma")
s.text(120, 112, "Each scale key plays its own chord", 10, FG, "Medium", "ma")
labels = {}
for m in range(53, 80):
    pc = m % 12
    if pc in (0, 2, 4, 5, 7, 9, 11):
        labels[m] = {0: "I", 2: "ii", 4: "iii", 5: "IV", 7: "V", 9: "vi", 11: "vii"}[pc]
lv = {m: "dim" for m in labels}
lv[67] = "held"
for m in (71, 74):
    lv[m] = "chord"
s.piano(8, 132, 224, 70, lv, labels)
s.text(120, 206, "Out-of-scale keys: soft (GLO to change)", 8, MUTED, "Medium", "ma")
s.legend([("K2", "inversion"), ("K3", "strum"), ("ARP", "arp"), ("EDIT", "manual")])
s.save("make_chord_keys.png")

# ---------------------------------------------------------------- MAKE: scales that fit what you made
s = Screen()
s.header("C  G  Am  F", "FITS", "90")
s.text(10, 28, "Scales that fit your chords", 12, FG, "Bold")
fits = [("C major", 3, "every note fits"), ("A minor", 3, "same notes, sad home"), ("C major pentatonic", 3, "no wrong notes"),
        ("C mixolydian", 2, "one note clashes: B"), ("A blues", 2, "bluesy, two clash")]
for i, (n, st, why) in enumerate(fits):
    y = 48 + i * 30
    sel = i == 2
    s.rect(8, y, 224, 27, (60, 50, 20) if sel else PANEL, r=5)
    s.text(16, y + 3, n, 11, ACCENT if sel else FG, "Bold")
    s.text(16, y + 15, why, 8, MUTED, "Medium")
    s.text(224, y + 6, "★" * st + "☆" * (3 - st), 11, ACCENT, "Regular", "ra")
s.piano(8, 200, 224, 20, {m: "dim" for m in range(53, 80) if m % 12 in (0, 2, 4, 7, 9)})
s.legend([("K1", "pick"), ("PLAY", "hear it"), ("SCL", "use it")])
s.save("make_fits.png")

# ---------------------------------------------------------------- MAKE: play a melody over the loop
s = Screen()
s.header("C major pent.", "MELODY", "90")
s.text(10, 28, "Now: Am", 14, CHORD, "Bold")
s.text(230, 28, "next: F", 12, MUTED, "SemiBold", anchor="ra")
s.rect(10, 50, 220, 6, PANEL, r=3)
s.rect(10, 50, 140, 6, ACCENT, r=3)
s.text(120, 66, "Blue keys fit. Coral keys are in the chord:", 9, FG, "Medium", "ma")
s.text(120, 79, "they sound the most at home.", 9, FG, "Medium", "ma")
lv = {m: "dim" for m in range(53, 80) if m % 12 in (0, 2, 4, 7, 9)}
for m in (57, 60, 64, 69, 72, 76):
    lv[m] = "chord"
lv[69] = "held"
s.piano(8, 98, 224, 60, lv)
for i, (n, x0, x1) in enumerate([("Chords", 0, 1), ("Bass", 0, 1), ("Melody", 0, .62), ("Drums", 0, 1)]):
    y = 166 + i * 13
    s.text(10, y, n, 9, TRACK[n], "Bold")
    s.rect(60, y + 2, 170, 8, PANEL, r=2)
    s.rect(60, y + 2, 170 * x1, 8, TRACK[n], r=2)
s.legend([("REC", "record"), ("SEQ", "track"), ("K1", "fits")])
s.save("make_melody.png")

# ---------------------------------------------------------------- MAKE: the four tracks (SEQ)
s = Screen()
s.header("My song 1", "TRACKS", "90")
bars = ["C", "G", "Am", "F"]
for t, (n, sound) in enumerate([("Chords", "Piano"), ("Bass", "Analog bass"), ("Melody", "Bell"), ("Drums", "Hip hop")]):
    y = 30 + t * 44
    sel = t == 1
    s.rect(4, y, 232, 40, (40, 38, 60) if sel else PANEL, r=5, outline=ACCENT if sel else None, width=1)
    s.text(10, y + 4, n, 11, TRACK[n], "Bold")
    s.text(10, y + 20, sound, 8, MUTED, "Medium")
    for b in range(4):
        x = 82 + b * 38
        if n == "Chords":
            s.rect(x, y + 8, 35, 24, TRACK[n], r=3)
            s.text(x + 17.5, y + 13, bars[b], 10, BG, "Bold", "ma")
        elif n == "Drums":
            for k in range(8):
                s.rect(x + 2 + k * 4, y + (10 if k % 2 else 22), 3, 6, TRACK[n])
        else:
            import random
            random.seed(t * 10 + b)
            for k in range(4 if n == "Bass" else 6):
                h = random.randint(0, 16)
                s.rect(x + 2 + k * (8 if n == "Bass" else 5.5), y + 26 - h, 5 if n == "Bass" else 4, 4, TRACK[n])
s.rect(82 + 1.4 * 38, 28, 2, 178, ACCENT)
s.legend([("SEQ", "track"), ("PRESETS", "sound"), ("SAVE", "keep")])
s.save("make_tracks.png")

# ---------------------------------------------------------------- GAME: kids view (the horse)
s = Screen()
SKY, GRASS = (120, 190, 240), (90, 170, 80)
s.rect(0, 0, 240, 240, SKY)
s.ellipse(180, 12, 34, 34, (255, 225, 110))                       # the sun
for cx in (30, 120):
    s.ellipse(cx, 28, 46, 18, (240, 248, 255))
    s.ellipse(cx + 14, 20, 30, 20, (240, 248, 255))
s.rect(0, 150, 240, 90, GRASS)
s.rect(0, 150, 240, 4, (70, 140, 60))
# the horse (our own drawing): body, neck, head, legs mid-gallop, mane and tail
BROWN, DARK = (150, 95, 55), (90, 55, 30)
s.ellipse(34, 106, 56, 28, BROWN)                                  # body
s.poly([(78, 112), (90, 84), (100, 88), (90, 118)], BROWN)        # neck
s.ellipse(88, 76, 26, 16, BROWN)                                   # head
s.poly([(92, 78), (95, 68), (99, 78)], BROWN)                      # ear
s.ellipse(105, 80, 4, 4, (30, 20, 10))                             # eye
s.poly([(80, 86), (92, 74), (88, 100), (80, 112)], DARK)           # mane
s.poly([(36, 112), (18, 118), (22, 132), (38, 120)], DARK)         # tail
for (x0, y0, x1, y1) in ((44, 128, 36, 150), (54, 130, 60, 150), (74, 128, 84, 148), (82, 126, 92, 140)):
    s.line([(x0, y0), (x1, y1)], DARK, 5)
# fences, each a note of the song
for i, (x, n) in enumerate([(130, "E"), (175, "D"), (220, "C")]):
    nxt = i == 0
    s.rect(x, 124, 4, 30, (255, 255, 255))
    s.rect(x + 22, 124, 4, 30, (255, 255, 255))
    s.rect(x - 2, 128, 30, 5, (230, 80, 70) if nxt else (240, 240, 240))
    s.rect(x - 2, 140, 30, 5, (230, 80, 70) if nxt else (240, 240, 240))
    s.ellipse(x + 1, 94, 24, 24, ACCENT if nxt else (255, 255, 255))
    s.text(x + 13, 98, n, 13, BG, "Bold", "ma")
s.rect(0, 0, 240, 22, (60, 120, 170))
s.text(6, 4, "Twinkle Twinkle", 12, FG, "Bold")
s.text(234, 4, "★★☆", 12, ACCENT, "Regular", "ra")
s.text(120, 166, "Play  E  to jump!", 16, FG, "Bold", "ma")
lv = {m: None for m in range(53, 80)}
lv = {64: "held"}
s.piano(8, 192, 224, 28, lv)
s.save("game_kids.png")

# ---------------------------------------------------------------- GAME: grown-up view (learn the part)
s = Screen()
s.header("Funk bass étude", "LEARN", "75%")
# a piano roll: notes come from the right to the line at x=60; the keyboard down the left
lo, hi = 53, 72
rows = hi - lo
rh = 168 / rows
for i, m in enumerate(range(hi - 1, lo - 1, -1)):
    y = 26 + i * rh
    black = NAMES[m % 12] not in ("C", "D", "E", "F", "G", "A", "B")
    s.rect(4, y, 30 if not black else 20, rh - 0.5, (40, 44, 58) if black else (200, 204, 215))
    if m == 57:
        s.rect(4, y, 30, rh - 0.5, ACCENT)
    if NAMES[m % 12] == "C":
        s.text(36, y - 1, "C%d" % (m // 12 - 1), 7, MUTED, "Medium")
s.rect(60, 26, 2, 168, ACCENT)
notes = [(57, 62, 16), (57, 82, 8), (60, 96, 8), (62, 110, 16), (64, 132, 8), (62, 146, 8), (57, 160, 24), (55, 190, 8), (57, 204, 16)]
for m, x, w in notes:
    y = 26 + (hi - 1 - m) * rh
    s.rect(x, y, w, rh - 1, TRACK["Bass"], r=2)
s.text(150, 200, "A  A  C  D  E  D  A  G  A", 8, MUTED, "Medium", "ma")
s.rect(150, 210, 80, 8, PANEL, r=2)
s.rect(150, 210, 58, 8, GOOD, r=2)
s.text(146, 209, "in time 72%", 8, FG, "Medium", "ra")
s.legend([("K1", "part"), ("K2", "speed"), ("K3/4", "loop"), ("EDIT", "wait")])
s.save("game_learn.png")

# ---------------------------------------------------------------- GAME: choose a song
s = Screen()
s.header("Songs", "GAME")
songs = [("Twinkle Twinkle", "Kids", "C major", 1), ("Frère Jacques", "Kids", "F major", 1),
         ("Ode to Joy", "Classic", "G major", 2), ("Funk bass étude 1", "Funk", "A minor", 2),
         ("West coast étude", "G-funk", "D minor", 3), ("My song 1", "Made by you", "C major", 2)]
for i, (n, st, key, lvl) in enumerate(songs):
    y = 28 + i * 31
    sel = i == 0
    s.rect(6, y, 228, 28, (60, 50, 20) if sel else PANEL, r=5)
    s.text(14, y + 3, n, 11, ACCENT if sel else FG, "Bold")
    s.text(14, y + 16, "%s  ·  %s" % (st, key), 8, MUTED, "Medium")
    s.text(226, y + 7, "●" * lvl + "○" * (3 - lvl), 10, ACCENT, "Regular", "ra")
s.legend([("K1", "song"), ("K2", "part"), ("GLO", "horse/dog"), ("PLAY", "go")])
s.save("game_songs.png")

# ---------------------------------------------------------------- the panel: what every control does
W, H = 900, 470
im = Image.new("RGB", (W * S, H * S), (24, 26, 36))
d = ImageDraw.Draw(im)


def T(x, y, s_, px=12, c=FG, w="SemiBold", anchor="la"):
    d.text((x * S, y * S), s_, font=font(px, w), fill=c, anchor=anchor)


def R(x, y, w, h, c, r=6, outline=None):
    d.rounded_rectangle([x * S, y * S, (x + w) * S, (y + h) * S], r * S, fill=c, outline=outline, width=2 * S)


def knob(x, y, name, what, c=ACCENT):
    d.ellipse([(x - 16) * S, (y - 16) * S, (x + 16) * S, (y + 16) * S], fill=(70, 74, 90), outline=(120, 124, 140), width=2 * S)
    T(x, y + 20, name, 9, MUTED, "Bold", "ma")
    T(x, y + 32, what, 10, c, "SemiBold", "ma")


def button(x, y, name, what, c=ACCENT):
    R(x, y, 56, 24, (60, 64, 80), 4)
    T(x + 28, y + 6, name, 9, FG, "Bold", "ma")
    T(x + 28, y + 28, what, 9, c, "SemiBold", "ma")


R(10, 10, W - 20, H - 20, (44, 46, 58), 14)
knob(70, 60, "MASTER", "volume", MUTED)
knob(150, 60, "SELECT", "tempo")
knob(70, 140, "PRESETS", "track's sound")
knob(150, 140, "ALGORITHM", "mode")
R(205, 30, 170, 170, BG, 8)
T(290, 100, "screen", 12, MUTED, "Medium", "ma")
T(290, 116, "the bottom line says", 9, MUTED, "Medium", "ma")
T(290, 128, "what K1-K4 do here", 9, MUTED, "Medium", "ma")
for i in range(4):
    knob(430 + i * 110, 60, "KNOB%d" % (i + 1), ["chord / song", "voicing / part", "strum / loop", "length / loop"][i])
row1 = [("FX", "effects"), ("SCL", "key & scale"), ("ENV", "sound shape"), ("LFO", "wobble"), ("EDIT", "1-key / manual"), ("GLO", "settings")]
row2 = [("HOME", "mode home"), ("SAVE", "keep song"), ("ARP", "arpeggio"), ("SEQ", "track"), ("PLAY", "play / stop"), ("REC", "record / add")]
for i, (n, w) in enumerate(row1):
    button(420 + i * 74, 130, n, w, MUTED if n in ("FX", "ENV", "LFO") else ACCENT)
for i, (n, w) in enumerate(row2):
    button(420 + i * 74, 190, n, w, GOOD if n in ("PLAY", "REC") else ACCENT)
R(40, 200, 60, 22, (60, 64, 80), 4)
T(70, 205, "OCT- OCT+", 9, FG, "Bold", "ma")
T(70, 226, "octave", 9, ACCENT, "SemiBold", "ma")
# the keys
whites = 16
kx, ky, kw = 30, 270, (W - 60) / whites
for i in range(whites):
    R(kx + i * kw, ky + 50, kw - 4, 90, (210, 212, 220), 4)
for i in range(whites - 1):
    if i % 7 in (3, 6):          # F G A B C D E: no black key after B or E
        continue
    R(kx + i * kw + kw * 0.6, ky, kw * 0.7, 46, (70, 72, 84), 4)
T(W / 2, ky + 150, "KEYS: notes, or one chord each (1-KEY)  ·  hold SCL + key: set the root  ·  lit keys: the scale, the chord, the next note",
  10, FG, "Medium", "ma")
im.save(OUT / "panel_map.png")
print("wrote", sorted(p.name for p in OUT.glob("*.png")))
