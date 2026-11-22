#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Everything the build and the factory need, on Linux x86-64 (CI, a cloud session, a dev machine):
#   - the JieLi toolchain (clang 4.0.1 for pi32v2), checked by hash
#   - the three JieLi AC79 SDK files the package carries (sparse clone, ~70 MB), checked by build.py
#   - Python packages (Pillow, numpy, lz4)
#   - the emulator (third_party/fm1-emu) with the factory patch, built
#   JIELI_HOME (~/.jieli)  AC79_SDK (~/fw-AC79_AIoT_SDK)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JIELI_HOME="${JIELI_HOME:-$HOME/.jieli}"
AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
TC_SHA256=a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd   # jieli-linux-toolchains-20260730.1

git -C "$ROOT" submodule update --init third_party/felucca-bsp third_party/felucca-engines third_party/fm1-emu

if [ ! -x "$JIELI_HOME/toolchain/pi32v2/bin/clang" ]; then
    mkdir -p "$JIELI_HOME/dl"
    curl -fL --retry 3 -o "$JIELI_HOME/dl/linux-toolchain.tar.xz" https://pkgman.jieliapp.com/s/linux-toolchain
    echo "$TC_SHA256  $JIELI_HOME/dl/linux-toolchain.tar.xz" | sha256sum -c - || {
        echo "setup: JieLi published a different toolchain; check it, then update TC_SHA256"; exit 1; }
    tar -xJf "$JIELI_HOME/dl/linux-toolchain.tar.xz" -C "$JIELI_HOME"
    ln -sfn "$(basename "$(ls -d "$JIELI_HOME"/jieli-linux-toolchains-* | sort | tail -1)")" "$JIELI_HOME/toolchain"
fi
echo "toolchain: $JIELI_HOME/toolchain"

if [ ! -f "$AC79_SDK/cpu/wl82/tools/uboot.boot" ]; then
    for try in 1 2 3 4; do                      # (gitee drops connections now and then)
        rm -rf "$AC79_SDK"
        if git clone --depth 1 --filter=blob:none --sparse --branch AC79NN_SDK_V1.2.1_2023-12-13 \
                https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$AC79_SDK" &&
           git -C "$AC79_SDK" sparse-checkout set cpu/wl82/tools &&
           [ -f "$AC79_SDK/cpu/wl82/tools/uboot.boot" ]; then
            break
        fi
        [ "$try" = 4 ] && { echo "setup: could not fetch the AC79 SDK from gitee"; exit 1; }
        sleep $((try * 5))
    done
fi
echo "sdk: $AC79_SDK"

python3 -m pip install -q Pillow numpy lz4 2>&1 | grep -v "^WARNING" || true

EMU="$ROOT/third_party/fm1-emu"
PATCH="$ROOT/factory/patches/fm1-emu-factory-outputs.patch"
# the factory patch (D-021: kept as a patch file): applied for the build, then taken out again, so the submodule
# stays as upstream has it and the built emulator carries the patch
if git -C "$EMU" apply --check "$PATCH" 2>/dev/null; then
    git -C "$EMU" apply "$PATCH"
fi
(cd "$EMU" && cargo build --release -p fm1-live)
git -C "$EMU" apply --reverse "$PATCH"
echo "emulator: $EMU/target/release/fm1-live"
echo "setup done. export JIELI_TOOLCHAIN=$JIELI_HOME/toolchain AC79_SDK=$AC79_SDK"
