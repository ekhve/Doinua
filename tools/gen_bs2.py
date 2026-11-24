#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate the selected patch dumps from upstream presets/factory.syx.

Usage: python3 tools/gen_bs2.py PATH/TO/factory.syx [OUT]
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BANK_SHA256 = "60577052fe317a2c83dbe370eadbbe057c9baabdb938f347b59bad0c05ae7111"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bank", type=Path)
    parser.add_argument("output", nargs="?", type=Path,
                        default=ROOT / "fm1kit/src/bs2/bs2_bank.h")
    args = parser.parse_args()
    data = args.bank.read_bytes()
    if hashlib.sha256(data).hexdigest() != BANK_SHA256:
        parser.error("expected presets/factory.syx from bass-something-2 at 4c8a82f")
    browser = json.loads((ROOT / "assets/sounds/browser.json").read_text())
    expected = [row["preset"] for row in browser["sounds"] if row["from"] == "Bass Something 2"]
    picks = [line.split(maxsplit=1) for line in (ROOT / "assets/sounds/bs2.txt").read_text().splitlines()
             if line.strip() and not line.startswith("#")]
    if [name for _, name in picks] != expected:
        parser.error("picks do not match the Sound Browser selection")
    lines = ["/* SPDX-License-Identifier: MPL-2.0",
             " * Copyright (C) 2026 John Brown",
             " * fm1-scales: selected from https://github.com/imijes/bass-something-2",
             " * at 4c8a82f by tools/gen_bs2.py. Do not edit by hand. */",
             "#pragma once", "#include <stdint.h>", "",
             f"#define BS2_N {len(picks)}",
             "static const char *const BS2_NAMES[BS2_N] = {" + ", ".join(json.dumps(n) for _, n in picks) + "};",
             "static const uint8_t BS2_PATCHES[BS2_N][152] = {"]
    for slot, name in picks:
        offset = (int(slot) - 1) * 154
        dump = data[offset:offset + 154]
        if dump[137:153].rstrip(b" \\0").decode("ascii") != name.upper():
            parser.error(f"slot {slot} does not contain {name}")
        payload = dump[1:-1]
        lines.append(f"    {{ /* P{slot} {name} */")
        for start in range(0, len(payload), 16):
            lines.append("        " + ", ".join(f"0x{byte:02x}" for byte in payload[start:start + 16]) + ",")
        lines.append("    },")
    args.output.write_text("\n".join(lines + ["};", ""]))


if __name__ == "__main__":
    main()
