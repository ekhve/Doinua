"""K-WASUB 1.2.1b: its 121 factory presets (97 basses, then 24 synths), stepped with PRESETS from A01 (HOOVER DNB). A bass
monosynth: the bass phrase, one note at a time; the synths the standard phrase. usage: plan_kw.py <out.json> [first] [count]"""
import re, sys
sys.path.insert(0, __import__("os").path.dirname(__file__))
from plan2 import make
SRC = sys.argv[4] if len(sys.argv) > 4 else "../K-WASUB_Firmware_for_FM1/source/firmware/src/ksub_data.c"
names = re.findall(r'PR\("([^"]*)"', open(SRC).read())
first = int(sys.argv[2]) if len(sys.argv) > 2 else 0
count = int(sys.argv[3]) if len(sys.argv) > 3 else len(names)
synth0 = names.index("PLUCK BRIGHT")
sounds = []
for i in range(first, min(first + count, len(names))):
    sel = [[-400, "enc", 6, 1]] if i > first else []
    sounds.append({"id": f"kw-{i + 1:03d}", "name": names[i].title(), "eng": "K-WASUB " + ("bass" if i < synth0 else "synth"),
                   "kind": "bass" if i < synth0 else "std", "ch": 0, "select": sel})
setup = [[600, "button", 0, 1], [700, "button", 0, 0]] + ([[1500, "enc", 6, first]] if first else [])
ms = make({"setup": setup, "sounds": sounds, "poly": False}, sys.argv[1])
print(ms, len(sounds))
