#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Every automated gate, in order; stops at the first failure. What CI runs, and what an agent runs
# before it calls a change done.
#   factory/check.sh [APP...]      (default: every app in apps/)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
APPS="$*"
[ -n "$APPS" ] || APPS="$(ls "$ROOT/apps")"
echo "== reqlint (requirement wording, docs/REQUIREMENTS-PLAYBOOK.md)"
python3 "$ROOT/factory/reqlint.py"
echo "== trace (requirements, scenarios, checks, decisions)"
python3 "$ROOT/factory/trace.py"
echo "== build"
python3 "$ROOT/factory/userkit.py"            # the user kit (R-137, R-138): before the real build, which follows
for a in $APPS; do python3 "$ROOT/tools/build.py" "$a"; done
echo "== unit tests (portable fm1kit code on the host)"
mkdir -p "$ROOT/work/unit"
for t in "$ROOT"/factory/unit/test_*.c; do
    n="$(basename "$t" .c)"
    m="${n#test_}"; m="${m%%_*}"                 # test_<module>[_...].c is linked with fm1kit/src/<module>.c
    x=""; [ "$m" = engines ] && x="$ROOT/fm1kit/src/juno.c $ROOT/fm1kit/src/fxbus.c"   # the Juno-106 (D-090), the bus (D-093)
    [ "$m" = synth ] && x="$ROOT/fm1kit/src/fxbus.c"           # the effects bus (D-093)
    cc -Wall -Wextra -O1 -I"$ROOT/fm1kit/include" -I"$ROOT/fm1kit/src" -I"$ROOT/build/gen" -I"$ROOT/third_party/felucca-engines/src" "$t" "$ROOT/fm1kit/src/$m.c" $x -lm -o "$ROOT/work/unit/$n"
    "$ROOT/work/unit/$n"
done
echo "== music oracle (C port vs Sketchpad)"
python3 "$ROOT/factory/oracle/check.py"
echo "== safety"
for a in $APPS; do python3 "$ROOT/factory/safety.py" "$a"; done
echo "== usb midi (out and in, emulated USB cable)"
for a in $APPS; do python3 "$ROOT/factory/usb_midi.py" "$a"; done
echo "== cpu (every sound, 8 voices, cycle-modelled)"
for a in $APPS; do python3 "$ROOT/factory/cpu.py" "$a"; done
echo "== scenarios (host simulator and emulated firmware)"
python3 "$ROOT/factory/run.py" --no-build
