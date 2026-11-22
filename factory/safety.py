#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate: the things that keep an FM-1 recoverable, which no change may break.

  factory/safety.py [APP]     (after tools/build.py APP)

  1. the update loader is byte-identical to the pinned felucca-bsp one (factory/loader.sha256)
  2. in the emulator, with a USB cable: the firmware enumerates, answers the installer's handshake,
     and leaves for update mode on the UBOOT soft key and on OCT- + OCT+ held 5 s (not sooner) (the emulator has no ROM update mode: it
     reboots there, so the check is that the firmware let go of USB in time)
  2b. the boot guard: a test build that faults at boot enters update mode on its third boot (R-036)
  3. the package identity is in the image, so installers recognise it
  4. apps only talk to the hardware through fm1kit.h: no BSP headers, no register addresses
  5. the image fits the app area, with room to spare"""
import hashlib
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
app = sys.argv[1] if len(sys.argv) > 1 else "hello"
out = ROOT / "build" / app
fails = []


def gate(ok, what):
    print(f"  {'ok  ' if ok else 'FAIL'} {what}")
    if not ok:
        fails.append(what)


pinned = (ROOT / "factory" / "loader.sha256").read_text().split()[0]
got = hashlib.sha256((out / "loader" / "loader.bin").read_bytes()).hexdigest()
gate(got == pinned, "update loader identical to the pinned felucca-bsp loader")

EMU = ROOT / "third_party" / "fm1-emu" / "target" / "release" / "fm1-live"


def usb_session(send, ms, keys=None):
    """the firmware in the emulator with a USB cable to a computer that sends `send` (MS:HEX;...), and the
    panel played by `keys` (the scenario key syntax)"""
    env = dict(os.environ, FM1_USB_SEND=send)
    if keys:
        env["FM1_BENCH_KEYS"] = keys
    r = subprocess.run([str(EMU), "--fwsc", str(out / f"{app}.fwsc"), "--usb", "--shot", str(ms)],
                       env=env, capture_output=True, text=True, cwd=ROOT / "work" / "usb")
    return r.stdout + r.stderr


(ROOT / "work" / "usb" / "work").mkdir(parents=True, exist_ok=True)
log = usb_session("1500:F0003245000000407FF7", 2500)              # the installer's device query
gate("usb: Configured" in log, "firmware enumerates on USB (USB-MIDI)")
gate(re.search(r"usb in:\s+f0 00 32 45", log) is not None, "firmware answers the installer's handshake (the update entry)")
log = usb_session("1500:F02224357DF7", 2500)                      # the UBOOT soft key
m = re.search(r"\[\s*(\d+) ms\] usb: Detached", log)
gate(m is not None and 1500 <= int(m.group(1)) <= 1700, "the UBOOT soft key takes the firmware off USB into update mode")
log = usb_session("", 6000, "1000:OCTDN,1000:OCTUP,3000:OCTDN^,3000:OCTUP^")   # both OCT held 2 s: nothing
gate("usb: Detached" not in log, "OCT- + OCT+ held 2 s does not enter update mode")
log = usb_session("", 7000, "1000:OCTDN,1000:OCTUP")                             # held on: update mode at 5 s
m = re.search(r"\[\s*(\d+) ms\] usb: Detached", log)
gate(m is not None and 5900 <= int(m.group(1)) <= 6300, "OCT- + OCT+ held 5 s enters update mode (5 s after the press)")

# the boot guard (R-036, #9): a test build that faults 1 s after boot must end in update mode on its third boot; the
# BSP's UBOOT entry leaves "usb_update_mode" in the boot mailbox (0x01C7FD80), which survives the emulator's chip reset
subprocess.run([sys.executable, str(ROOT / "tools" / "build.py"), app, "--test-fault", "1000"], check=True,
               stdout=subprocess.DEVNULL)


def boots(fwsc, ms):
    r = subprocess.run([str(EMU), "--fwsc", str(fwsc), "--shot", str(ms)], capture_output=True, text=True,
                       cwd=ROOT / "work" / "usb", env=dict(os.environ, FM1_RESET_TRACE="1", FM1_DUMP="01c7fd80"))
    log = r.stdout + r.stderr
    resets = len(re.findall(r"chip reset at \d+ ms", log))
    m = re.search(r"dump 0x1c7fd80: ((?:[0-9a-f]{8} ?){4})", log)
    words = m.group(1).split() if m else []
    mailbox = b"".join(int(w, 16).to_bytes(4, "little") for w in words).split(b"\0")[0].decode(errors="replace")
    return resets, mailbox


resets, mailbox = boots(ROOT / "build" / f"{app}-testfault" / f"{app}.fwsc", 9000)
gate(mailbox == "usb_update_mode" and resets >= 3,
     f"a firmware that crashes at boot enters update mode on its third boot ({resets} resets, mailbox '{mailbox}')")
resets, mailbox = boots(out / f"{app}.fwsc", 9000)
gate(resets == 0 and mailbox == "", f"the normal build neither resets nor asks for update mode ({resets} resets)")

build_py = (ROOT / "tools" / "build.py").read_text()
product = re.search(r'^PRODUCT = "([^"]+)"', build_py, re.M).group(1)
img = (out / f"{app}.bin").read_bytes()
gate(product.encode() in img, f"package identity {product} is in the image")

for src in sorted((ROOT / "apps").rglob("*.[ch]")):
    text = src.read_text()
    inc = re.findall(r'#include\s+[<"]([^>"]+)[>"]', text)
    bad = [i for i in inc if i.startswith("fm1_") or i in ("libc.c", "usb.c", "ota.c")]
    gate(not bad, f"{src.relative_to(ROOT)} uses only fm1kit.h (no BSP headers)")
    regs = re.findall(r"\(\s*volatile[^)]*\*\s*\)\s*0x[0-9a-fA-F]+", text)
    gate(not regs, f"{src.relative_to(ROOT)} touches no registers")

gate(len(img) <= 0x8DFBC * 9 // 10, f"image {len(img)} B within 90% of the app area")
print(f"{'PASS' if not fails else 'FAIL'}: safety, {len(fails)} failed")
sys.exit(1 if fails else 0)
