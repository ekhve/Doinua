#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: the links between requirements, scenarios, executable checks and decisions hold.

  1. every requirement's "Verified by" names a scenario in features/fm1-scales.feature (or is a note in parentheses,
     for inspection), and its mode agrees with the scenario's tag (A: @auto; H, K: @manual)
  2. every scenario is tagged exactly one of @auto / @manual, and is cited by at least one requirement or is itself an
     approved requirement (@approved:owner or @approved:agent, with an existing @UC-nn)
  3. every @auto scenario is run: by an executable scenario in factory/scenarios/ (its "scenario" field is the title)
     or by a gate named in a @gate:<name> tag
  4. every executable scenario names an @auto scenario that exists
  5. every source (D-nnn, UC-nn, HAZ-nn) a requirement or a scenario cites exists in docs/PLATFORM.md, docs/DESIGN.md,
     docs/USE_CASES.md or docs/HAZARDS.md
  6. docs/TRACEABILITY.md, generated from all of the above (--write), is not stale"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GATES = {"build", "unit", "oracle", "safety", "usb_midi", "cpu"}
fails = []


def fail(msg):
    fails.append(msg)
    print("  FAIL", msg)


# scenarios
scen = {}
tags = []
for ln in (ROOT / "features" / "fm1-scales.feature").read_text().splitlines():
    s = ln.strip()
    if s.startswith("@"):
        tags += s.split()
    elif s.startswith(("Scenario:", "Scenario Outline:")):
        title = s.split(":", 1)[1].strip()
        if title in scen:
            fail(f"scenario title used twice: {title}")
        scen[title] = set(tags)
        tags = []
    elif s and not s.startswith("#"):
        tags = [] if s.startswith("Feature:") else tags

# requirements
reqs = []
for ln in (ROOT / "docs" / "REQUIREMENTS.md").read_text().splitlines():
    m = re.match(r"\|\s*(R-\d{3})\s*\|([^|]*)\|([^|]*)\|\s*([AHKIJ])\s*\|([^|]*)\|([^|]*)\|\s*$", ln)
    if m:
        reqs.append({"id": m.group(1), "source": m.group(3).strip(), "mode": m.group(4),
                     "status": m.group(5).strip(), "by": m.group(6).strip()})
    elif re.match(r"\|\s*R-\d{3}\s*\|", ln):
        fail(f"requirement row not in the 6-column form: {ln[:60]}")
ids = [r["id"] for r in reqs]
for d in sorted({i for i in ids if ids.count(i) > 1}):
    fail(f"requirement id used twice: {d}")

# what exists
known = set()
for doc in ("PLATFORM.md", "DESIGN.md", "USE_CASES.md", "HAZARDS.md"):
    p = ROOT / "docs" / doc
    if p.exists():
        known |= set(re.findall(r"^\|\s*~*(D-\d{3})", p.read_text(), re.M))
        known |= set(re.findall(r"^#+\s*(UC-\d{2})", p.read_text(), re.M))
        known |= set(re.findall(r"^\|\s*(HAZ-\d{2})", p.read_text(), re.M))

cited = set()
for r in reqs:
    if not re.match(r"^(Draft|Approved \((agent|owner)\))$", r["status"]):
        fail(f"{r['id']}: status '{r['status']}' is not Draft, Approved (agent) or Approved (owner)")
    for src in re.findall(r"(?:D|UC|HAZ)-\d+", r["source"]):
        if src not in known:
            fail(f"{r['id']}: source {src} is not written anywhere")
    if r["by"].startswith("*(") or r["by"].startswith("("):
        if r["mode"] not in "IJ":
            fail(f"{r['id']}: mode {r['mode']} needs a scenario, not a note")
        continue
    if r["by"] not in scen:
        fail(f"{r['id']}: verified by '{r['by']}', which is not a scenario in the feature file")
        continue
    cited.add(r["by"])
    t = scen[r["by"]]
    if r["mode"] == "A" and "@auto" not in t:
        fail(f"{r['id']}: mode A but '{r['by']}' is not @auto")
    if r["mode"] in "HK" and "@manual" not in t:
        fail(f"{r['id']}: mode {r['mode']} but '{r['by']}' is not @manual")

impl = {}
for p in sorted((ROOT / "factory" / "scenarios").glob("*.json")):
    t = json.loads(p.read_text()).get("scenario")
    if not t:
        fail(f"{p.name}: no \"scenario\" field naming the feature scenario it implements")
    elif t not in scen:
        fail(f"{p.name}: implements '{t}', which is not a scenario in the feature file")
    elif "@auto" not in scen[t]:
        fail(f"{p.name}: implements '{t}', which is not @auto")
    else:
        impl.setdefault(t, []).append(p.name)

