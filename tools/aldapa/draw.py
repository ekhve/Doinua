# SPDX-License-Identifier: GPL-3.0-only
"""Aldapeko Sagarra: three concept scenes, 240 x 240, pixel art drawn by code (our own)."""
import math, random, sys
from PIL import Image, ImageDraw
W = H = 240
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]

def hexc(h): h = h.lstrip('#'); return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))
def mix(a, b, t): return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))

class Pic:
    def __init__(s): s.im = Image.new('RGB', (W, H)); s.px = s.im.load(); s.d = ImageDraw.Draw(s.im)
    def put(s, x, y, c):
        if 0 <= x < W and 0 <= y < H: s.px[x, y] = c
    def get(s, x, y): return s.px[x, y] if 0 <= x < W and 0 <= y < H else (0, 0, 0)
    def blend(s, x, y, c, a):
        if 0 <= x < W and 0 <= y < H: s.px[x, y] = mix(s.px[x, y], c, a)
    def rect(s, x, y, w, h, c):
        x, y, w, h = int(round(x)), int(round(y)), int(round(w)), int(round(h))
        for yy in range(y, y + h):
            for xx in range(x, x + w): s.put(xx, yy, c)
    def grad(s, y0, y1, stops):
        """a vertical gradient, ordered-dithered between bands (8-bit look)"""
        cols = [hexc(c) for c in stops]
        for y in range(y0, y1):
            t = (y - y0) / max(1, y1 - y0 - 1) * (len(cols) - 1)
            i = min(int(t), len(cols) - 2); f = t - i
            for x in range(W):
                steps = 4
                q = f * steps; lo = int(q); fr = q - lo
                f2 = (lo + (1 if fr * 16 > BAYER[y % 4][x % 4] else 0)) / steps
                s.put(x, y, mix(cols[i], cols[i + 1], min(1, f2)))
    def disc(s, cx, cy, r, c, a=1.0):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                    if a >= 1: s.put(x, y, c)
                    else: s.blend(x, y, c, a)
    def ridge(s, y_of, c, top_c=None, until=H):
        for x in range(W):
            y = int(y_of(x))
            for yy in range(max(0, y), until): s.put(x, yy, c)
            if top_c: s.put(x, y, top_c); s.put(x, y + 1, top_c)

def sky_and_sun(p, stops, sun, sun_r=12, rays=False, glow=(255, 220, 150)):
    p.grad(0, 150, stops)
    sx, sy = sun
    for y in range(0, 160):                                    # the sun's glow, dithered rings
        for x in range(W):
            d = math.hypot(x - sx, y - sy)
            if d < 70:
                a = (1 - d / 70) ** 2 * 0.75
                if a * 16 > BAYER[y % 4][x % 4] * 0.6: p.blend(x, y, glow, min(0.9, a + 0.15))
    if rays:
        for k in range(14):
            ang = -math.pi + k * math.pi / 13
            for r in range(sun_r + 4, 230):
                if (k % 2 == 0):
                    for w in (-1, 0, 1):
                        x = int(sx + math.cos(ang) * r + w * math.sin(ang) * r / 30); y = int(sy + math.sin(ang) * r)
                        if 0 <= y < 200 and (x + y) % 2 == 0: p.blend(x, y, (255, 238, 190), 0.10 * (1 - r / 230))
    p.disc(sx, sy, sun_r + 2, (255, 244, 205), 0.6); p.disc(sx, sy, sun_r, (255, 250, 228))

