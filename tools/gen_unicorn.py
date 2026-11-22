#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The game's unicorn (D-059): drawn from shapes at 44 x 42 pixels, hard-edged, then outlined, so it reads as pixel
art at 2x on the FM-1. Five frames: two running, the jump (legs tucked, wings up), two flying (wings up, wings down).
  tools/gen_unicorn.py OUT_DIR       writes uni_a.png uni_b.png uni_up.png uni_fly1.png uni_fly2.png uni_sleep.png, and the flight's
                                     frames smaller and smaller as it flies off to the moon: uf<width>_<1|2><r|l>.png
                                     (r facing right, l left), and uni_fly1_l.png uni_fly2_l.png"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

W, H = 44, 42                                                # (drawn 4 rows down: the horn fits)
WHITE, SHADE, LAV = (255, 255, 255, 255), (214, 222, 248, 255), (178, 190, 236, 255)
GOLD, ORANGE, PINK = (255, 210, 63, 255), (242, 157, 56, 255), (255, 214, 120, 255)
WING, WING2, WING3 = (236, 242, 255, 255), (196, 212, 250, 255), (150, 172, 236, 255)
EYE, CHEEK, HOOF, OUT = (40, 22, 70, 255), (255, 158, 199, 255), (255, 200, 80, 255), (27, 34, 87, 255)


def leg(d, x, y, kx, ky, fx, fy, w=3):
    """a leg from the body (x, y) to the knee and the hoof"""
    d.line([(x, y), (kx, ky)], fill=WHITE, width=w)
    d.line([(kx, ky), (fx, fy)], fill=WHITE, width=w)
    d.rectangle([fx - 1, fy, fx + 1, fy + 1], fill=HOOF)


def wing(d, up):
    """feathers fanning from the shoulder: up (a jump, a beat) or along the back"""
    if up:
        rows = [((19, 15), (9, 2)), ((20, 15), (13, 0)), ((21, 15), (17, 1)), ((22, 15), (21, 4))]
    else:
        rows = [((19, 16), (5, 12)), ((20, 16), (6, 9)), ((21, 15), (8, 6)), ((22, 15), (11, 4))]
    for i, (a, b) in enumerate(rows):
        d.line([a, b], fill=[WING, WING2, WING, WING2][i], width=3)
    d.ellipse([16, 12, 24, 18], fill=WING)
    for a, b in rows:
        d.point(b, fill=WING3)


class Down:
    """ImageDraw, moved 4 rows down"""
    def __init__(self, d): self.d = d
    def __getattr__(self, name):
        f = getattr(self.d, name)
        def moved(xy, *a, **k):
            if isinstance(xy, tuple) and len(xy) == 2 and not isinstance(xy[0], tuple):
                return f((xy[0], xy[1] + 4), *a, **k)
            if isinstance(xy[0], tuple):
                return f([(x, y + 4) for x, y in xy], *a, **k)
            return f([v + 4 if i % 2 else v for i, v in enumerate(xy)], *a, **k)
        return moved


