"""A general plan: a list of sounds, each with the events that select it, a MIDI channel and a phrase kind.
make(spec, out) writes out.json (events) and out.json.slots.json; spec = {"setup": [events at ms <= 2500],
"sounds": [{"id", "name", "eng", "kind", "ch", "select": [[dt_ms, ...event]]}], "poly": bool}"""
import json, sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])
from phrases import notes, drums, BEAT_MS

SLOT_MS, T0 = 13500, 3000


def make(spec, out):
    ev = list(spec.get("setup", []))
    slots = []
    for k, s in enumerate(spec["sounds"]):
        t = T0 + k * SLOT_MS
        for e in s.get("select", []):
            ev.append([t + e[0]] + e[1:])
        ev.append([t + 700, "shot", s["id"]])
        ns = drums(*s["drums"]) if s["kind"] == "drums" else notes(s["kind"], spec.get("poly", True))
        for st, l, n, v in ns:
            a = t + 1000 + st * BEAT_MS
            ev.append([round(a), "midi", 0x90 | s["ch"], n, v])
            ev.append([round(a + l * BEAT_MS), "midi", 0x80 | s["ch"], n, 0])
        slots.append({k2: s[k2] for k2 in ("id", "name", "eng", "kind")} | {"ms": t + 1000})
    ev.sort(key=lambda e: e[0])
    json.dump(ev, open(out, "w"))
    ms = T0 + len(spec["sounds"]) * SLOT_MS + 500
    json.dump({"ms": ms, "slots": slots}, open(out + ".slots.json", "w"))
    return ms