def peaks(p, base, height, c_shadow, c_lit, seed, jag=1.0, peak_x=None, until=H, sun_left=True):
    rnd = random.Random(seed)
    raw = []
    y = base - height * 0.5; v = 0
    for x in range(W + 12):                                    # a ridge: a wandering line, smoothed
        v = v * 0.85 + rnd.uniform(-1, 1) * jag; y += v
        y = min(base - 6, max(base - height, y)); raw.append(y)
    pts = [sum(raw[x:x + 9]) / 9 for x in range(W)]
    if peak_x is not None:
        pts = [min(y, base - height * 0.2 - max(0, 40 - abs(x - peak_x)) * height / 40 * 1.05) for x, y in enumerate(pts)]
    for x in range(W):                                         # lit faces turned to the sun, a band along the crest
        y = int(pts[x]); slope = (pts[x + 1] - pts[x - 1]) if 0 < x < W - 1 else 0
        lit_face = slope > 0.15 if sun_left else slope < -0.15
        for yy in range(y, until):
            c = hexc(c_shadow)
            if lit_face and yy < y + 12 + (x * 7) % 5: c = mix(hexc(c_lit), hexc(c_shadow), (yy - y) / 16)
            p.put(x, yy, c)
        p.put(x, y, hexc(c_lit))
    return pts

def cloud_sea(p, y0, c_top, c_body, c_shade, seed):
    rnd = random.Random(seed)
    for k in range(60):                                        # billows: discs, lit from above
        cx = rnd.uniform(-10, 250); cy = y0 + rnd.uniform(-2, 16); r = rnd.uniform(6, 16)
        p.disc(cx, cy + 2, r, hexc(c_shade)); p.disc(cx, cy, r - 1, hexc(c_body)); p.disc(cx - r * 0.25, cy - r * 0.35, r * 0.55, hexc(c_top))
    p.rect(0, y0 + 14, W, 40, hexc(c_body))

