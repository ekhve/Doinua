#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render each ST-72 factory pick with OberVave's own FM-1 host simulator.

Usage: python3 tools/audition/obervave_refs.py UPSTREAM_CHECKOUT OUTPUT_DIRECTORY
Each WAV contains C4, then a C-major chord, then the release and effects tail.
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("upstream", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
src = args.upstream.resolve()
pin = "a1f660c4439583d33f72941bd184a7e187d11835"
head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=src, text=True).strip()
if head != pin:
    parser.error(f"expected OberVave at {pin}, got {head}")
subprocess.run(["git", "diff", "--exit-code", "HEAD", "--", "firmware", "tests/host.c"],
               cwd=src, check=True)
out = args.output.resolve()
out.mkdir(exist_ok=True, parents=True)
manifest = Path(__file__).resolve().parents[2] / "assets/sounds/obervave.txt"
with tempfile.TemporaryDirectory() as temp:
    exe = Path(temp) / "obervave"
    script = Path(temp) / "keys.txt"
    subprocess.run(["cc", "-O2", "-DOBV_HOST", "-Ifirmware/src", "-Ifirmware/gen",
                    "firmware/src/obervave.c", "-o", exe], cwd=src, check=True)
    for line in manifest.read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        slot, name = line.split(maxsplit=1)
        # Encoder 6 selects the program from slot 1 on a fresh boot.
        script.write_text(f"3000 enc 6 {int(slot) - 1}\n3500 keys 0x80\n4500 keys 0\n"
                          "5500 keys 0x4880\n7000 keys 0\n9500 end\n")
        prefix = Path(temp) / f"{slot}-{name.lower().replace(' ', '-')}"
        subprocess.run([exe, prefix, script], cwd=temp, check=True)
        shutil.copyfile(prefix.with_suffix(".wav"), out / (prefix.name + ".wav"))
