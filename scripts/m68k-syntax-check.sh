#!/bin/sh
# Compile-check every source for a 68000 against the AmigaOS 3.2 NDK headers
# without an Amiga toolchain: uses Debian/Ubuntu's gcc-m68k-linux-gnu and
# the GCC-ready NDK from https://github.com/sacredbanana/AmigaSDK-gcc.
#
#   scripts/m68k-syntax-check.sh <path-to-AmigaSDK-gcc>
#
# Catches syntax, type and prototype errors against the real NDK; it does
# not link and does not replace a vbcc build or a test on an Amiga.
set -e
SDK=${1:?usage: $0 <AmigaSDK-gcc checkout>}/amigaos3/sdk/m68k-amigaos
CC=${CC:-m68k-linux-gnu-gcc}
cd "$(dirname "$0")/.."
SHIM=$(mktemp -d)
trap 'rm -rf "$SHIM"' EXIT
mkdir -p "$SHIM/proto"
echo '#include <proto/wb.h>' > "$SHIM/proto/workbench.h"
status=0
for file in src/*.c Plugins/*/*.c scripts/create_icons.c sdk/src/*.c sdk/Example/*/*.c; do
    if ! "$CC" -m68000 -std=gnu89 -fsyntax-only -Wall -Wextra -Werror \
        -Wno-unused-parameter -Wno-pointer-sign \
        -D__stdargs= -D__saveds= -D__regargs= -D__chip= -D__far= -D__near= \
        -D__amigaos__ -Dmc68000 -nostdinc -I"$SHIM" \
        -I"$SDK/libnix/include" -I"$SDK/sys-include" -I"$SDK/ndk-include" \
        -Iinclude -Isdk/include -isystem "$("$CC" -print-file-name=include)" "$file"; then
        echo "FAILED: $file" >&2
        status=1
    fi
done
[ $status -eq 0 ] && echo "m68k syntax check: all sources PASS"
exit $status
