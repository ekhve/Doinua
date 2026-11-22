# SPDX-License-Identifier: GPL-3.0-only
"""the txantxangorri, drawn at its own size (26 x 21), facing left into the low sun; three frames"""
REST = [
 "..........oooooo..........",
 "........oobbbbbboo........",
 ".......obbbBBBBBBBo.......",
 "......orrrbbBBBBBBBo......",
 ".....orrrrrbBBBBBBBBo.....",
 "....orrrrrrrgBBBBBBBBo....",
 "...orhrkkrrrrgBBBBBdBBo...",
 "..oyorhkerrrrrgBBBddBBBo..",
 ".yyyorrrrrrrrrgBBBddfBBBo.",
 "..oorhrrrrrrRRgBBddfBBdBo.",
 "...ohrrrrrrRRRwgBdfBBBdBo.",
 "...ohrrrrrRRRwwwBdfBBdBBo.",
 "...orrrrrRRRwwwwBBfBdBBBoo",
 "....orrRRRwwwwwWBBBdBBBdBo",
 ".....oRRwwwwwwWWoBBBddoddo",
 "......owwwwwwWWo.oBBdddo..",
 ".......oWWWWWWo...oddo....",
 "........oooooo.....oo.....",
 "..........l...l...........",
 "..........l...l...........",
 ".........lll.lll.........."]
PAL = {'o': '#2a1c12', 'B': '#7a5434', 'b': '#9a6c42', 'd': '#5a3a22', 'f': '#3f2614', 'R': '#d8501e', 'r': '#f26a2a',
       'h': '#ff9a4c', 'g': '#8a94a8', 'w': '#f2ece0', 'W': '#d6cdbd', 'k': '#111111', 'e': '#ffffff', 'y': '#3a2a1a', 'l': '#6a4a30'}
RIM = '#ffd9a0'

def sing():
    """the beak open, the head lifted a pixel: rows 0-9 up one"""
    rows = [list(r) for r in REST]
    head = [r[:] for r in rows[:10]]
    out = [list('.' * 26)] + [r[:] for r in rows]
    for j in range(10): out[j] = head[j][:]
    out[10] = rows[9][:]                                       # (the neck stays where it was)
    out = out[:21]
    out[6][1] = 'y'; out[6][2] = 'y'; out[8][1] = 'y'; out[8][2] = 'y'; out[7][1] = 'R'; out[7][2] = 'R'   # upper and lower beak, the open mouth
    return [''.join(r) for r in out]

def tilt():
    """the head turned up a little: the eye and the beak a pixel higher, the beak tipped up"""
    rows = [list(r) for r in REST]
    rows[6] = list("...orhrrrrrrrgBBBBBdBBo...")      # the eye leaves row 6 ...
    rows[7] = list("...orhrkerrrrrgBBBddBBBo..")      # ... comes up from row 7 to 6/7, the beak leaves row 7 and 8
    rows[5] = list("..yorrrkkrrrgBBBBBBBBo....")
    rows[6] = list(".yyorhrkerrrrgBBBBBdBBo...")
    rows[7] = list("...orhrrrrrrrrgBBBddBBBo..")
    rows[8] = list("...oorrrrrrrrrgBBBddfBBBo.")
    return [''.join(r) for r in rows]

def shy():
    """a wing up over its eyes: it can't watch"""
    rows = [list(r) for r in REST]
    rows[5] = list("....orrrrrrdBBBBBBBBBo....")
    rows[6] = list("...orhrdddddBBBBBBdBBo....")
    rows[7] = list("..oyorddddffBBBBBddBBBo...")
    rows[8] = list(".yyyorrrdddfgBBBddfBBBo...")
    rows[9] = list("..oorhrrrrdRgBBddfBBdBo...")
    return [''.join(r[:26]) for r in rows]

FRAMES = {'rest': REST, 'sing': sing(), 'tilt': tilt(), 'shy': shy()}

def draw(p, x, y, frame='rest'):
    from draw import hexc
    rows = FRAMES[frame]
    for j, row in enumerate(rows):
        first = True
        for i, c in enumerate(row):
            if c in PAL:
                col = PAL[c]
                if first and c != 'l': col = RIM; first = False      # the low sun's warm edge, on the side facing it
                p.put(x + i, y + j, hexc(col))
    for j, row in enumerate(rows):                              # and along the top of its head
        for i, c in enumerate(row):
            if c in PAL and c != 'l':
                if j < 4: p.put(x + i, y + j, hexc(RIM))
                break
