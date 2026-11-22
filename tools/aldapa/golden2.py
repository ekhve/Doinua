# SPDX-License-Identifier: GPL-3.0-only
"""Aldapeko Sagarra, second pass: the golden afternoon composed as a photograph. 240 x 240, our own pixel art.
Rule of thirds (the robin on the upper-right third point, the horizon on the lower third), backlight with rim light and
long shadows, atmospheric depth, leading lines up to the tree, a soft out-of-focus foreground frame, warm light against
cool shade, a vignette."""
import math, random, sys
from PIL import Image
from draw import Pic, hexc, mix, BAYER, W, H, baserri
from golden import latxa, ROBIN, RP

SUN = (64, 140)                                                # setting behind the far ridge, on the left third
WARM, COOL = (255, 204, 120), (90, 96, 150)

def slope_y(x): return 226 - x * 0.28                         # the hillside, rising to the right (a leading line)

def sky(p):
    p.grad(0, 150, ['#2f3f78', '#5a5f96', '#a07a9c', '#e09a84', '#f8bf78', '#ffdc96'])
    p.rect(0, 150, W, H - 150, hexc('#ffdc96'))                # (the horizon's colour under everything below)
    sx, sy = SUN
    for y in range(0, 190):                                    # the sun's glow: wide and warm, dithered
        for x in range(W):
            d = math.hypot((x - sx) * 0.75, y - sy)
            if d < 150:
                a = (1 - d / 150) ** 2.4
                if a * 16 > BAYER[y % 4][x % 4] * 0.5: p.blend(x, y, (255, 222, 150), min(0.9, a * 1.1))
    for k in range(9):                                         # crepuscular rays fanning up from behind the ridge
        ang = -math.pi * 0.08 - k * math.pi * 0.105
        for r in range(20, 260, 1):
            for w in range(-2 - r // 40, 3 + r // 40):
                x = int(sx + math.cos(ang) * r - math.sin(ang) * w); y = int(sy + math.sin(ang) * r + math.cos(ang) * w)
                if 0 <= x < W and 0 <= y < 150 and (x + y) % 2 == 0: p.blend(x, y, (255, 236, 190), 0.13 * (1 - r / 260))

def sun_disc(p):
    sx, sy = SUN
    p.disc(sx, sy, 16, (255, 236, 190), 0.5); p.disc(sx, sy, 12, (255, 244, 214)); p.disc(sx, sy, 9, (255, 252, 236))

def lit_clouds(p):
    """sunset clouds in the top left: soft banks with flat undersides; violet on top, pink below, and a burning gold rim
    along the bottom edge where the sun, low on the left, lights them from beneath"""
    rnd = random.Random(30)
    mask = [[0] * W for _ in range(H)]
    banks = ((40, 46, 70, 6), (12, 24, 44, 5), (96, 30, 40, 4), (70, 64, 52, 4), (6, 70, 30, 3))
    for (bx, by, bw, bh) in banks:                             # each bank: overlapping wide ellipses, flat along the bottom
        for k in range(10):
            ex = bx + rnd.uniform(-bw / 2, bw / 2); rx = rnd.uniform(bw * 0.25, bw * 0.45); ry = rnd.uniform(bh * 0.8, bh * 1.6)
            ey = by - rnd.uniform(0, bh)
            for y in range(int(ey - ry), int(by) + 1):
                for x in range(int(ex - rx), int(ex + rx) + 1):
                    if 0 <= x < W and 0 <= y < H and ((x - ex) / rx) ** 2 + ((y - ey) / ry) ** 2 <= 1: mask[y][x] = 1
    for x in range(W):
        for y in range(H):
            if not mask[y][x]: continue
            below = 0                                          # how far above the cloud's underside this pixel is
            while y + below + 1 < H and mask[y + below + 1][x]: below += 1
            near = max(0, 1 - math.hypot(x - SUN[0], y - SUN[1]) / 180)
            if below == 0: c = mix(hexc('#ffc070'), hexc('#fff0b0'), near)
            elif below == 1: c = mix(hexc('#ff9a6a'), hexc('#ffd08a'), near)
            elif below < 4: c = mix(hexc('#d06a84'), hexc('#ff9a7a'), near)
            else:
                c = mix(hexc('#6a5a8e'), hexc('#9a7098'), near)
                if (x + y) % 2 == 0 and below < 6: c = mix(c, hexc('#d06a84'), 0.5)   # dithered into the pink
            p.put(x, y, c)

def cloud_streaks(p):
    """long sunset streaks: violet above, their undersides burning gold and pink towards the sun"""
    rnd = random.Random(8)
    for (y, x0, x1, th) in ((30, 90, 250, 4), (44, 20, 170, 3), (58, 130, 250, 3), (70, -10, 110, 2), (86, 150, 250, 2), (24, -10, 60, 2)):
        for x in range(int(x0), int(x1)):
            wob = math.sin(x / 13 + y) * 1.5 + math.sin(x / 5) * 0.6
            t = abs(x - (x0 + x1) / 2) / ((x1 - x0) / 2)
            h = max(1, int(th * (1 - t * t) + 0.5))
            near = max(0, 1 - math.hypot(x - SUN[0], y - SUN[1]) / 200)
            for j in range(h + 1):
                yy = int(y + wob + j)
                c = mix(hexc('#8a6f9c'), hexc('#f2a088'), near) if j < h else mix(hexc('#ffb070'), hexc('#fff0b8'), near)
                if 0 <= yy < 150: p.put(x, yy, c)

def ridge(p, base, height, col, rim, seed, jag, until, sun_rim=True):
    rnd = random.Random(seed); raw = []; y = base - height * 0.5; v = 0
    for x in range(W + 14):
        v = v * 0.86 + rnd.uniform(-1, 1) * jag; y = min(base - 3, max(base - height, y + v)); raw.append(y)
    pts = [sum(raw[x:x + 13]) / 13 for x in range(W)]
    for x in range(W):
        y = int(pts[x])
        for yy in range(y, until): p.put(x, yy, hexc(col))
        if sun_rim:                                            # the crest catches the light, brighter nearer the sun
            near = max(0, 1 - abs(x - SUN[0]) / 120)
            p.put(x, y, mix(hexc(col), hexc(rim), 0.4 + 0.6 * near))
    return pts

def glow_mist(p, y, h, a):
    for yy in range(y, y + h):
        for x in range(W):
            near = max(0, 1 - abs(x - SUN[0]) / 170)
            k = a * (1 - abs(yy - y - h / 2) / (h / 2 + 1)) * (0.5 + near)
            if (x // 2 + yy) % 2 == 0 or k > 0.3: p.blend(x, yy, mix((214, 190, 210), (255, 226, 170), near), min(0.85, k))

def mid_valley(p):
    """the valley: misty blue-green hills, the baserri on its knoll in the last sun, the dark wood below"""
    ridge(p, 152, 16, '#8a86a8', '#ffd8a0', 3, 1.0, 158)       # far: pale, hazy (atmospheric perspective)
    glow_mist(p, 146, 12, 0.7)
    ridge(p, 166, 16, '#5f7488', '#f2c890', 9, 0.8, 176)       # middle: cooler, bluer
    glow_mist(p, 162, 10, 0.5)
    ridge(p, 178, 14, '#4a6a4e', '#e8c070', 14, 0.6, 205)      # the baserri's knoll
    rnd = random.Random(5)                                     # the wood at the foot of the hill: a dark band, a few lit crowns
    for k in range(60):
        x = rnd.uniform(-8, 150); y = 196 + rnd.uniform(-4, 14); r = rnd.uniform(5, 9)
        p.disc(x + 1, y + 1, r, hexc('#1f3324')); p.disc(x, y, r - 1, hexc('#2c4a32'))
        if x < 110: p.disc(x - r * 0.4, y - r * 0.45, r * 0.4, hexc('#8a9a50'))

def hillside(p):
    rnd = random.Random(2)
    for x in range(W):
        y0 = int(slope_y(x))
        for y in range(max(0, y0), H):
            d = y - y0
            c = mix(hexc('#d8c870'), hexc('#6aa04a'), min(1, d / 5)) if d < 5 else mix(hexc('#5a9444'), hexc('#2f5a34'), min(1, d / 80))
            p.put(x, y, c)
    for k in range(600):                                       # grass blades, rimmed gold against the sun
        x = rnd.randrange(W); y = int(slope_y(x)) + rnd.randrange(2, 90)
        if y >= H: continue
        h = rnd.randrange(2, 6)
        for j in range(h): p.put(x, y - j, hexc('#f2d878') if j == h - 1 and k % 2 else hexc('#3a6a34'))

def stone_wall(p):
    """a dry-stone wall from the bottom left up towards the tree: the leading line"""
    for i in range(80):
        x = 2 + i * 2; y = int(slope_y(x)) + 30 - i // 3
        top = mix(hexc('#c8b8a0'), hexc('#ffe0a8'), max(0, 1 - i / 60))
        p.rect(x, y, 2, 4, hexc('#8a8478') if i % 3 else hexc('#a49c8e')); p.put(x, y, top); p.put(x, y + 4, hexc('#4a4a48'))

def apple_tree(p, tx):
    base = int(slope_y(tx)) + 4
    T, TD, RIM = hexc('#4a3020'), hexc('#2e1c12'), hexc('#ffc070')
    trunk = []
    for i in range(52):
        y = base - i; x = tx - i * 0.22 + math.sin(i / 7) * 3
        w = 12 - i // 7; trunk.append((x, y))
        for k in range(w): p.put(int(x - w / 2 + k), y, RIM if k == 0 else (TD if k > w - 4 else T))
        if i % 8 == 3: p.put(int(x) + 1, y, TD)
    for (dy, l, ang) in ((46, 18, -0.6), (50, 14, -1.9)):
        x0, y0 = trunk[dy]
        for j in range(l):
            x = x0 + math.cos(ang) * j; y = y0 + math.sin(ang) * j
            for k in range(max(2, 4 - j // 8)): p.put(int(x) + k, int(y), RIM if k == 0 else T)
    rnd = random.Random(11)
    cx, cy, r = tx - 12, base - 62, 40
    for k in range(110):                                       # the crown: deep shade inside, a gold rim on the sun side
        a = rnd.uniform(0, math.pi * 2); d = rnd.uniform(0, r) ** 0.9
        x = cx + math.cos(a) * d * 1.15; y = cy + math.sin(a) * d * 0.62; rr = rnd.uniform(5, 9)
        sun = (x - cx) < -10 or (y - cy) < -12
        p.disc(x + 1, y + 2, rr, hexc('#1f3e26')); p.disc(x, y, rr - 1, hexc('#356a34'))
        if sun and rnd.random() < 0.6: p.disc(x - rr * 0.55, y - rr * 0.5, rr * 0.3, hexc('#f0c860'))
        else: p.disc(x - rr * 0.4, y - rr * 0.35, rr * 0.4, hexc('#5a8a3a'))
    for k in range(18):                                        # cider apples: mostly red, some yellow, some blushed
        x = cx + rnd.uniform(-r, r) * 1.0; y = cy + rnd.uniform(-r * 0.1, r) * 0.55; kind = [0, 0, 1, 2, 0, 2][k % 6]
        p.disc(x, y, 2.2, hexc(['#c42a1e', '#e8c23a', '#e0a83a'][kind]))
        if kind == 2: p.put(int(x) + 1, int(y) - 1, hexc('#d0402a'))
        p.put(int(x) - 1, int(y) - 1, hexc('#fff0c0'))
    for k in range(18):                                        # leaves catching the light at the crown's edge, and turning
        a = rnd.uniform(math.pi * 0.6, math.pi * 1.6); x = cx + math.cos(a) * r * 1.12; y = cy + math.sin(a) * r * 0.66
        p.put(int(x), int(y), hexc('#ffd878')); p.put(int(x) + 1, int(y) + 1, hexc('#d8962e'))
    tipx, tipy = cx - 14, cy - 24                              # the highest branch, the bird on its tip at the crown's top
    x0, y0 = cx - 4, cy - 12
    for i in range(14):
        t = i / 13; x = x0 + (tipx - x0) * t + math.sin(t * 3) * 1.5; y = y0 + (tipy - y0) * t
        p.put(int(x), int(y), T); p.put(int(x) + 1, int(y), T); p.put(int(x) - 1, int(y), RIM)
    for i in range(80):                                        # its long shadow up the slope, cool
        x = tx + 4 + i; y = int(slope_y(x)) + 6
        for j in range(5): p.blend(x, y + j, (40, 60, 90), 0.4 * (1 - i / 80))
    return tipx, tipy

def robin(p, tipx, tipy, s=1.5):
    """1.5 times the 20 x 16 drawing (each target pixel takes the nearest source pixel), backlit"""
    w, h = int(20 * s), int(16 * s)
    x = tipx - int(8 * s); y = tipy - h + 1
    for ty in range(h):
        row = ROBIN[min(15, int(ty / s))]
        first = True
        for tx in range(w):
            c = row[min(19, int(tx / s))] if int(tx / s) < len(row) else '.'
            if c in RP:
                col = hexc(RP[c])
                if first: col = hexc('#ffe0a0') if c == 'o' else hexc('#ffb060'); first = False   # the rim the low sun lights
                p.put(x + tx, y + ty, col)
    p.put(x + int(4.5 * s), y + int(5.5 * s), hexc('#ffffff'))
    p.rect(x + int(6 * s), y + int(7 * s), 2, 1, hexc('#e0503a'))
    for i in range(0):                                         # (the branch now runs under its toes)
        p.put(tipx - 4 + i, tipy + (1 if i > 9 else 0), hexc('#4a3020')); p.put(tipx - 4 + i, tipy - 1 + (1 if i > 9 else 0), hexc('#ffc070') if i < 8 else hexc('#4a3020'))

def sheep_rim_body(p, x, y, up=False):
    """a latxa without its shadow: grazing, or with its head up, looking out"""
    from golden import latxa as sheep
    sheep(p, x, y, 'up' if up else 'down')
    for dx in range(4, 28):                                    # backlit wool: a glowing edge along the top
        for yy in range(y - 2, y + 16):
            if p.get(x + dx, yy) in (hexc('#ece2c8'), hexc('#c4b48e'), hexc('#fff2d0')):
                p.put(x + dx, yy, hexc('#fff4c8')); break

def sheep_rim(p, x, y):
    latxa(p, x, y)
    for dx in range(4, 28):                                    # backlit wool: a glowing edge along the top
        for yy in range(y - 2, y + 16):
            if p.get(x + dx, yy) in (hexc('#ece2c8'), hexc('#c4b48e'), hexc('#fff2d0')):
                p.put(x + dx, yy, hexc('#fff4c8')); break
    for i in range(36):                                        # its long shadow
        for j in range(3): p.blend(x + 14 + i, int(y + 22 - i * 0.42) + j, (40, 60, 90), 0.35 * (1 - i / 36))

def crown_leaves_over(p, tipx, tipy):
    """a few leaves in front of the branch round its feet: it sits in the tree, not on a pole"""
    for (dx, dy, c) in ((-6, 2, '#356a34'), (6, 3, '#356a34'), (12, 1, '#5a8a3a'), (-12, 4, '#356a34'), (0, 5, '#2c5a2c'), (16, 5, '#f0c860')):
        p.disc(tipx + dx, tipy + dy, 3, hexc(c)); p.put(tipx + dx - 1, tipy + dy - 2, hexc('#f0c860'))

def notes(p):
    def note(x, y, c):
        c = hexc(c)
        p.rect(x, y + 5, 4, 3, c); p.rect(x + 3, y, 1, 7, c); p.rect(x + 3, y, 3, 1, c); p.rect(x + 5, y + 1, 1, 2, c)
    note(150, 66, '#ffe14d'); note(136, 54, '#ff9f1c'); note(150, 42, '#ff6bc6')

def foreground_frame(p):
    """out-of-focus grass across the bottom, dark against the light, and warm bokeh where the sun catches seed heads"""
    rnd = random.Random(21)
    for k in range(70):
        x = rnd.uniform(-4, 244); h = rnd.uniform(10, 34); lean = rnd.uniform(-5, 5); w = rnd.uniform(2, 4)
        for j in range(int(h)):
            t = j / h; xx = x + lean * t * t
            for q in range(int(w * (1 - t) + 1)):
                p.blend(int(xx + q), H - 1 - j, (28, 40, 26), 0.75)
    for k in range(14):
        x = rnd.uniform(0, 240); y = rnd.uniform(196, 236); r = rnd.uniform(2, 5)
        p.disc(x, y, r, (255, 214, 140), 0.25); p.disc(x, y, r * 0.6, (255, 236, 190), 0.25)

def flare(p):
    sx, sy = SUN
    for (t, r, a) in ((0.55, 5, 0.18), (0.8, 3, 0.22), (1.25, 8, 0.12)):   # a faint lens flare towards the frame's middle
        x = sx + (150 - sx) * t; y = sy + (90 - sy) * t
        p.disc(x, y, r, (255, 230, 190), a)

def vignette(p):
    for y in range(H):
        for x in range(W):
            d = math.hypot((x - 120) / 120, (y - 120) / 120)
            if d > 0.75: p.blend(x, y, (20, 14, 30), min(0.45, (d - 0.75) * 0.9))

def scene():
    p = Pic()
    sky(p)
    sun_disc(p)
    cloud_streaks(p)
    lit_clouds(p)
    mid_valley(p)
    baserri(p, 88, 214, 64, 34, 'right', '#7a2e24', loft='#6b4a30')   # set into the hill: the slope hides its lower half
    hillside(p)
    stone_wall(p)
    sheep_rim(p, 156, int(slope_y(156)) + 2)
    sheep_rim(p, 182, int(slope_y(182)) + 6)
    tip = apple_tree(p, 214)
    import robin2
    robin2.draw(p, tip[0] - 12, tip[1] - 20)
    crown_leaves_over(p, *tip)
    notes(p)
    flare(p)
    foreground_frame(p)
    vignette(p)
    return p

if __name__ == '__main__':
    im = scene().im
    im.save(sys.argv[1] + '/golden2.png'); im.resize((720, 720), Image.NEAREST).save(sys.argv[1] + '/golden2_3x.png')
    print('ok')