def mist(p, y, h, c, a):
    for yy in range(y, y + h):
        for x in range(W):
            if (x // 3 + yy) % 3 != 0: p.blend(x, yy, hexc(c), a * (1 - abs(yy - y - h / 2) / (h / 2 + 1)))

def baserri(p, x0, base, w=124, wall=58, light='left', shutters='#3f6f3f', loft='#2a2420', smoke=True):
    """a Basque farmhouse, gable to the front: wide low roof, whitewashed walls, timber frame and balcony upstairs,
    a stone arch below, small windows with shutters, peppers and corn drying on the balcony"""
    WHITE, WSH, STONE, STONE_D, WOOD, WOOD_D, ROOF, ROOF_D, ROOF_L, DARK = (hexc(c) for c in
        ('#f3efe5', '#d9d2c4', '#b3aa9a', '#8b8274', '#7a4a2a', '#5a341c', '#b0553b', '#86402c', '#cf7656', '#2a2420'))
    top = base - wall
    for y in range(top, base):                                 # walls, the shaded side darker
        for x in range(x0, x0 + w):
            sh = (x > x0 + w * 0.62) if light == 'left' else (x < x0 + w * 0.38)
            p.put(x, y, WSH if sh else WHITE)
    for y in range(top + 2, base, 7):                          # stone quoins at the corners
        for (qx, qw) in ((x0, 6 if (y // 7) % 2 else 9), (x0 + w - (6 if (y // 7) % 2 else 9), 6 if (y // 7) % 2 else 9)):
            p.rect(qx, y, qw, 6, STONE); p.rect(qx, y + 5, qw, 1, STONE_D)
    cx = x0 + w // 2
    # upstairs: the timber frame and the balcony (ganbara) under the gable
    p.rect(x0, top + 22, w, 3, WOOD); p.rect(x0, top + 24, w, 1, WOOD_D)
    for bx in range(x0 + 14, x0 + w - 10, 22):
        p.rect(bx, top, 3, 22, WOOD)
    for i in range(18):                                        # braces
        p.put(x0 + 17 + i, top + 21 - i, WOOD); p.put(x0 + w - 20 - i, top + 21 - i, WOOD)
    # the roof: a wide, low gable with an overhang, rows of tiles
    apex = top - 30; over = 12
    for y in range(apex, top + 4):
        t = (y - apex) / (top + 4 - apex)
        half = int((w / 2 + over) * t) + 2
        for x in range(cx - half, cx + half + 1):
            c = ROOF if (y - apex) % 4 else ROOF_D
            if (x - cx) * (-1 if light == 'left' else 1) > 0 and (y - apex) % 4: c = ROOF_L
            p.put(x, y, c)
    for y in range(apex + 2, top + 4):                         # the gable's wooden edge (bargeboards)
        t = (y - apex) / (top + 4 - apex); half = int((w / 2 + over) * t)
        p.put(cx - half, y, WOOD_D); p.put(cx + half, y, WOOD_D); p.put(cx - half - 1, y, WOOD_D); p.put(cx + half + 1, y, WOOD_D)
    # the gable's wall under the roof: white, with the balcony across it
    for y in range(apex + 8, top):
        t = (y - apex) / (top + 4 - apex); half = int((w / 2) * t) - 3
        for x in range(cx - half, cx + half): p.put(x, y, WHITE if light == 'left' and x < cx + half * 0.3 else WSH)
    by = top - 6
    lw = min(34, w // 2 - 8)
    p.rect(cx - lw, by - 12, lw * 2, 12, hexc(loft))           # the open loft
    p.rect(cx - 36, by, 72, 2, WOOD); p.rect(cx - 36, by + 6, 72, 2, WOOD_D)
    for bx in range(cx - 35, cx + 36, 4): p.rect(bx, by, 1, 8, WOOD)
    for i, bx in enumerate(range(cx - 30, cx + 30, 6)):        # ristras: red peppers, golden corn
        col = hexc('#c8281e') if i % 2 == 0 else hexc('#f2b83a')
        for k in range(9):
            p.put(bx + (k % 2), by - 10 + k, col); p.put(bx + 1 - (k % 2), by - 10 + k, mix(col, (0, 0, 0), 0.25))
    # downstairs: the stone arch, the dark entrance and its half-open door
    aw, ah = 34, 34; ax = cx - aw // 2
    for y in range(base - ah, base):
        for x in range(ax, ax + aw):
            dx = (x - cx) / (aw / 2); dy = (base - ah + aw / 2 - y) / (aw / 2)
            inside = dy <= 0 or dx * dx + dy * dy <= 1
            if inside: p.put(x, y, DARK)
    acy = base - ah + aw / 2                                   # the arch's voussoirs: a ring of dressed stones
    for y in range(int(acy - aw / 2 - 5), int(acy) + 1):
        for x in range(cx - aw // 2 - 5, cx + aw // 2 + 6):
            d = math.hypot(x - cx, (y - acy))
            if aw / 2 <= d <= aw / 2 + 4:
                k = int((math.atan2(acy - y, x - cx)) / math.pi * 11)
                p.put(x, y, STONE_D if (d > aw / 2 + 3 or k % 2) else STONE)
    p.rect(ax - 3, base - ah + aw // 2, 3, ah - aw // 2, STONE); p.rect(ax + aw, base - ah + aw // 2, 3, ah - aw // 2, STONE_D)
    p.rect(cx - 15, base - 22, 12, 22, WOOD); p.rect(cx - 15, base - 22, 12, 1, WOOD_D)
    # windows with shutters, geraniums on the sills
    sc = hexc(shutters)
    for (wx, wy) in ((x0 + 14, base - 30), (x0 + w - 26, base - 30), (x0 + 22, top + 4), (x0 + w - 34, top + 4)):
        p.rect(wx, wy, 12, 13, DARK); p.rect(wx + 1, wy + 1, 10, 11, hexc('#3b4a5a'))
        p.rect(wx + 5, wy + 1, 1, 11, WOOD_D); p.rect(wx + 1, wy + 6, 10, 1, WOOD_D)
        p.rect(wx - 5, wy, 5, 13, sc); p.rect(wx + 12, wy, 5, 13, sc)
        p.rect(wx - 1, wy + 13, 14, 2, STONE)
        for k in range(5): p.put(wx + 1 + k * 2, wy + 12 - (k % 2), hexc('#e8343a')); p.put(wx + 2 + k * 2, wy + 13, hexc('#3f7a3a'))
    # the chimney and a thread of smoke
    p.rect(cx + 24, apex + 8, 6, 10, STONE); p.rect(cx + 23, apex + 7, 8, 2, STONE_D)
    for k in range(40 if smoke else 0):
        p.blend(int(cx + 27 + math.sin(k / 5) * 3 + k * 0.4), apex + 6 - k, (235, 232, 240), 0.5 * (1 - k / 40))
        p.blend(int(cx + 28 + math.sin(k / 5) * 3 + k * 0.4), apex + 6 - k, (235, 232, 240), 0.3 * (1 - k / 40))

def apple_tree(p, tx, base, crown=(0, 0, 34), blossom=False, apples=True, branch_to=None):
    T, TD = hexc('#6b4426'), hexc('#4a2e18')
    for y in range(base - 52, base):                           # a gnarled trunk
        wob = int(math.sin(y / 7) * 2)
        p.rect(tx - 5 + wob, y, 10, 1, T); p.put(tx + 4 + wob, y, TD); p.put(tx + 5 + wob, y, TD)
    if branch_to:                                              # the long branch out to the robin
        bx, by = branch_to
        x1, y1 = tx - 2, base - 50
        for i in range(120):
            t = i / 119
            x = x1 + (bx - x1) * t; y = y1 + (by - y1) * t - math.sin(t * math.pi) * 10
            th = 3 - int(t * 2)
            for k in range(th): p.put(int(x), int(y) + k, T if k < th - 1 else TD)
        for (lx, ly) in ((bx + 6, by - 4), (bx + 16, by - 10), (bx + 28, by - 14)):   # a few leaves on it
            leaf = hexc('#f6d8e0') if blossom else hexc('#5aa84e')
            p.disc(lx, ly, 3, leaf); p.put(lx - 1, ly - 1, hexc('#ffffff') if blossom else hexc('#7cc46a'))
    cx, cy, r = tx + crown[0], base - 52 - crown[2] + 8 + crown[1], crown[2]
    rnd = random.Random(7)
    dark, mid, lit = (hexc('#2f6b3a'), hexc('#3f8a45'), hexc('#68b45a')) if not blossom else (hexc('#c98aa0'), hexc('#ecbccb'), hexc('#fbe8ee'))
    for k in range(70):                                        # the crown: clumps, lit from the upper left
        a = rnd.uniform(0, math.pi * 2); d = rnd.uniform(0, r * 0.9)
        x = cx + math.cos(a) * d; y = cy + math.sin(a) * d * 0.8
        rr = rnd.uniform(5, 10)
        p.disc(x + 1, y + 2, rr, dark); p.disc(x, y, rr - 1, mid); p.disc(x - rr * 0.3, y - rr * 0.35, rr * 0.45, lit)
    if blossom:
        for k in range(26): x = cx + rnd.uniform(-r, r); y = cy + rnd.uniform(-r * 0.7, r * 0.7); p.disc(x, y, 1.5, hexc('#5aa84e'))
    if apples:
        for k in range(12):
            x = cx + rnd.uniform(-r * 0.8, r * 0.8); y = cy + rnd.uniform(-r * 0.5, r * 0.7)
            p.disc(x, y, 3, hexc('#c8281e')); p.put(int(x) - 1, int(y) - 1, hexc('#ff8a70')); p.put(int(x), int(y) - 3, hexc('#4a2e18'))

ROBIN = [  # 20 x 17, facing left: the txantxangorri, bigger and finer
 "......oooooo........",
 "....oobbbbbboo......",
 "...obbbbbbbbbbo.....",
 "..orrrrbbbbbbbbo....",
 ".orrrrrrbbbbbbbo....",
 "orrkkrrrrbbbbbbbo...",
 "yyrkwrrrrrbbbbBbbo..",
 ".yrrrrrrrrrbbbBBbbo.",
 "..orrrrrrrrrbbbBBbbo",
 "..orrrrrrrrrwbbbBBbo",
 "...orrrrrrrwwbbbbBBo",
 "....owrrrwwwwbbbbBBo",
 ".....owwwwwwwoooooBBo",
 "......owwwwwo....oBBo",
 ".......ooooo......ooo",
 "........l..l........",
 ".......ll.ll........"]
RP = {'o': '#2a1d14', 'b': '#8d5f39', 'B': '#6b4426', 'r': '#f26a2a', 'k': '#111111', 'w': '#f6f0e4', 'y': '#3a2a1a', 'l': '#6a4a30'}
def robin(p, x, y, sing=False, s=2):
    for j, row in enumerate(ROBIN):
        for i, c in enumerate(row):
            if c in RP: p.rect(x + i * s, y + j * s, s, s, hexc(RP[c]))
    p.rect(x + 4 * s, y + 5 * s, 1, 1, hexc('#ffffff'))
    for j in range(3, 12):                                     # a softer edge between the breast and the back
        p.put(x + (10 + j // 3) * s, y + j * s, hexc('#c86a3a'))
    if sing: p.rect(x - 2 * s, y + 4 * s, 2 * s, s, hexc(RP['y'])); p.rect(x - s, y + 6 * s, s, s, hexc(RP['y'])); p.rect(x - s, y + 5 * s, s, s, hexc('#e0503a'))

def pottoka(p, x, y):
    """a pottoka standing, head up, facing left: small and stocky, dark bay with a lighter back, a thick black mane,
    forelock and tail, black legs (drawn with outlined shapes, no smoothing)"""
    d = p.d
    OUTC, BAY, BAYL, BAYD, BLK, MUZ = '#1a120c', '#5a3620', '#7a4c2c', '#40261a', '#151010', '#8a6248'
    def ell(box, c): d.ellipse(box, fill=hexc(c))
    def poly(pts, c): d.polygon([(x + a, y + b) for a, b in pts], fill=hexc(c))
    # the tail, behind
    poly([(37, 4), (41, 6), (43, 14), (42, 24), (39, 26), (38, 16), (36, 8)], BLK)
    # far legs, darker
    for lx in (14, 31):
        poly([(lx, 14), (lx + 4, 14), (lx + 4, 26), (lx + 3, 30), (lx, 30)], BLK)
    # the body and chest (an outline first)
    ell((x + 5, y + 1, x + 41, y + 20), OUTC); ell((x + 6, y + 2, x + 40, y + 19), BAY)
    ell((x + 10, y + 2, x + 36, y + 9), BAYL)                    # the light along its back
    ell((x + 8, y + 12, x + 38, y + 20), BAYD)                   # the belly in shade
    # the neck, up and forward, and the head
    poly([(6, 10), (13, 3), (11, -6), (6, -12), (1, -9), (2, 2)], OUTC)
    poly([(7, 9), (12, 3), (10, -5), (6, -10), (2, -8), (3, 2)], BAY)
    poly([(-1, -13), (6, -15), (8, -10), (5, -3), (0, -2), (-4, -6)], OUTC)
    poly([(0, -12), (6, -14), (7, -10), (4, -4), (0, -3), (-3, -6)], BAY)
    poly([(-4, -7), (0, -4), (0, -2), (-4, -3)], MUZ)            # the muzzle
    poly([(5, -16), (7, -19), (8, -14)], OUTC)                   # an ear
    # the mane along the neck, and the forelock
    poly([(6, -13), (12, -6), (14, 2), (12, 4), (10, -3), (5, -10)], BLK)
    poly([(1, -14), (4, -15), (3, -10), (0, -9)], BLK)
    p.put(x + 2, y - 10, hexc('#ffffff'))                        # its eye's glint
    p.put(x - 3, y - 4, hexc(OUTC))                              # a nostril
    # near legs, with knees and hooves
    for lx in (9, 26):
        poly([(lx - 1, 13), (lx + 5, 13), (lx + 4, 22), (lx + 4, 30), (lx, 30), (lx, 22)], OUTC)
        poly([(lx, 14), (lx + 4, 14), (lx + 3, 22), (lx + 3, 29), (lx + 1, 29), (lx + 1, 22)], BAYD)
        p.rect(x + lx, y + 25, 4, 5, hexc(BLK))
    p.rect(x + 8, y + 30, 6, 1, hexc('#2e5a2a')); p.rect(x + 25, y + 30, 6, 1, hexc('#2e5a2a'))   # its shadow on the grass

def latxa(p, x, y):
    """a latxa sheep: long shaggy cream wool, a dark face, curling horns"""
    WOOL, WD, WL, FACE = hexc('#ece3cc'), hexc('#cbbf9f'), hexc('#fbf6e8'), hexc('#2e2420')
    rnd = random.Random(3)
    for k in range(40):
        cx, cy = x + rnd.uniform(4, 30), y + rnd.uniform(2, 14)
        p.disc(cx, cy + 1, 3.2, WD); p.disc(cx, cy, 2.6, WOOL); p.put(int(cx) - 1, int(cy) - 1, WL)
    for dy in range(4, 22):                                    # long wool, hanging
        for dx in range(5, 31, 3): p.put(x + dx + (dy % 2), y + 12 + dy // 3, WD if dy % 3 else WOOL)
    for dy in range(0, 12):
        for dx in range(0, 9):
            if (dx - 4) ** 2 / 20 + (dy - 6) ** 2 / 36 <= 1: p.put(x - 6 + dx, y + 2 + dy, FACE)
    p.put(x - 4, y + 6, hexc('#ffffff'))
    for a in range(10):                                        # the horn
        p.put(x + int(math.cos(a / 3) * 4), y + 3 + int(math.sin(a / 3) * 3), hexc('#c9b48a'))
    for lx in (x + 8, x + 12, x + 22, x + 26):
        p.rect(lx, y + 18, 2, 7, FACE)

def meadow(p, y_of, c, c_light, flowers=True, seed=1):
    rnd = random.Random(seed)
    for x in range(W):
        y = int(y_of(x))
        for yy in range(y, H):
            t = (yy - y) / 80
            p.put(x, yy, mix(hexc(c_light), hexc(c), min(1, t * 2)) if yy < y + 3 else hexc(c))
    for k in range(260):                                       # grass tufts
        x = rnd.randrange(W); y = int(y_of(x)) + rnd.randrange(2, 70)
        if y < H: p.put(x, y, mix(hexc(c), (0, 0, 0), 0.2)); p.put(x, y - 1, mix(hexc(c_light), hexc(c), 0.3))
    if flowers:
        for k in range(50):
            x = rnd.randrange(W); y = int(y_of(x)) + rnd.randrange(4, 70)
            if y < H: p.put(x, y, hexc(rnd.choice(['#ffffff', '#ffe14d', '#f6d8e0', '#c8b8ff'])))

def dew(p, n, y0, y1, seed):
    rnd = random.Random(seed)
    for k in range(n):
        x, y = rnd.randrange(W), rnd.randrange(y0, y1)
        p.put(x, y, (255, 255, 255)); p.blend(x - 1, y, (255, 255, 230), 0.5); p.blend(x + 1, y, (255, 255, 230), 0.5)

def far_birds(p, pts):
    for (x, y) in pts:
        p.put(x, y, (40, 30, 50)); p.put(x - 1, y - 1, (40, 30, 50)); p.put(x + 1, y - 1, (40, 30, 50)); p.put(x - 2, y - 1, (40, 30, 50)); p.put(x + 2, y - 1, (40, 30, 50))

def note(p, x, y, c):
    c = hexc(c)
    p.rect(x, y + 5, 4, 3, c); p.rect(x + 3, y, 1, 7, c); p.rect(x + 3, y, 3, 1, c); p.rect(x + 5, y + 1, 1, 2, c)

# ---- A: the sea of clouds at sunrise ----
def scene_a():
    p = Pic()
    sky_and_sun(p, ['#1d2b5a', '#3b3f7a', '#7a4f8a', '#d9758a', '#f7a978', '#fde2a2'], (150, 92), 13)
    peaks(p, 112, 54, '#6f6a9c', '#e8a6b4', 11, 1.2, peak_x=70)
    peaks(p, 118, 30, '#585a8c', '#d08aa0', 4, 0.9)
    cloud_sea(p, 112, '#fff1e4', '#f2c9c4', '#c9a0b8', 5)
    p.rect(0, 126, W, 60, hexc('#f2c9c4'))
    mist(p, 120, 16, '#ffe6d6', 0.35)
    meadow(p, lambda x: 170 - 22 * math.exp(-((x - 50) / 70) ** 2) + x * 0.04, '#3f7d40', '#a8d27a', seed=2)
    baserri(p, 6, 196, 124, 56, 'right', '#9b2f24')
    apple_tree(p, 200, 206, crown=(-4, 0, 34), branch_to=(140, 152))
    robin(p, 118, 118, sing=True)
    dew(p, 40, 175, 240, 9)
    latxa(p, 150, 204)
    far_birds(p, [(196, 40), (204, 36), (212, 42)])
    note(p, 106, 108, '#ff9f1c'); note(p, 96, 92, '#5ed36b')
    return p

# ---- B: golden rays through the apple tree ----
def scene_b():
    p = Pic()
    sky_and_sun(p, ['#47629a', '#7f8fb8', '#c8b4b8', '#f2cfa0', '#fff0c2'], (196, 70), 11, rays=True, glow=(255, 236, 170))
    peaks(p, 116, 40, '#7d8fb0', '#f2d2b0', 21, 1.0, peak_x=40, sun_left=False)
    mist(p, 104, 18, '#fff4e0', 0.55)
    peaks(p, 130, 26, '#5f7f6a', '#9fbf8a', 8, 0.7, sun_left=False)
    mist(p, 124, 14, '#fff4e0', 0.45)
    meadow(p, lambda x: 182 - x * 0.12, '#4a8a42', '#b8e08a', seed=4)
    baserri(p, 14, 200, 120, 58, 'right', '#3f6f3f')
    apple_tree(p, 196, 214, crown=(0, -2, 36), branch_to=(140, 158))
    robin(p, 118, 124, sing=True)
    dew(p, 70, 185, 240, 3)
    pottoka(p, 154, 182)
    note(p, 106, 112, '#2ec4f0'); note(p, 96, 96, '#ff4d4d'); note(p, 228, 120, '#ffe14d')
    return p

# ---- C: the apple tree in blossom, petals in the morning air ----
def scene_c():
    p = Pic()
    sky_and_sun(p, ['#6fa8d8', '#9cc4e6', '#cfe0ee', '#f4e6d6', '#fdf2dc'], (60, 50), 10, glow=(255, 246, 214))
    peaks(p, 118, 46, '#8ea3c4', '#e6eef6', 31, 1.3, peak_x=176)
    mist(p, 110, 16, '#ffffff', 0.5)
    peaks(p, 132, 24, '#6b9a6e', '#a8cf8f', 17, 0.8)
    meadow(p, lambda x: 176 - 10 * math.sin(x / 60), '#52933f', '#c4e88e', seed=8)
    baserri(p, 4, 194, 126, 56, 'left', '#3f6f3f')
    apple_tree(p, 198, 212, crown=(-2, 0, 36), blossom=True, apples=False, branch_to=(140, 154))
    robin(p, 118, 120, sing=True)
    latxa(p, 60, 208); latxa(p, 98, 214)
    rnd = random.Random(12)
    for k in range(40):                                        # petals drifting
        x, y = rnd.randrange(W), rnd.randrange(40, 230)
        c = hexc(rnd.choice(['#ffffff', '#fbe0ea', '#f6c4d4'])); p.put(x, y, c); p.put(x + 1, y, mix(c, (200, 140, 170), 0.3))
    note(p, 106, 108, '#c26bff'); note(p, 96, 92, '#ff6bc6')
    return p

if __name__ == '__main__':
  out = sys.argv[1]
  for name, f in (('a_sea_of_clouds', scene_a), ('b_golden_rays', scene_b), ('c_blossom', scene_c)):
    im = f().im
    im.save(f'{out}/{name}.png'); im.resize((720, 720), Image.NEAREST).save(f'{out}/{name}_3x.png')
  print('ok')
