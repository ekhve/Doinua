#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build an fm1kit app: the host simulator, the FM-1 firmware and its installable package.

  tools/build.py [APP]            APP = a folder in apps/ (default: hello)
  tools/build.py APP --host-only  no JieLi toolchain needed: just the PC build

  JIELI_TOOLCHAIN  JieLi Linux toolchain (default: ~/.jieli/toolchain; tools/setup.sh fetches it)
  AC79_SDK         JieLi AC79 SDK checkout, for three files the package carries (default: ~/fw-AC79_AIoT_SDK)

Outputs in build/APP/: app-host (PC), APP.bin (firmware app), ota.bin (the update loader),
APP.fwsc (the package an FM-1 installs). Every check below stops the build when it fails."""
import argparse
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KIT = ROOT / "fm1kit"
BSP = ROOT / "third_party" / "felucca-bsp"
PRODUCT = "FM-1_1301"                     # the identity the installer sees (device.c FM1KIT_PRODUCT)
CFLAGS = ["-Os", "-ffunction-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]
# The Juno and BS2 engines compute in single-precision float on the AC79 FPU.
# (-mcpu=r3 -mfprev1, the SDK's flags; without them clang emits soft-float calls this link has no library for).
# The FPU is used on the audio path alone, so no other code's float state can be disturbed.
FPU_FLAGS = {src: ["-mcpu=r3", "-mfprev1", "-ffp-contract=off"] for src in ("juno.c", "bs2.c")}
PORTABLE = ["core.c", "gfx.c", "synth.c", "music.c", "engines.c", "store.c", "bank.c", "bs2.c", "juno.c", "fxbus.c", "midi.c", "player.c", "image.c", "seq.c"]
ENGINES = ROOT / "third_party" / "felucca-engines"
APP_BASE = 0x02000120
APP_SLOT = 0x8DFBC                        # the app area's size (fm1pkg.py APP_SLOT)
RAM_LIMIT = 96 * 1024
POOL_LIMIT = 0x54000
LOADER_LOAD = 0x01C0A800                  # where the SPL loads the update loader
LOADER_NAME = b"usb_hid_ota.bin"          # the file name the SPL looks for
SDK_SHA256 = {                            # AC79NN_SDK_V1.2.1_2023-12-13, as Felucca checks them
    "cpu/wl82/tools/uboot.boot": "4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4",
    "cpu/wl82/tools/cfg_tool.bin": "276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b",
    "cpu/wl82/tools/cfg/eq_cfg_hw.bin": "41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d",
}


def run(*cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], capture_output=True, text=True, **kw)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit(f"build: failed: {' '.join(str(c) for c in cmd[:3])} ...")
    return r.stdout


def generate(gen):
    gen.mkdir(parents=True, exist_ok=True)
    if not (gen / "fe_tables.h").exists():
        print(run(sys.executable, ENGINES / "tools" / "gen_engine_tables.py", gen / "fe_tables.h").strip())
    for script, out in (("gen_font.py", "kit_font_data.h"), ("gen_tables.py", "kit_tables.h")):
        target = gen / out
        src = ROOT / "tools" / script
        if not target.exists() or target.stat().st_mtime < src.stat().st_mtime:
            print(run(sys.executable, src, target).strip())


def app_assets(app, out):
    """the app's images (D-028): apps/<app>/assets/*.png -> build/<app>/gen/app_assets.h"""
    target = out / "gen" / "app_assets.h"
    print(run(sys.executable, ROOT / "tools" / "gen_images.py", ROOT / "apps" / app / "assets", target).strip())
    return out / "gen"


def build_host(app, out, gen):
    cc = os.environ.get("CC", "cc")
    srcs = [KIT / "src" / f for f in PORTABLE] + [KIT / "src" / "host.c", ROOT / "apps" / app / "app.c"]
    exe = out / "app-host"
    run(cc, "-O2", "-g", "-Wall", "-Wextra", "-Wno-unused-parameter", "-std=gnu11", f"-I{out / 'gen'}",
        f"-I{KIT / 'include'}", f"-I{KIT / 'src'}", f"-I{gen}", f"-I{ENGINES / 'src'}", *srcs, "-o", exe)
    print(f"host     {exe}")


# ---- device ----

