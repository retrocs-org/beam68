#!/bin/sh
# Produces a test-only executable in build/; never installs or launches it.
set -eu
cd "$(dirname "$0")/.."
: "${BEAM68_CC:?Set BEAM68_CC to m68k-amigaos-gcc}"
mkdir -p build
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -include tests/native/fault_hooks.h -c src/preset.c -o build/qa-preset.o
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68ReleaseQA tests/native/release_qa.c tests/native/faults.c build/qa-preset.o src/config.c src/appearance.c src/matcher.c src/search_rank.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68RexxProbe tests/native/rexx_probe.c
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -o build/Beam68SlowHost tests/native/slow_host.c
