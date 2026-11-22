#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Our own pixel art for the concept screens (D-027): python3 apps/hello/assets/make_art.py
Writes the PNGs next to this file; tools/gen_images.py turns every PNG here into C at build time."""
from pathlib import Path
from PIL import Image

HERE = Path(__file__).resolve().parent
PAL = {".": None, "B": (156, 98, 54), "L": (196, 140, 92), "D": (74, 44, 26), "E": (20, 14, 10),
       "W": (250, 250, 250), "R": (224, 72, 60), "G": (120, 120, 128), "Y": (255, 205, 70), "O": (240, 160, 40),
       "C": (244, 248, 255), "S": (210, 222, 240)}

ART = {
    "horse": """
....................DD......
...................DBBD.....
..................DDBBBB....
.................DDBBEBBB...
................DDBBBBBBBB..
...............DDBBBBBBBBBB.
..............DDBBBBB.BBBBB.
.............DDBBBBB...BBB..
............DDBBBBB.........
..DD.......BBBBBBBB.........
.DDDDBBBBBBBBBBBBBB.........
DDD.BBBBBBBBBBBBBBB.........
DD..BBBBBBBBBBBBBBB.........
D...BBBLLLLLLLLBBBB.........
....BBB.BB....BBB.BB........
...BB....BB....BB..BB.......
..BB......BB....BB..BB......
.BB........BB....BB..BB.....
.DD.........DD....DD..DD....
""",
    "fence": """
WW........WW
WWRRRRRRRRWW
WWRRRRRRRRWW
WW........WW
WW........WW
WWRRRRRRRRWW
WWRRRRRRRRWW
WW........WW
WW........WW
WW........WW
GG........GG
""",
    "cloud": """
......CCCC..........
....CCCCCCCC...CC...
..CCCCCCCCCCCCCCCC..
.CCCCCCCCCCCCCCCCCC.
CCCCCCCCCCCCCCCCCCCC
.SSSSSSSSSSSSSSSSSS.
""",
    "star": """
....Y....
....Y....
...YYY...
YYYYOYYYY
.YYOOOYY.
..YYOYY..
..YY.YY..
.YY...YY.
.Y.....Y.
""",
}

for name, art in ART.items():
    rows = [r for r in art.strip("\n").split("\n")]
    w = max(len(r) for r in rows)
    im = Image.new("RGBA", (w, len(rows)), (0, 0, 0, 0))
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            c = PAL[ch]
            if c:
                im.putpixel((x, y), c + (255,))
    im.save(HERE / f"{name}.png")
print("art:", ", ".join(sorted(ART)))
