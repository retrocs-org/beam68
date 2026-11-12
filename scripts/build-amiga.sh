#!/bin/sh
# Native 68000 build with either a configured GCC or vbcc/NDK toolchain.
set -eu
project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
if [ -n "${BEAM68_CC:-}" ]; then
    make -B -C "$project_root" CC="$BEAM68_CC" "$@"
    compiler=$BEAM68_CC
    flags=${CFLAGS:--O2 -m68000 -mcrt=nix20 -Wall -Wextra -Wno-pointer-sign}
else
    task_root=$(CDPATH= cd "$project_root/../.." && pwd)
    toolchain=${BEAM68_VBCC:-"$task_root/work/amiga-toolchain/vbcc-0.9hp3"}
    if [ ! -x "$toolchain/bin/vc" ]; then
        echo "Set BEAM68_CC to a compatible m68k-amigaos-gcc, or BEAM68_VBCC to your vbcc/NDK installation." >&2
        exit 1
    fi
    export VBCC="$toolchain"
    export PATH="$toolchain/bin:$PATH"
    make -B -C "$project_root" -f Makefile.cross "$@"
    compiler=$toolchain/bin/vc
    flags=${CFLAGS:--O2 -cpu=68000}
fi
if [ -f "$project_root/Beam68" ] && [ -f "$project_root/Beam68Prefs" ]; then
    python3 "$project_root/scripts/build-provenance.py" record --compiler "$compiler" --flags "$flags"
fi