for title, t in scen.items():
    if ("@auto" in t) == ("@manual" in t):
        fail(f"'{title}': tag it exactly one of @auto / @manual")
    # agreed: cited by a requirement, or itself the requirement (the way of working, 2026-11-08): approved
    # (@approved:owner or @approved:agent) and tied to a use case (@UC-nn) that exists
    own = ("@approved:owner" in t or "@approved:agent" in t) and any(re.match(r"@UC-\d+$", x) for x in t)
    if title not in cited and not own:
        fail(f"'{title}': no requirement cites it, and it is not an approved scenario of a use case (behaviour nobody agreed to)")
    gates = {x.split(":", 1)[1] for x in t if x.startswith("@gate:")}
    for g in gates - GATES:
        fail(f"'{title}': unknown gate {g}")
    if "@auto" in t and title not in impl and not (gates & GATES):
        fail(f"'{title}': @auto but nothing runs it (no factory/scenarios/*.json, no @gate)")
    for src in (x[1:] for x in t if re.match(r"@(D|UC)-\d+$", x)):
        if src not in known:
            fail(f"'{title}': tag @{src} is not written anywhere")

# the matrix: docs/TRACEABILITY.md, generated (factory/trace.py --write), checked for staleness in every run
def matrix():
    stmt = {}
    for ln in (ROOT / "docs" / "REQUIREMENTS.md").read_text().splitlines():
        m = re.match(r"\|\s*(R-\d{3})\s*\|([^|]*)\|", ln)
        if m:
            stmt[m.group(1)] = m.group(2).strip()
    haz = []
    hp = ROOT / "docs" / "HAZARDS.md"
    if hp.exists():
        for ln in hp.read_text().splitlines():
            m = re.match(r"\|\s*(HAZ-\d{2})\s*\|([^|]*)\|[^|]*\|[^|]*\|\s*([^|]*)\|[^|]*\|\s*([^|]*)\|", ln)
            if m:
                haz.append((m.group(1), m.group(2).strip(), m.group(3).strip(), m.group(4).strip()))
    def runby(r):
        if r["by"].startswith("*(") or r["by"].startswith("("):
            return "inspection"
        t = scen.get(r["by"], set())
        if "@manual" in t:
            return "manual (owner)"
        parts = [f"`{f}`" for f in impl.get(r["by"], [])]
        parts += [f"gate `{x.split(':', 1)[1]}`" for x in sorted(t) if x.startswith("@gate:")]
        return ", ".join(parts) or "—"
    order = sorted(reqs, key=lambda r: r["id"])
    out = ["# fm1-scales — Traceability", "",
           "Generated by `factory/trace.py --write` from `REQUIREMENTS.md`, `HAZARDS.md`, `PLATFORM.md`,",
           "`features/fm1-scales.feature` and `factory/scenarios/`. Do not edit: `factory/check.sh` fails if it is stale.", "",
           f"{len(order)} requirements · {len(scen)} scenarios · {len(haz)} hazards", "",
           "## Requirements → verification", "",
           "| Req | Statement | Sources | Mode | Status | Verified by | Run by |", "|---|---|---|---|---|---|---|"]
    for r in order:
        out.append(f"| {r['id']} | {stmt.get(r['id'], '')} | {r['source']} | {r['mode']} | {r['status']} | "
                   f"{r['by']} | {runby(r)} |")
    out += ["", "## Hazards → mitigating requirements", "", "| Hazard | Severity | Status | Requirements |", "|---|---|---|---|"]
    for hid, name, sev, status in haz:
        rs = [r["id"] for r in order if hid in r["source"]]
        if not rs:
            fail(f"{hid}: no requirement mitigates it (a requirement's Source must name it)")
        out.append(f"| {hid} {name} | {sev} | {status.split(':')[0]} | {', '.join(rs) or '—'} |")
    out += ["", "## Decisions → requirements", "", "| Decision | Requirements |", "|---|---|"]
    for d in sorted({x for r in order for x in re.findall(r"D-\d{3}", r["source"])}):
        out.append(f"| {d} | {', '.join(r['id'] for r in order if d in r['source'])} |")
    out += ["", "## Scenarios → requirements", "", "| Scenario | Kind | Run by | Requirements |", "|---|---|---|---|"]
    for title, t in scen.items():
        kind = "automated" if "@auto" in t else "manual"
        rb = ", ".join([f"`{f}`" for f in impl.get(title, [])] + [f"gate `{x.split(':', 1)[1]}`" for x in sorted(t) if x.startswith("@gate:")])
        out.append(f"| {title} | {kind} | {rb or 'owner'} | {', '.join(r['id'] for r in order if r['by'] == title)} |")
    return "\n".join(out) + "\n"


tm = ROOT / "docs" / "TRACEABILITY.md"
text = matrix()
if "--write" in sys.argv:
    tm.write_text(text)
    print(f"  wrote {tm.relative_to(ROOT)}")
elif not tm.exists() or tm.read_text() != text:
    fail("docs/TRACEABILITY.md is stale: run factory/trace.py --write and commit it")

auto = sum("@auto" in t for t in scen.values())
print(f"{'PASS' if not fails else 'FAIL'}: trace, {len(reqs)} requirements, {len(scen)} scenarios "
      f"({auto} automated, {len(scen) - auto} manual), {len(fails)} broken links")
sys.exit(1 if fails else 0)
