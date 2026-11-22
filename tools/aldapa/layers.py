# SPDX-License-Identifier: GPL-3.0-only
"""Aldapeko Sagarra (D-063): the scene in layers, our own pixel art: the still background and the sprites that move.
  tools/aldapa/layers.py OUT_DIR    writes ald_bg.png (at most 255 colours, for tools/gen_images.py), ald_robin_*.png,
                                    ald_sheep*_*.png and ald_layers.json (where each sprite goes)"""
import math, sys, json
from PIL import Image
import golden2 as G
from draw import Pic, hexc, W, H, baserri

out = sys.argv[1]
# the background: everything that stays still (no robin, no sheep, no notes, no smoke)
p = Pic()
G.sky(p); G.sun_disc(p); G.cloud_streaks(p); G.lit_clouds(p); G.mid_valley(p)
baserri(p, 88, 214, 64, 34, 'right', '#7a2e24', loft='#6b4a30', smoke=False)
G.hillside(p); G.stone_wall(p)
for sx in (138, 184):                                          # the sheep's long shadows stay on the grass
    y = int(G.slope_y(sx)) + 2 if sx == 138 else int(G.slope_y(sx)) + 6
    for i in range(36):
        for j in range(3): p.blend(sx + 14 + i, int(y + 22 - i * 0.42) + j, (40, 60, 90), 0.35 * (1 - i / 36))
tip = G.apple_tree(p, 214)
G.crown_leaves_over(p, *tip)
G.flare(p); G.foreground_frame(p); G.vignette(p)
p.im.quantize(colors=255, dither=Image.Dither.NONE).convert('RGB').save(f'{out}/ald_bg.png')

KEY = (255, 0, 255)
def sprite(draw, w, h, name):
    q = Pic(); q.rect(0, 0, W, H, KEY); draw(q)
    im = q.im.convert('RGBA'); px = im.load()
    for y in range(H):
        for x in range(W):
            if px[x, y][:3] == KEY: px[x, y] = (0, 0, 0, 0)
    bb = im.getbbox(); im.crop(bb).save(f'{out}/ald_{name}.png'); return bb

meta = {}
# the robin: resting, singing (the beak open, a lift of the chest), head tilted
import robin2
def robin_frame(kind):
    def d(q):
        tx, ty = tip
        robin2.draw(q, tx - 12, ty - 20, kind)                 # its feet on the branch's tip
    return d
for k in ('rest', 'sing', 'tilt', 'shy'): meta['robin_' + k] = sprite(robin_frame(k), 0, 0, 'robin_' + k)
# the sheep: grazing (head down) and looking up
def sheep_graze(x, y): return lambda q: G.sheep_rim_body(q, x, y, up=False)
def sheep_up(x, y): return lambda q: G.sheep_rim_body(q, x, y, up=True)
for i, sx in enumerate((138, 184)):
    y = int(G.slope_y(sx)) + (2 if i == 0 else 6)
    meta[f'sheep{i}_graze'] = sprite(sheep_graze(sx, y), 0, 0, f'sheep{i}_graze')
    meta[f'sheep{i}_up'] = sprite(sheep_up(sx, y), 0, 0, f'sheep{i}_up')
# where the windows and the chimney are, and the hill's line (it hides the baserri's lower half)
x0, base, w, wall = 88, 214, 64, 34
top = base - wall; cx = x0 + w // 2; apex = top - 30
meta['windows'] = [(x0 + 14, base - 30), (x0 + w - 26, base - 30)]
meta['chimney'] = (cx + 27, apex + 6)
meta['tip'] = tip
json.dump(meta, open(f'{out}/ald_layers.json', 'w'))
print(meta)
