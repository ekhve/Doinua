# SPDX-License-Identifier: GPL-3.0-only
"""Aldapeko Sagarra: a golden late afternoon just before autumn (the owner's brief). 240 x 240, our own pixel art."""
import math, random, sys
from PIL import Image
from draw import Pic, hexc, mix, BAYER, W, H, mist, baserri

SUN = (54, 104)                                                # low in the west, over the valley
AMBER = (255, 196, 110)

def slope_y(x): return 246 - x * 0.46                         # the steep hillside (aldapa), up to the right

def sky(p):
    p.grad(0, 132, ['#55689e', '#8a86ae', '#c79e9a', '#eeb784', '#ffd58c', '#ffe6a8'])
    sx, sy = SUN
    for y in range(0, 170):                                    # the low sun's amber haze
        for x in range(W):
            d = math.hypot((x - sx) * 0.8, y - sy)
            if d < 110:
                a = (1 - d / 110) ** 2.2 * 0.85
                if a * 16 > BAYER[y % 4][x % 4] * 0.55: p.blend(x, y, (255, 214, 140), min(0.85, a + 0.1))
    p.disc(sx, sy, 15, (255, 236, 190), 0.55); p.disc(sx, sy, 11, (255, 246, 220))

def clouds(p, seed):
    rnd = random.Random(seed)
    for (cx, cy, n, sc) in ((150, 44, 9, 1.0), (214, 70, 6, 0.8), (96, 62, 5, 0.7), (20, 34, 5, 0.7)):
        for k in range(n):                                     # soft cumulus, lit gold from below-left, violet above
            x = cx + rnd.uniform(-22, 22) * sc; y = cy + rnd.uniform(-5, 5) * sc; r = rnd.uniform(5, 10) * sc
            p.disc(x, y, r, hexc('#d6aeb0'), 0.55)
            p.disc(x - r * 0.25, y + r * 0.3, r * 0.75, hexc('#f6c08e'), 0.9)
            p.disc(x - r * 0.4, y + r * 0.45, r * 0.4, hexc('#ffe2b0'), 0.9)

def ridge(p, base, height, col, top_col, seed, jag, until):
    rnd = random.Random(seed); raw = []; y = base - height * 0.5; v = 0
    for x in range(W + 14):
        v = v * 0.86 + rnd.uniform(-1, 1) * jag; y = min(base - 4, max(base - height, y + v)); raw.append(y)
    pts = [sum(raw[x:x + 13]) / 13 for x in range(W)]
    for x in range(W):
        y = int(pts[x])
        for yy in range(y, until): p.put(x, yy, hexc(col))
        if pts[x - 1 if x else 0] > pts[x] + 0.05 or x < 3: p.put(x, y, hexc(top_col)); p.put(x, y + 1, hexc(top_col))   # sunlit edges
    return pts

def tree_blob(p, x, y, r, dark, mid, lit):
    p.disc(x + 1, y + 1, r, hexc(dark)); p.disc(x, y, r - 1, hexc(mid)); p.disc(x - r * 0.35, y - r * 0.3, r * 0.45, hexc(lit))

def forest_edge(p, y0):
    """the beech and oak wood at the foot of the hill (oihanaren zolan): dense, rounded, warm on the sun side"""
    rnd = random.Random(5)
    p.rect(0, y0, W, 70, hexc('#2c4a2c'))
    for k in range(70):
        x = rnd.uniform(-8, 200); y = y0 + rnd.uniform(-6, 26); r = rnd.uniform(6, 11)
        oak = rnd.random() < 0.5
        tree_blob(p, x, y, r, '#23401f' if oak else '#2f4a26', '#3f6a33' if oak else '#5a7a34', '#a8b858' if not oak else '#7fa858')
        if rnd.random() < 0.18: p.disc(x + rnd.uniform(-3, 3), y + rnd.uniform(-3, 3), 2.5, hexc('#c9963a'))   # a first yellow
    for k in range(14): x = rnd.uniform(0, 190); p.rect(int(x), y0 + 20, 2, 8, hexc('#3a2a1c'))

