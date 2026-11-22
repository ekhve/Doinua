#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""An app's images for fm1kit (D-028): every PNG in apps/<app>/assets/ becomes IMG_<name> (a kit_image_t).
  tools/gen_images.py ASSETS_DIR OUT.h
And the user kit (ST-35, D-056): kits/user/NN-name.wav (or the folder FM1_USER_KIT names) becomes USER_KIT, each
sound on the key its number names (01 is F3, 27 G5), named the rest of its file name ("01-kick1.wav" is kick1),
mono, 22 050 Hz, 16 bits, a second at most each. Sound files without a LICENSE file (LICENSE, LICENSE.txt or
LICENSE.md: whose sounds they are and on what terms) stop the build.
Palette-indexed (at most 255 colours: pixel art, or quantise first) and LZ coded (D-089, fm1kit.h KIT_IMAGE_LZ):
literal runs and copies of the last 4096 indices, row after row, about half the size of run-length pairs. Pixels with alpha below 128 use index 0, the transparent one; an image without such pixels
has no transparent index."""
import os
import re
import struct
import sys
import wave
from pathlib import Path

from PIL import Image


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def convert(path):
    im = Image.open(path).convert("RGBA")
    w, h = im.size
    px = list(im.get_flattened_data() if hasattr(im, "get_flattened_data") else im.getdata())
    clear = any(a < 128 for *_, a in px)
    pal, index = ([None] if clear else []), {}
    idx = []
    for r, g, b, a in px:
        if a < 128:
            idx.append(0)
            continue
        c = rgb565(r, g, b)
        if c not in index:
            index[c] = len(pal)
            pal.append(c)
        idx.append(index[c])
    if len(pal) > 255:
        raise SystemExit(f"gen_images: {path.name} has {len(pal)} colours (255 at most: quantise it)")
    pal = [0 if c is None else c for c in pal]
    return w, h, pal, lz(idx), 0 if clear else -1


LZ_WIN = 4096


def lz(d):
    """KIT_IMAGE_LZ: b < 0x80: b + 1 literal indices; b >= 0x80: (b & 0x7F) + 3 indices copied from `off` back
    (2 bytes, 1..LZ_WIN). Greedy, the longest match among the last 64 at the same 3 indices"""
    out, lit, heads, i, n = [], [], {}, 0, len(d)

    def flush():
        while lit:
            ch = lit[:128]
            out.append(len(ch) - 1)
            out.extend(ch)
            del lit[:128]
    while i < n:
        best, boff = 0, 0
        key = tuple(d[i:i + 3])
        if len(key) == 3:
            for j in reversed(heads.get(key, [])[-64:]):
                if i - j > LZ_WIN:
                    break
                k = 0
                while i + k < n and d[j + k] == d[i + k] and k < 130:
                    k += 1
                if k > best:
                    best, boff = k, i - j
        if best >= 3:
            flush()
            out += [0x80 | (best - 3), boff & 255, boff >> 8]
            for k in range(best):
                if i + k + 3 <= n:
                    heads.setdefault(tuple(d[i + k:i + k + 3]), []).append(i + k)
            i += best
        else:
            lit.append(d[i])
            if len(key) == 3:
                heads.setdefault(key, []).append(i)
            i += 1
    flush()
    return out


USER_RATE = 22050
USER_TOTAL = 240 * 1024                     # bytes in all: the firmware's room


def read_wav(path):
    """mono floats at USER_RATE, at most a second"""
    with wave.open(str(path)) as w:
        ch, width, rate, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if width == 1:
        xs = [(b - 128) / 128 for b in raw]
    elif width == 2:
        xs = [v / 32768 for v in struct.unpack("<%dh" % (len(raw) // 2), raw)]
    elif width == 3:
        xs = [int.from_bytes(raw[i:i + 3], "little", signed=True) / 8388608 for i in range(0, len(raw), 3)]
    else:
        raise SystemExit(f"gen_images: {path.name}: {8 * width}-bit sound (8, 16 or 24 bits, PCM)")
    mono = [sum(xs[i:i + ch]) / ch for i in range(0, len(xs), ch)]
    out, step, pos = [], rate / USER_RATE, 0.0
    while pos < len(mono) - 1 and len(out) < USER_RATE:     # linear resampling
        i = int(pos)
        f = pos - i
        out.append(mono[i] * (1 - f) + mono[i + 1] * f)
        pos += step
    return out


def user_kit(lines):
    kdir = Path(os.environ.get("FM1_USER_KIT", Path(__file__).resolve().parent.parent / "kits" / "user"))
    if not kdir.is_dir():
        return 0
    files = sorted(p for p in kdir.iterdir() if p.suffix.lower() == ".wav")
    if not files:
        return 0
    if not any((kdir / n).is_file() for n in ("LICENSE", "LICENSE.txt", "LICENSE.md")):
        raise SystemExit(f"gen_images: {kdir}: {len(files)} sound files and no LICENSE file. Add one saying whose the "
                         "sounds are and on what terms they may be used (R-138)")
    pads, total = [], 0
    for p in files:
        m = re.match(r"^(\d\d)-(.+)$", p.stem)
        if not m or not 1 <= int(m.group(1)) <= 27:
            raise SystemExit(f"gen_images: {p.name}: name it NN-name.wav, NN from 01 (F3) to 27 (G5)")
        pad, name = int(m.group(1)) - 1, m.group(2)[:10]
        if any(q[0] == pad for q in pads):
            raise SystemExit(f"gen_images: two sound files for key {pad + 1:02d}")
        pcm = [max(-32767, min(32767, round(v * 32767))) for v in read_wav(p)]
        total += 2 * len(pcm)
        if total > USER_TOTAL:
            raise SystemExit(f"gen_images: kits/user: over {USER_TOTAL // 1024} KB of sound in all (shorten some)")
        pads.append((pad, name, pcm))
    for pad, name, pcm in pads:
        lines.append(f"static const int16_t USER_KIT_{pad}[{len(pcm)}] = {{ {', '.join(map(str, pcm))} }};")
    lines.append("static const kit_sample_t USER_KIT[] = {")
    for pad, name, pcm in pads:
        lines.append(f'    {{ {pad}, "{name}", USER_KIT_{pad}, {len(pcm)} }},')
    lines.append("};")
    lines.append(f"#define USER_KIT_N {len(pads)}")
    lines.append("")
    return total


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    lines = ["/* generated by tools/gen_images.py from %s: do not edit */" % src.as_posix().split("/apps/")[-1],
             "#pragma once", '#include "fm1kit.h"', ""]
    total = 0
    for png in sorted(src.glob("*.png")) if src.exists() else []:
        name = re.sub(r"\W", "_", png.stem).upper()
        w, h, pal, rle, clear = convert(png)
        total += len(rle) + 2 * len(pal)
        lines.append(f"static const kit_color_t IMG_{name}_pal[] = {{ {', '.join(f'0x{c:04X}' for c in pal)} }};")
        body = ", ".join(str(b) for b in rle)
        lines.append(f"static const uint8_t IMG_{name}_rle[] = {{ {body} }};")
        lines.append(f"static const kit_image_t IMG_{name} = {{ {w}, {h}, IMG_{name}_pal, IMG_{name}_rle, {clear}, 1 }};")
        lines.append("")
    kit = user_kit(lines)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n")
    print(f"images   {out} ({total} B)" + (f", the user kit {kit} B" if kit else ""))


if __name__ == "__main__":
    main()
