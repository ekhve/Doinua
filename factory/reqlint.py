#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: requirement wording (docs/REQUIREMENTS-PLAYBOOK.md section 6).

Checks every row of docs/REQUIREMENTS.md against the rules of the requirements playbook that a machine can apply,
the subset of the owner's requirements method ("Core Authoring Rules" and "Vague and Problematic Terms") that this
project enforces:

  R1   an EARS pattern: "The <subject> shall", "While ..., the <subject> shall", "When ..., ...", "If ..., then ..."
  R3   the subject before "shall" is a subject named in the glossary (docs/REQUIREMENTS.md, Glossary)
  R18  exactly one "shall"                                  (mandatory)
  R19  at most one When / If trigger; "and" / "or" after "shall" is reported as POSSIBLE (two responses?)
  R2   no passive "shall be <verb>ed"
  R7, R8, R9, R10, R16, R26, R34, R35   the term lists, in factory/reqlint-terms.json (one source); a term marked
       conditional is POSSIBLE, the others are VIOLATION; R16 is always POSSIBLE ("flag all; evaluate case by case")
  R17  no "/" or "and/or" in the statement (paths in `backticks` are not part of the wording)

VIOLATION fails the gate unless factory/reqlint-waivers.json waives that rule for that requirement with a reason.
POSSIBLE is printed for the reviewer and does not fail."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TERMS = ROOT / "factory" / "reqlint-terms.json"
REQS = ROOT / "docs" / "REQUIREMENTS.md"
WAIVERS = ROOT / "factory" / "reqlint-waivers.json"


def term_lists():
    """{rule: [(term, conditional)]} from factory/reqlint-terms.json"""
    return {rule: [(term.lower(), bool(cond)) for term, cond in terms] for rule, terms in json.loads(TERMS.read_text())["terms"].items()}


def glossary_subjects():
    subs = []
    text = REQS.read_text()
    sec = text.split("## Glossary", 1)
    if len(sec) == 2:
        for ln in sec[1].splitlines():
            m = re.match(r"\|\s*\*\*(.+?)\*\*\s*\|\s*subject\s*\|", ln)
            if m:
                subs.append(m.group(1).lower())
    return subs


def requirements():
    rows = []
    for ln in REQS.read_text().splitlines():
        m = re.match(r"\|\s*(R-\d{3})\s*\|(.*?)\|", ln)
        if m:
            rows.append((m.group(1), m.group(2).strip()))
    return rows


def has(text, term):
    if term in ("100%", "0%"):
        return term in text
    return re.search(r"(?<![\w-])" + re.escape(term) + r"(?![\w-])", text) is not None


def lint(rid, stmt, lists, subjects):
    found = []      # (rule, state, evidence)
    plain = re.sub(r"`[^`]*`", "`…`", stmt)          # code spans (paths, names) are not wording
    low = plain.lower()
    n_shall = len(re.findall(r"\bshall\b", low))
    if n_shall != 1:
        found.append(("R18", "VIOLATION", f"{n_shall} × 'shall'"))
    if not re.match(r"^(the |each |while |when |if )", low) or "shall" not in low:
        found.append(("R1", "VIOLATION", f"no EARS opening: '{plain[:40]}…'"))
    if low.startswith("if ") and not re.search(r",\s*then\b", low):
        found.append(("R1", "VIOLATION", "'If …' without ', then'"))
    triggers = len(re.findall(r"(?:^|,\s*)(?:when|if)\b", low))
    if triggers > 1:
        found.append(("R19", "VIOLATION", f"{triggers} When/If triggers"))
    if "shall" in low:
        before, after = low.split("shall", 1)
        subj = re.split(r",\s*(?:then\s+)?", before.strip())[-1].strip()
        if subjects and subj not in subjects:
            found.append(("R3", "VIOLATION", f"subject '{subj}' is not a glossary subject"))
        if re.search(r"\b(and|or)\b", after):
            w = re.search(r"\b(and|or)\b", after).group(1)
            found.append(("R19", "POSSIBLE", f"'{w}' after 'shall': one response?"))
        if re.match(r"\s*be\s+\w+ed\b", after):
            found.append(("R2", "VIOLATION", f"passive: 'shall{after[:20]}'"))
    if "/" in plain or "and/or" in low:
        found.append(("R17", "VIOLATION", "'/' in the statement"))
    words = re.sub(r"(^|,\s*)(while|when|if|then)\b", r"\1", low)   # EARS's own keywords are not wording terms
    for rule, terms in lists.items():
        for term, cond in terms:
            if has(words, term):
                state = "POSSIBLE" if cond or rule == "R16" else "VIOLATION"
                found.append((rule, state, f"'{term}'"))
    return found


def main():
    lists = term_lists()
    if not lists:
        raise SystemExit(f"reqlint: no term lists found in {TERMS}")
    subjects = glossary_subjects()
    waivers = json.loads(WAIVERS.read_text()) if WAIVERS.exists() else {}
    fails = possibles = 0
    reqs = requirements()
    for rid, stmt in reqs:
        for rule, state, ev in lint(rid, stmt, lists, subjects):
            w = next((x for x in waivers.get(rid, []) if x["rule"] == rule), None)
            if state == "VIOLATION" and w:
                print(f"  waived  {rid} [{rule}] {ev}: {w['reason']}")
            elif state == "VIOLATION":
                fails += 1
                print(f"  FAIL    {rid} [{rule}] {ev}")
            else:
                possibles += 1
                print(f"  check   {rid} [{rule}] {ev}")
    for rid in waivers:
        if rid not in dict(reqs):
            fails += 1
            print(f"  FAIL    waiver for {rid}, which is not a requirement")
    terms = sum(len(v) for v in lists.values())
    print(f"{'PASS' if not fails else 'FAIL'}: reqlint, {len(reqs)} requirements, {terms} listed terms in "
          f"factory/reqlint-terms.json, {fails} violations, {possibles} to check by eye")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
