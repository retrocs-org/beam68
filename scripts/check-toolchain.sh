#!/bin/sh
# Host-side preflight only. No AmigaOS binaries are built by this script.
set -eu
CC=${CC:-m68k-amigaos-gcc}
if ! command -v "$CC" >/dev/null 2>&1; then
    echo "Missing: $CC" >&2
    echo "Install the m68k AmigaOS cross-toolchain and AmigaOS 3.2 NDK." >&2
    echo "Portable tests can still be run with: make host-test" >&2
    exit 1
fi
printf 'Compiler: '
"$CC" --version | head -n 1
# Checks both supported -m68000 mode and the NDK system headers.
printf '#include <exec/types.h>\n#include <proto/dos.h>\n#include <proto/intuition.h>\n#include <proto/gadtools.h>\n#include <proto/wb.h>\n#include <workbench/workbench.h>\n#include <libraries/asl.h>\nint test(void) { return (int)sizeof(ULONG); }\n' |
    "$CC" -m68000 -mcrt=nix20 -x c -fsyntax-only - >/dev/null || {
        echo "Compiler found, but the m68000 flags or AmigaOS NDK headers are unavailable." >&2
        exit 1
    }
echo '68000 compiler flags and NDK header preflight: PASS'
echo 'Next: make CC='"$CC"
echo 'Native execution under AmigaOS 3.2 remains a separate validation step.'
