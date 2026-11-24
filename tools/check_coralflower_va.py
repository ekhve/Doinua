#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare the selected bank with the pinned upstream engine, on the host.

Run CoralFlower's tools/build.py --gen-only in its checkout first.
"""
import argparse
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
REVISION = "46790b943bdb5e4143212d83dc495c832eefdbb8"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("upstream", type=Path)
args = parser.parse_args()
upstream = args.upstream.resolve()
head = subprocess.check_output(["git", "-C", upstream, "rev-parse", "HEAD"], text=True).strip()
if head != REVISION:
    parser.error(f"expected CoralFlower {REVISION}, found {head}")
subprocess.run(["git", "-C", upstream, "diff", "--exit-code", "HEAD", "--",
                "firmware", "tests/cr_va_test.c"], check=True)

engine = (upstream / "firmware/src/eng_va.c").read_text()
start = engine.index("static const uint8_t VAP_LUSH[]")
end = engine.index("#define VA_NPRESETS", start)
driver = (upstream / "tests/cr_va_test.c").read_text().split(
    "/* ------------------------------------------------------------- checks --- */", 1
)[0].replace('#include "../firmware/src/va_store.c"', "")
slots = [
    line.split()[0].lstrip("0") or "0"
    for line in (ROOT / "assets/sounds/coralflower-va.txt").read_text().splitlines()
    if line.strip() and not line.startswith("#")
]
probe = r"""
int main(void)
{
    const unsigned slots[] = {SLOTS};
    const unsigned keys[] = {60, 64, 67, 71};
    for (unsigned i = 0; i < sizeof slots / sizeof slots[0]; ++i) {
        unsigned slot = SELECT_SLOT;
        int8_t patch[VA_NP];
        va_preset_patch(slot, patch);
        fwrite(patch, sizeof patch, 1, stdout);
        track_t *track = &trk[0];
        drv_reset(track);
        va_blob_preset(track, slot);
        unsigned voices = VA_PRESETS[slot].mono ? 1u : 4u;
        for (unsigned key = 0; key < voices; ++key) drv_on(track, key, keys[key], 100);
        int heard = 0;
        for (unsigned tick = 0; tick < 8192; ++tick) {
            int32_t audio[CTL];
            if (tick == 2048)
                for (unsigned key = 0; key < voices; ++key) track->v[key].gate = 0;
            drv_tick(track, audio);
            for (unsigned frame = 0; frame < CTL; ++frame) heard |= audio[frame] != 0;
            fwrite(audio, sizeof audio, 1, stdout);
        }
        if (!heard) return 1;
    }
    return ferror(stdout) != 0;
}
"""
with TemporaryDirectory() as directory:
    temp = Path(directory)
    bank = "fm1kit/src/coralflower/va_bank.h"
    (temp / "selected.h").write_text(engine[:start] + f'#include "{bank}"\n' + engine[end:])
    results = []
    for selected in (False, True):
        source, exe = temp / "probe.c", temp / "probe"
        preamble = driver.replace('#include "../firmware/src/eng_va.c"', '#include "selected.h"') if selected else driver
        source.write_text(preamble + probe.replace("SLOTS", ", ".join(slots)).replace(
            "SELECT_SLOT", "i" if selected else "slots[i]"
        ))
        subprocess.run(["cc", "-O2", "-I", ROOT, "-I", upstream / "tests", "-I", upstream / "build/gen",
                        "-I", upstream / "firmware/src", source, "-o", exe, "-lm"], check=True)
        results.append(subprocess.check_output([exe]))
if results[0] != results[1]:
    raise SystemExit("selected VA bank differs from upstream in patch data or rendered audio")
print(f"PASS: {len(slots)} CoralFlower presets match upstream in patch data and host audio")
