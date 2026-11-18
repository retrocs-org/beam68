#!/bin/sh
# Produces a test-only executable in build/; never installs or launches it.
set -eu
cd "$(dirname "$0")/.."
: "${BEAM68_CC:?Set BEAM68_CC to m68k-amigaos-gcc}"
mkdir -p build
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -include tests/native/fault_hooks.h -c src/preset.c -o build/qa-preset.o
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -include tests/native/fault_hooks.h -c src/item_index.c -o build/qa-item-index.o
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -include tests/native/fault_hooks.h -c src/index_cache.c -o build/qa-index-cache.o
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68ReleaseQA tests/native/release_qa.c tests/native/faults.c build/qa-preset.o build/qa-item-index.o build/qa-index-cache.o src/plugin_job.c src/protocol.c src/config.c src/appearance.c src/matcher.c src/search_rank.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68RexxProbe tests/native/rexx_probe.c src/rexx_commands.c
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -o build/Beam68SlowHost tests/native/slow_host.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68AminetQA tests/native/aminet_qa.c src/aminet.c src/aminet_http.c src/stack_guard.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68StackQA tests/native/stack_guard_qa.c src/stack_guard.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -DBEAM_PLUGIN_INSTALL_QA -Iinclude -o build/Beam68PluginImportQA tests/native/plugin_import_qa.c src/plugin_install.c src/plugin_catalog.c src/plugin_package.c src/config.c src/appearance.c src/dos_compat.c src/stack_guard.c src/beam68prefs_strings.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -Isdk/include -o build/Beam68AmiHASSQA tests/native/amihass_plugin_qa.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68AmiHASSTransportQA tests/native/amihass_transport_qa.c Plugins/AmiHASS/client.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -Isdk/include -o build/Beam68AminetCompletionQA tests/native/aminet_completion_qa.c src/aminet.c src/control.c src/stack_guard.c src/protocol.c src/plugin_common.c src/plugin_locale.c src/beam68plugins_strings.c sdk/src/beam68plugin.c

# Compile only: live controls always require explicit operator authorization.
"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68AmiHASSLiveQA tests/native/amihass_live_qa.c Plugins/AmiHASS/client.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68QueryStateQA tests/native/query_state_qa.c src/item_index.c src/index_cache.c src/query_history.c src/favorites.c src/search_rank.c src/matcher.c src/plugin_job.c src/protocol.c src/config.c src/appearance.c src/stack_guard.c

"$BEAM68_CC" -O2 -m68000 -mcrt=nix20 -Wall -Wextra -Werror -Wno-pointer-sign -Iinclude -o build/Beam68PolishQA tests/native/polish_runner_qa.c src/stack_guard.c