def frame(pose):
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = Down(ImageDraw.Draw(im))
    up = pose in ("up", "fly1")
    if pose not in ("up", "fly1", "fly2"):     # (asleep too: the wing folded)
        wing(d, False)
    # the tail, flowing behind
    d.polygon([(10, 17), (4, 18), (1, 24), (3, 30), (6, 25), (8, 21)], fill=GOLD)
    d.line([(9, 18), (3, 24), (2, 29)], fill=ORANGE, width=1)
    # legs: running out, running in, tucked
    if pose == "a":
        leg(d, 13, 23, 9, 28, 6, 33); leg(d, 16, 24, 17, 29, 20, 33)
        leg(d, 27, 24, 31, 28, 35, 31); leg(d, 30, 23, 33, 28, 32, 33)
    elif pose == "b":
        leg(d, 13, 23, 13, 28, 12, 33); leg(d, 16, 24, 15, 29, 16, 33)
        leg(d, 27, 24, 28, 29, 27, 33); leg(d, 30, 23, 31, 28, 30, 33)
    else:
        leg(d, 13, 23, 10, 26, 13, 28); leg(d, 16, 24, 14, 27, 17, 29)
        leg(d, 27, 24, 31, 25, 29, 28); leg(d, 30, 23, 34, 24, 33, 27)
    # body, a lavender belly
    d.ellipse([9, 13, 33, 26], fill=WHITE)
    d.chord([10, 17, 32, 26], 20, 160, fill=SHADE)
    d.chord([12, 21, 30, 26], 30, 150, fill=LAV)
    # neck and head
    d.polygon([(26, 16), (29, 6), (33, 6), (32, 18)], fill=WHITE)
    d.ellipse([28, 3, 38, 11], fill=WHITE)
    d.ellipse([33, 6, 41, 12], fill=WHITE)
    d.point((40, 9), fill=SHADE)
    d.polygon([(30, 4), (31, 1), (33, 4)], fill=WHITE)            # the ear
    d.line([(33, 4), (37, -3)], fill=GOLD, width=2)              # the horn
    d.point((36, -2), fill=PINK); d.point((35, 0), fill=ORANGE)
    if pose == "sleep":                         # the eye closed
        d.line([(34, 7), (36, 7)], fill=EYE)
    else:
        d.rectangle([35, 6, 36, 7], fill=EYE); d.point((35, 6), fill=WHITE)
    d.point((37, 9), fill=CHEEK); d.point((38, 9), fill=CHEEK)
    # the mane, down the neck
    d.polygon([(30, 3), (27, 5), (24, 10), (23, 15), (26, 14), (27, 10), (30, 7)], fill=GOLD)
    d.line([(29, 4), (25, 9), (24, 14)], fill=ORANGE, width=1)
    d.polygon([(32, 2), (30, 1), (29, 4)], fill=GOLD)
    if pose in ("up", "fly1", "fly2"):
        wing(d, up)
    # the outline: every clear pixel next to the unicorn
    px = im.load()
    solid = [[px[x, y][3] > 0 for x in range(W)] for y in range(H)]
    for y in range(H):
        for x in range(W):
            if solid[y][x]:
                continue
            if any(0 <= x + dx < W and 0 <= y + dy < H and solid[y + dy][x + dx] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[x, y] = OUT
    return im


SIZES = (66, 52, 40, 30, 22, 16, 11)          # the flight's widths between 88 (2x) and 44 (1x), and below


def smaller(im, w):
    """the frame w wide: shrunk from 2x, hard-edged again, in its own colours; outlined anew while big enough"""
    big = im.resize((W * 2, H * 2), Image.NEAREST)
    h = round(H * w / W)
    sm = big.resize((w, h), Image.LANCZOS)
    pal = im.convert("RGB").getcolors(256)
    colours = [c for _, c in pal]
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px, q = sm.load(), out.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 128:
                continue
            r, g, b = (min(255, v * 255 // max(a, 1)) for v in (r, g, b))
            q[x, y] = min(colours, key=lambda c: (c[0] - r) ** 2 + (c[1] - g) ** 2 + (c[2] - b) ** 2) + (255,)
    if w >= 30:
        solid = [[q[x, y][3] > 0 and q[x, y][:3] != OUT[:3] for x in range(w)] for y in range(h)]
        for y in range(h):
            for x in range(w):
                if not solid[y][x] and any(0 <= x + dx < w and 0 <= y + dy < h and solid[y + dy][x + dx]
                                           for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    q[x, y] = OUT
    return out


out = Path(sys.argv[1])
for name, pose in (("uni_a", "a"), ("uni_b", "b"), ("uni_up", "up"), ("uni_fly1", "fly1"), ("uni_fly2", "fly2"),
                   ("uni_sleep", "sleep")):
    frame(pose).save(out / f"{name}.png")
n = 0
for k, pose in ((1, "fly1"), (2, "fly2")):
    f = frame(pose)
    f.transpose(Image.FLIP_LEFT_RIGHT).save(out / f"uni_fly{k}_l.png")
    for w in SIZES:
        s = smaller(f, w)
        s.save(out / f"uf{w}_{k}r.png")
        s.transpose(Image.FLIP_LEFT_RIGHT).save(out / f"uf{w}_{k}l.png")
        n += 2
print(f"unicorn: 6 frames, 44 x 42; the flight: {n + 2} more")