def toolchain():
    tc = Path(os.environ.get("JIELI_TOOLCHAIN", Path.home() / ".jieli" / "toolchain"))
    if not (tc / "pi32v2" / "bin" / "clang").exists():
        raise SystemExit(f"build: no JieLi toolchain in {tc} (tools/setup.sh fetches it; or --host-only)")
    return tc


def cc(tc, *args):
    return run(tc / "pi32v2" / "bin" / "clang", "-target", "pi32v2", *args)


def build_app(app, out, gen, tc, defines=()):
    obj = out / "obj"
    obj.mkdir(parents=True, exist_ok=True)
    inc = [f"-I{out / 'gen'}", f"-I{BSP / 'chip'}", f"-I{BSP / 'boards' / 'fm1'}", f"-I{KIT / 'include'}", f"-I{KIT / 'src'}", f"-I{gen}", f"-I{ENGINES / 'src'}"]
    objs = []
    for s in ("crt0.S", "fm1_vec.S", "fm1_isr.S"):
        o = obj / (s.replace(".S", ".o"))
        cc(tc, "-c", BSP / "chip" / s, "-o", o)
        objs.append(o)
    for src in [KIT / "src" / "device.c"] + [KIT / "src" / f for f in PORTABLE] + [ROOT / "apps" / app / "app.c"]:
        o = obj / (src.stem + ".o")
        cc(tc, *CFLAGS, *FPU_FLAGS.get(src.name, []), f'-DFM1KIT_PRODUCT="{PRODUCT}"', *defines, *inc, "-c", src, "-o", o)
        objs.append(o)
    elf = out / f"{app}.elf"
    run(tc / "pi32v2" / "bin" / "ld", "-T", KIT / "app.ld", *objs, "-o", elf)
    for sec, name in ((".text", "text.bin"), (".data", "data.bin"), (".ram_text", "rt.bin")):
        run(tc / "common" / "bin" / "objcopy", "-O", "binary", "-j", sec, elf, obj / name)
    syms = run(tc / "common" / "bin" / "objdump", "-t", elf)
    sym = lambda n: int(re.search(rf"^([0-9a-f]+) .*\s{n}$", syms, re.M).group(1), 16)
    if sym("_start") != APP_BASE:
        raise SystemExit(f"build: _start at {sym('_start'):#x}, not {APP_BASE:#x}")
    img = bytearray((obj / "text.bin").read_bytes())
    for at, part in sorted([(sym("_rt_load"), (obj / "rt.bin").read_bytes()), (sym("_data_load"), (obj / "data.bin").read_bytes())]):
        if part:
            if at - APP_BASE < len(img):
                raise SystemExit("build: sections overlap")
            img += b"\xff" * (at - APP_BASE - len(img)) + part
    img += b"\xff" * (-len(img) % 4)
    ram = sym("_bss_end") - sym("_data_start")
    pool = sym("_pool_end") - sym("_pool_start")
    checks = [("image", len(img), APP_SLOT), ("RAM .data+.bss", ram, RAM_LIMIT), ("pool", pool, POOL_LIMIT)]
    for name, used, limit in checks:
        ok = used <= limit
        print(f"  {'ok' if ok else 'FAIL':5} {name} {used} B of {limit}")
        if not ok:
            raise SystemExit(f"build: {name} too big")
    binp = out / f"{app}.bin"
    binp.write_bytes(img)
    print(f"app      {binp}  {len(img)} B")
    return binp


def crc16(d, c=0):
    for b in d:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) if c & 0x8000 else c << 1
        c &= 0xFFFF
    return c


def ldr_head(dcrc, a, b, attr, name):
    body = struct.pack("<HIIBBH16s", dcrc, a, b, attr, 0, 0, name)
    return struct.pack("<H", crc16(body)) + body