def stream_bridge(p):
    """a stream from the wood, a little wooden bridge, a stone wall up the slope"""
    for i in range(70):                                        # the stream, glinting
        x = 6 + i; y = 214 + int(math.sin(i / 9) * 3) + i // 5
        p.rect(x, y, 4, 2, hexc('#6f8fb0')); p.put(x + 1, y, hexc('#ffe6b0') if i % 7 == 0 else hexc('#8fb0cc'))
    bx, by = 34, 214                                           # the bridge: planks, two rails
    for k in range(9): p.rect(bx + k * 3, by + (1 if k in (0, 8) else 0), 2, 3, hexc('#7a5230'))
    p.rect(bx - 1, by - 5, 28, 1, hexc('#5a3a20')); p.rect(bx, by - 5, 1, 6, hexc('#5a3a20')); p.rect(bx + 25, by - 5, 1, 6, hexc('#5a3a20'))
    for i in range(46):                                        # a dry-stone wall climbing the slope
        x = 62 + i * 2; y = int(slope_y(x)) + 22 - i // 4
        p.rect(x, y, 2, 3, hexc('#9a9284' if (i % 3) else '#b5ad9e')); p.put(x, y + 3, hexc('#6a6458'))

def hillside(p):
    """the steep pasture: tall grass, gold on the sun side, its blades swaying a little; the hill's own shadow"""
    rnd = random.Random(2)
    for x in range(W):
        y0 = int(slope_y(x))
        for y in range(max(0, y0), H):
            t = (y - y0) / 60
            c = mix(hexc('#a6c45a'), hexc('#4f8a3a'), min(1, t * 1.6)) if y < y0 + 4 else mix(hexc('#5f9a40'), hexc('#3f7432'), min(1, (y - y0) / 90))
            p.put(x, y, c)
    for k in range(700):                                       # blades of tall grass
        x = rnd.randrange(W); y = int(slope_y(x)) + rnd.randrange(1, 100)
        if y >= H: continue
        h = rnd.randrange(2, 6); lit = rnd.random() < 0.45
        for j in range(h): p.put(x + (1 if j > h // 2 and k % 3 == 0 else 0), y - j, hexc('#d8d070') if lit and j == h - 1 else hexc('#6fa848') if lit else hexc('#355f2c'))

def lilies(p, pts):
    for (x, y) in pts:                                         # wild white lilies, late in the season
        p.rect(x, y, 1, 7, hexc('#3f7a32'))
        for (dx, dy) in ((-2, -1), (2, -1), (-1, -2), (1, -2), (0, -1), (-1, 0), (1, 0)): p.put(x + dx, y + dy, hexc('#fbf7ee'))
        p.put(x, y - 1, hexc('#f2d06a'))

def shadow(p, x, y, length, h=3):
    """a long late-afternoon shadow, cast up the slope away from the sun"""
    for i in range(length):
        for j in range(h):
            xx = x + i; yy = int(y - i * 0.46) + j
            p.blend(xx, yy, (40, 70, 40), 0.35 * (1 - i / length))

def apple_tree(p, tx):
    """the old apple tree (sagarrondoa): a gnarled, leaning trunk, a wide crown heavy with red-and-yellow cider apples,
    warm amber light on its sun side, and one high thin branch up to the bird"""
    base = int(slope_y(tx)) + 4
    T, TD, TL = hexc('#5a3a22'), hexc('#3a2414'), hexc('#a8703a')
    trunk = []
    for i in range(70):                                        # the trunk leans into the slope, twisting
        y = base - i; x = tx - i * 0.18 + math.sin(i / 8) * 3
        w = 13 - i // 9
        trunk.append((x, y))
        for k in range(w):
            c = TL if k < 2 else (TD if k > w - 3 else T)
            p.put(int(x - w / 2 + k), y, c)
        if i % 9 == 4: p.put(int(x), y, TD); p.put(int(x) + 1, y, TD)   # bark furrows
    for (dx, dy, l, ang) in ((0, 50, 34, -2.4), (2, 58, 30, -0.7), (-2, 40, 22, -2.9), (0, 64, 26, -1.9)):   # big boughs
        x0, y0 = trunk[dy][0] + dx, trunk[dy][1]
        for j in range(l):
            x = x0 + math.cos(ang) * j; y = y0 + math.sin(ang) * j
            for k in range(max(2, 5 - j // 8)): p.put(int(x) + k, int(y), T if k else TL)
    # the crown
    rnd = random.Random(11)
    cx, cy, r = tx - 14, base - 86, 46
    DARK, MID, LIT, GOLD = '#2c5a2c', '#4a8238', '#9ab84a', '#e8c25a'
    for k in range(120):
        a = rnd.uniform(0, math.pi * 2); d = rnd.uniform(0, r) ** 0.9 * 1.0
        x = cx + math.cos(a) * d * 1.15; y = cy + math.sin(a) * d * 0.62
        rr = rnd.uniform(5, 10)
        sun_side = x < cx - 6
        p.disc(x + 1, y + 2, rr, hexc(DARK))
        p.disc(x, y, rr - 1, hexc(MID))
        p.disc(x - rr * 0.35, y - rr * 0.35, rr * 0.5, hexc(GOLD if sun_side and rnd.random() < 0.6 else LIT))
    for k in range(30):                                        # a few leaves already turning
        x = cx + rnd.uniform(-r, r) * 1.1; y = cy + rnd.uniform(-r, r) * 0.55
        p.put(int(x), int(y), hexc('#d89a3a')); p.put(int(x) + 1, int(y), hexc('#c8803a'))
    for k in range(28):                                        # apples: red, yellow, and yellow with a red blush
        x = cx + rnd.uniform(-r, r) * 1.05; y = cy + rnd.uniform(-r * 0.2, r) * 0.6
        kind = k % 3
        body = hexc(['#c42a1e', '#e8c23a', '#e0a83a'][kind])
        p.disc(x, y, 3, body)
        if kind == 2: p.disc(x + 1, y - 1, 1.6, hexc('#d0402a'))
        p.put(int(x) - 1, int(y) - 1, hexc('#fff0c0')); p.put(int(x), int(y) - 3, hexc('#3a2414'))
    # the highest branch: a thin twig rising out of the crown, the bird on its very tip
    tipx, tipy = cx - 20, cy - 38
    x0, y0 = cx - 8, cy - 20
    for i in range(30):
        t = i / 29
        x = x0 + (tipx - x0) * t + math.sin(t * 3) * 2; y = y0 + (tipy - y0) * t
        p.put(int(x), int(y), T); p.put(int(x) + 1, int(y), TL if t < 0.7 else T)
    for (lx, ly) in ((tipx + 4, tipy + 10), (tipx - 1, tipy + 18), (tipx + 6, tipy + 4)):   # a few leaves on the twig
        p.disc(lx, ly, 2.2, hexc(MID)); p.put(int(lx) - 1, int(ly) - 1, hexc(GOLD))
    shadow(p, tx + 4, base + 2, 70, 5)
    return (tipx, tipy)

ROBIN = [  # 20 x 16, facing left (into the valley), perched: the chest puffed out, the beak open, the toes round the twig
 ".......oooooo.......",
 ".....oobbbbbboo.....",
 "....obbbbbbbbbbo....",
 "...orrrrbbbbbbbbo...",
 "..orrrrrrrbbbbbbo...",
 ".orrkkrrrrbbbbbbbo..",
 "yyrkwrrrrrrbbbbBbbo.",
 "..rrrrrrrrrrbbbBBbbo",
 "y.orrrrrrrrrrbbbBBbo",
 "..orrrrrrrrrrwbbbBBo",
 "..orrrrrrrrrwwbbbBBo",
 "...orrrrrrwwwbbbbBBo",
 "....owwwwwwwwoooooBBo",
 ".....owwwwwwo...oBBBo",
 "......oooooo.....ooo.",
 ".......l...l........"]
RP = {'o': '#2a1d14', 'b': '#86603c', 'B': '#634428', 'r': '#f0662a', 'k': '#111111', 'w': '#f4ecdc', 'y': '#3a2a1a', 'l': '#5a3a20'}
def robin(p, tipx, tipy, s=2):
    """standing on the twig's tip: its feet on the wood, the twig passing under its toes"""
    x = tipx - 8 * s; y = tipy - 15 * s
    for j, row in enumerate(ROBIN):
        for i, c in enumerate(row):
            if c in RP: p.rect(x + i * s, y + j * s, s, s, hexc(RP[c]))
    p.rect(x + 4 * s, y + 5 * s, 1, 1, hexc('#ffffff'))
    for j in range(3, 12):                                     # the warm rim of low sunlight along its breast and head
        for i in range(20):
            if ROBIN[j][i] in 'r':
                p.rect(x + i * s, y + j * s, 1, s, hexc('#ffb070')); break
    p.rect(x + 6 * s, y + 7 * s, s * 2, s, hexc('#e0503a'))     # the open beak's red
    for (fx, fy) in ((x + 7 * s, y + 15 * s), (x + 11 * s, y + 15 * s)):   # toes curled round the twig
        p.rect(fx - 1, fy + 1, 4, 1, hexc('#5a3a20'))
    p.rect(tipx - 10, tipy, 22, 2, hexc('#5a3a22')); p.rect(tipx - 10, tipy, 22, 1, hexc('#a8703a'))   # the twig's tip under its feet

def latxa(p, x, y, head='down'):
    """a latxa grazing, facing left: long shaggy cream wool to its knees, a black face down to the grass, ears out,
    a curl of horn, black legs; warm light on its back"""
    WOOL, WD, WL, FACE = hexc('#ece2c8'), hexc('#c4b48e'), hexc('#fff2d0'), hexc('#2a201c')
    rnd = random.Random(x)
    for k in range(34):                                        # the fleece in curly locks
        cx, cy = x + rnd.uniform(6, 28), y + rnd.uniform(3, 13)
        p.disc(cx, cy + 1, 3, WD); p.disc(cx, cy, 2.4, WOOL); p.put(int(cx) - 1, int(cy) - 1, WL)
    for dx in range(6, 30, 2):                                 # long wool hanging down
        for dy in range(0, 5 + (dx % 4)): p.put(x + dx, y + 14 + dy, WD if (dx + dy) % 3 else WOOL)
    if head == 'down':
        for i in range(11):                                    # the neck and the black face, down to the grass
            p.rect(x + 6 - i // 2, y + 6 + i, 5, 1, WOOL if i < 3 else FACE)
        p.rect(x, y + 15, 4, 3, FACE)                          # the muzzle in the grass
        p.rect(x + 7, y + 7, 4, 2, FACE); p.put(x + 11, y + 8, FACE)   # an ear
        p.put(x + 3, y + 11, hexc('#ffffff'))
        for a in range(8): p.put(x + 6 + int(math.cos(a / 2.5) * 3), y + 8 + int(math.sin(a / 2.5) * 2), hexc('#c9b48a'))   # the horn's curl
    else:                                                      # looking up: the neck raised out of the fleece, the face out front
        for i in range(6): p.rect(x + 4 - i // 3, y + 4 - i, 5, 1, WOOL)
        p.rect(x - 2, y - 7, 6, 5, FACE); p.rect(x - 4, y - 4, 3, 3, FACE)   # the long black face, the muzzle
        p.rect(x + 3, y - 7, 4, 2, FACE); p.put(x + 7, y - 6, FACE)          # an ear
        p.put(x, y - 6, hexc('#ffffff'))
        for a in range(8): p.put(x + 3 + int(math.cos(a / 2.5) * 3), y - 4 + int(math.sin(a / 2.5) * 2), hexc('#c9b48a'))
    for lx in (x + 8, x + 12, x + 22, x + 26): p.rect(lx, y + 18, 2, 6, FACE)
    for i in range(18): p.put(x + 10 + i, y + 2, WL)           # the low sun on its back

def notes(p):
    def note(x, y, c):
        c = hexc(c)
        p.rect(x, y + 5, 4, 3, c); p.rect(x + 3, y, 1, 7, c); p.rect(x + 3, y, 3, 1, c); p.rect(x + 5, y + 1, 1, 2, c)
    note(124, 20, '#ff9f1c'); note(110, 8, '#ffe14d'); note(138, 30, '#ff6bc6')

def scene():
    p = Pic()
    sky(p)
    clouds(p, 4)
    ridge(p, 116, 30, '#8f8fb4', '#f2c6a0', 3, 1.2, 140)           # far mountains, hazy violet
    mist(p, 108, 12, '#ffe8cc', 0.45)
    ridge(p, 128, 22, '#6f7f8a', '#e8c090', 9, 0.9, 150)           # nearer ridges, blue-green in haze
    mist(p, 122, 12, '#ffe0b8', 0.4)
    ridge(p, 146, 20, '#4f7046', '#c8b864', 14, 0.7, 200)          # the valley's green hills, gold-tipped
    baserri(p, 14, 180, 78, 36, 'right', '#7a2e24', loft='#6b4a30')                # further down: the baserri
    forest_edge(p, 188)
    hillside(p)
    stream_bridge(p)
    for (sx, sy) in ((58, 206), (96, 190), (126, 180)):            # the flock, grazing across the slope
        shadow(p, sx + 18, int(slope_y(sx + 18)) + 22, 40)
    latxa(p, 58, int(slope_y(58)) + 4)
    latxa(p, 98, int(slope_y(98)) + 6)
    latxa(p, 130, int(slope_y(130)) + 8)
    lilies(p, [(20, 236), (28, 232), (110, 232), (150, 226), (200, 214), (226, 206)])
    tip = apple_tree(p, 196)
    robin(p, *tip)
    notes(p)
    return p

if __name__ == '__main__':
    im = scene().im
    im.save(sys.argv[1] + '/golden.png'); im.resize((720, 720), Image.NEAREST).save(sys.argv[1] + '/golden_3x.png')
    print('ok')