def build_loader(out, tc):
    """felucca-bsp's update loader, unchanged, wrapped as the SPL expects: outer JLFS head + inner
    head (load address) + LZ4 blocks [clen u32][dlen u32][block]."""
    import lz4.block
    ldr = out / "loader"
    ldr.mkdir(parents=True, exist_ok=True)
    inc = [f"-I{BSP / 'chip'}", f"-I{BSP / 'boards' / 'fm1'}"]
    cc(tc, "-c", BSP / "loader" / "crt0_ldr.S", "-o", ldr / "crt0_ldr.o")
    cc(tc, *CFLAGS, *inc, "-c", BSP / "loader" / "loader.c", "-o", ldr / "loader.o")
    elf = ldr / "loader.elf"
    run(tc / "pi32v2" / "bin" / "ld", "--gc-sections", "-e", "_start", "-T", BSP / "loader" / "loader.ld",
        ldr / "crt0_ldr.o", ldr / "loader.o", "-o", elf)
    run(tc / "common" / "bin" / "objcopy", "-O", "binary", "-j", ".text", elf, ldr / "loader.bin")
    for ln in run(tc / "common" / "bin" / "objdump", "-h", elf).splitlines():   # nothing may run from XIP
        p = ln.split()
        if len(p) > 4 and p[1].startswith(".") and int(p[3], 16) >= 0x02000000 and int(p[2], 16):
            raise SystemExit(f"loader: section {p[1]} at {p[3]} is outside RAM")
    image = (ldr / "loader.bin").read_bytes()
    if not image or len(image) > 0x14000:
        raise SystemExit("loader: image empty or too big")
    blocks = bytearray()
    for i in range(0, len(image), 4096):
        raw = image[i:i + 4096]
        c = lz4.block.compress(raw, store_size=False, mode="high_compression")
        if len(c) > 4096 or lz4.block.decompress(c, uncompressed_size=len(raw)) != raw:
            raise SystemExit(f"loader: block {i // 4096} does not compress below 4096 B")
        blocks += struct.pack("<II", len(c), len(raw)) + c
    inner = ldr_head(crc16(image), len(image), LOADER_LOAD, 0, LOADER_NAME)
    body = inner + bytes(blocks)
    ota = ldr_head(crc16(body), 0x20, len(body), 0x41, LOADER_NAME) + body
    (out / "ota.bin").write_bytes(ota)
    digest = hashlib.sha256(image).hexdigest()
    print(f"loader   {out / 'ota.bin'}  {len(ota)} B (image sha256 {digest[:16]}...)")
    return out / "ota.bin", digest


def package(app, out, binp, ota):
    sdk = Path(os.environ.get("AC79_SDK", Path.home() / "fw-AC79_AIoT_SDK"))
    for rel, want in SDK_SHA256.items():
        f = sdk / rel
        if not f.exists():
            raise SystemExit(f"build: no JieLi AC79 SDK in {sdk} (tools/setup.sh fetches it)")
        if hashlib.sha256(f.read_bytes()).hexdigest() != want:
            raise SystemExit(f"build: {rel} is not the tested SDK file")
    fwsc = out / f"{app}.fwsc"
    run(sys.executable, BSP / "tools" / "fm1pkg.py", binp, ota, fwsc, "--product", PRODUCT, "--sdk", sdk)
    print(f"package  {fwsc}  {fwsc.stat().st_size} B, identity {PRODUCT}")
    return fwsc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("app", nargs="?", default="hello")
    ap.add_argument("--host-only", action="store_true")
    ap.add_argument("--test-fault", type=int, metavar="MS",
                    help="factory only: a firmware that faults MS ms after boot, into build/APP-testfault/ (never shipped)")
    a = ap.parse_args()
    if not (ROOT / "apps" / a.app / "app.c").exists():
        raise SystemExit(f"build: no apps/{a.app}/app.c")
    if not (BSP / "chip").exists():
        raise SystemExit("build: third_party/felucca-bsp is empty (git submodule update --init)")
    out = ROOT / "build" / (a.app + ("-testfault" if a.test_fault else ""))
    gen = ROOT / "build" / "gen"
    out.mkdir(parents=True, exist_ok=True)
    generate(gen)
    app_assets(a.app, out)
    build_host(a.app, out, gen)
    if a.host_only:
        return
    tc = toolchain()
    defines = [f"-DFM1KIT_TEST_FAULT_MS={a.test_fault}u"] if a.test_fault else []
    binp = build_app(a.app, out, gen, tc, defines)
    ota, digest = build_loader(out, tc)
    pinned = (ROOT / "factory" / "loader.sha256").read_text().split()[0] if (ROOT / "factory" / "loader.sha256").exists() else None
    if pinned and pinned != digest:
        raise SystemExit("build: the update loader changed (factory/loader.sha256). It is what keeps an FM-1 "
                         "updatable: a change to it is a reviewed decision, never a side effect.")
    package(a.app, out, binp, ota)


if __name__ == "__main__":
    main()
