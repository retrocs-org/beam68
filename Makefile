# Motorola 68000 / AmigaOS 3.2 NDK. No external GUI or FPU dependencies.
CC = m68k-amigaos-gcc
CFLAGS ?= -O2 -m68000 -mcrt=nix20 -Wno-pointer-sign -Wall -Wextra
INC = -Iinclude -Isdk/include
COMMON = src/config.c src/control.c
PLUGIN_COMMON = src/protocol.c src/plugin_common.c src/plugin_locale.c src/beam68plugins_strings.c sdk/src/beam68plugin.c
NEW_PLUGINS = Plugins/FileFinder/FileFinder Plugins/SystemCommands/SystemCommands Plugins/DosTools/DosTools Plugins/TaskManager/TaskManager Plugins/ClipboardTools/ClipboardTools Plugins/DateUtilities/DateUtilities Plugins/WindowSwitcher/WindowSwitcher Plugins/DeveloperTools/DeveloperTools Plugins/ARexxRunner/ARexxRunner Plugins/MusicControls/MusicControls

all: Beam68 Beam68Prefs Beam68Ctl CreateIcons Plugins/Calc/Calc Plugins/Clock/Clock Plugins/Memory/Memory $(NEW_PLUGINS)

Beam68: src/main.c src/rexx_sequence.c src/matcher.c src/search_rank.c src/search_scope.c src/calc_engine.c src/history.c src/query_history.c src/plugin_health.c src/favorites.c src/aliases.c src/index_cache.c src/actions.c src/plugin_job.c src/launcher_ui.c src/appearance.c src/recovery.c src/rexx_commands.c src/plugin_diagnostics.c src/scan_status.c src/completion.c src/item_index.c src/dos_compat.c src/wb_launch.c src/stack_guard.c src/startup_error.c src/icon_info.c src/whdload.c src/clip.c src/hidden.c src/sources.c src/locale_support.c src/beam68_strings.c $(COMMON) include/beam.h include/control.h include/search_rank.h include/search_scope.h include/history.h include/query_history.h include/plugin_health.h include/favorites.h include/aliases.h include/index_cache.h include/actions.h include/plugin_job.h include/launcher_ui.h include/appearance.h include/recovery.h include/rexx_commands.h include/plugin_diagnostics.h include/scan_status.h include/completion.h include/item_index.h include/dos_compat.h include/wb_launch.h include/stack_guard.h include/icon_info.h include/whdload.h include/clip.h include/hidden.h include/locale_support.h include/beam68_strings.h
	$(CC) $(CFLAGS) $(INC) -o $@ src/main.c src/rexx_sequence.c src/matcher.c src/search_rank.c src/search_scope.c src/calc_engine.c src/history.c src/query_history.c src/plugin_health.c src/favorites.c src/aliases.c src/index_cache.c src/actions.c src/plugin_job.c src/launcher_ui.c src/appearance.c src/recovery.c src/rexx_commands.c src/plugin_diagnostics.c src/scan_status.c src/completion.c src/item_index.c src/dos_compat.c src/wb_launch.c src/stack_guard.c src/startup_error.c src/icon_info.c src/whdload.c src/clip.c src/hidden.c src/sources.c src/locale_support.c src/beam68_strings.c $(COMMON)

Beam68Prefs: src/prefs.c src/launcher_ui.c src/hotkey_capture.c src/plugin_catalog.c src/plugin_diagnostics.c src/plugin_health.c src/favorites.c src/aliases.c src/wb_drop.c src/appearance.c src/theme_file.c src/dos_compat.c src/stack_guard.c src/startup_error.c src/plugin_package.c src/plugin_install.c src/preset.c src/catalogue.c src/catalogue_window.c src/beam68prefs_strings.c $(COMMON) include/beam.h include/control.h include/hotkey_capture.h include/plugin_catalog.h include/plugin_diagnostics.h include/plugin_health.h include/aliases.h include/wb_drop.h include/appearance.h include/theme_file.h include/dos_compat.h include/stack_guard.h include/plugin_package.h include/plugin_install.h include/prefs_locale.h include/beam68prefs_strings.h
	$(CC) $(CFLAGS) $(INC) -o $@ src/prefs.c src/launcher_ui.c src/hotkey_capture.c src/plugin_catalog.c src/plugin_diagnostics.c src/plugin_health.c src/favorites.c src/aliases.c src/wb_drop.c src/appearance.c src/theme_file.c src/dos_compat.c src/stack_guard.c src/startup_error.c src/plugin_package.c src/plugin_install.c src/preset.c src/catalogue.c src/catalogue_window.c src/beam68prefs_strings.c $(COMMON)

Beam68Ctl: src/ctl.c src/control.c include/control.h
	$(CC) $(CFLAGS) $(INC) -o $@ src/ctl.c src/control.c src/locale_support.c src/beam68_strings.c

CreateIcons: scripts/create_icons.c
	$(CC) $(CFLAGS) $(INC) -o $@ scripts/create_icons.c

Plugins/Calc/Calc: Plugins/Calc/Calc.c src/calc_engine.c $(PLUGIN_COMMON) include/calc_engine.h include/plugin_protocol.h include/plugin_reply.h
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/Calc/Calc.c src/calc_engine.c $(PLUGIN_COMMON)

Plugins/Clock/Clock: Plugins/Clock/Clock.c $(PLUGIN_COMMON) include/plugin_protocol.h include/plugin_reply.h
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/Clock/Clock.c $(PLUGIN_COMMON)

Plugins/Memory/Memory: Plugins/Memory/Memory.c $(PLUGIN_COMMON) include/plugin_protocol.h include/plugin_reply.h
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/Memory/Memory.c $(PLUGIN_COMMON)

Plugins/FileFinder/FileFinder: Plugins/FileFinder/FileFinder.c src/config.c src/appearance.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/FileFinder/FileFinder.c src/config.c src/appearance.c $(PLUGIN_COMMON)

Plugins/SystemCommands/SystemCommands: Plugins/SystemCommands/SystemCommands.c src/dos_compat.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/SystemCommands/SystemCommands.c src/dos_compat.c $(PLUGIN_COMMON)

Plugins/DosTools/DosTools: Plugins/DosTools/DosTools.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/DosTools/DosTools.c $(PLUGIN_COMMON)

Plugins/TaskManager/TaskManager: Plugins/TaskManager/TaskManager.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/TaskManager/TaskManager.c $(PLUGIN_COMMON)

Plugins/ClipboardTools/ClipboardTools: Plugins/ClipboardTools/ClipboardTools.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/ClipboardTools/ClipboardTools.c $(PLUGIN_COMMON)

Plugins/DateUtilities/DateUtilities: Plugins/DateUtilities/DateUtilities.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/DateUtilities/DateUtilities.c $(PLUGIN_COMMON)

Plugins/WindowSwitcher/WindowSwitcher: Plugins/WindowSwitcher/WindowSwitcher.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/WindowSwitcher/WindowSwitcher.c $(PLUGIN_COMMON)

Plugins/DeveloperTools/DeveloperTools: Plugins/DeveloperTools/DeveloperTools.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/DeveloperTools/DeveloperTools.c $(PLUGIN_COMMON)

Plugins/ARexxRunner/ARexxRunner: Plugins/ARexxRunner/ARexxRunner.c src/dos_compat.c $(PLUGIN_COMMON)
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/ARexxRunner/ARexxRunner.c src/dos_compat.c $(PLUGIN_COMMON)

Plugins/MusicControls/MusicControls: Plugins/MusicControls/MusicControls.c src/music_control.c $(PLUGIN_COMMON) include/music_control.h
	$(CC) $(CFLAGS) $(INC) -o $@ Plugins/MusicControls/MusicControls.c src/music_control.c $(PLUGIN_COMMON)

# Syntax-check native translation units without linking; requires a real 3.2 NDK.
# Complements the portable host tests, but does not replace target runtime validation.
native-syntax: check-toolchain
	@set -e; for file in src/*.c Plugins/*/*.c scripts/create_icons.c sdk/src/*.c; do \
	  echo "Checking $$file"; $(CC) $(CFLAGS) $(INC) -fsyntax-only $$file; \
	done

HOST_CC ?= gcc
check-toolchain:
	CC=$(CC) sh scripts/check-toolchain.sh

host-test:
	@mkdir -p build
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_rexx_sequence tests/test_rexx_sequence.c src/rexx_sequence.c
	./build/test_rexx_sequence
	@mkdir -p build
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_core tests/test_core.c src/matcher.c src/calc_engine.c src/protocol.c
	./build/test_core
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_search_rank tests/test_search_rank.c src/search_rank.c src/matcher.c
	./build/test_search_rank
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_narrowing tests/test_narrowing.c src/search_rank.c src/matcher.c
	./build/test_narrowing
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_search_scope tests/test_search_scope.c src/search_scope.c
	./build/test_search_scope
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -DBEAM_HOST -o build/test_aliases tests/test_aliases.c tests/fake_dos.c src/aliases.c src/config.c src/appearance.c
	./build/test_aliases
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_config tests/test_config.c tests/fake_dos.c src/config.c src/appearance.c
	./build/test_config
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_plugin_settings tests/test_plugin_settings.c tests/fake_dos.c src/config.c src/appearance.c
	./build/test_plugin_settings
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_history tests/test_history.c tests/fake_dos.c src/history.c
	./build/test_history
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_query_history tests/test_query_history.c tests/fake_dos.c src/query_history.c
	./build/test_query_history
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_plugin_health tests/test_plugin_health.c tests/fake_dos.c src/plugin_health.c
	./build/test_plugin_health
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_favorites tests/test_favorites.c tests/fake_dos.c src/favorites.c
	./build/test_favorites
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -DBEAM_HOST -o build/test_actions tests/test_actions.c src/actions.c src/locale_support.c src/beam68_strings.c
	./build/test_actions
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -DBEAM_HOST -Iinclude -o build/test_item_index tests/test_item_index.c src/item_index.c
	./build/test_item_index
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_icon_info tests/test_icon_info.c src/icon_info.c
	./build/test_icon_info
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_whdload tests/test_whdload.c src/whdload.c
	./build/test_whdload
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -DBEAM_HOST -o build/test_hidden tests/test_hidden.c tests/fake_dos.c src/hidden.c src/config.c src/appearance.c
	./build/test_hidden
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_sources tests/test_sources.c tests/fake_dos.c src/sources.c src/config.c src/appearance.c
	./build/test_sources
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_preset tests/test_preset.c tests/fake_dos.c src/preset.c src/config.c src/appearance.c
	./build/test_preset
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_catalogue tests/test_catalogue.c tests/fake_dos.c src/catalogue.c src/plugin_package.c src/config.c src/appearance.c
	./build/test_catalogue
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -DBEAM_HOST -Itests/fake_ndk -Iinclude -o build/test_cache tests/test_cache.c tests/fake_dos.c src/index_cache.c src/item_index.c
	./build/test_cache
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_plugin_job tests/test_plugin_job.c src/plugin_job.c src/protocol.c
	./build/test_plugin_job
	@mkdir -p build/testsettings/T
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_plugin_reply tests/test_plugin_reply.c tests/fake_dos.c src/plugin_reply.c
	./build/test_plugin_reply
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -Isdk/include -DBEAM68_NO_LOCALE -DBEAM68_NO_PORTS -o build/test_plugin_common tests/test_plugin_common.c tests/fake_dos.c src/protocol.c src/plugin_common.c sdk/src/beam68plugin.c
	./build/test_plugin_common
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -DBEAM_HOST -o build/test_scan_status tests/test_scan_status.c src/scan_status.c src/locale_support.c src/beam68_strings.c
	./build/test_scan_status
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_completion tests/test_completion.c src/completion.c
	./build/test_completion
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_music_control tests/test_music_control.c src/music_control.c
	./build/test_music_control
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_wb_drop tests/test_wb_drop.c src/wb_drop.c
	./build/test_wb_drop
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_hotkey_capture tests/test_hotkey_capture.c src/hotkey_capture.c
	./build/test_hotkey_capture
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_launcher_ui tests/test_launcher_ui.c src/launcher_ui.c
	./build/test_launcher_ui
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_appearance tests/test_appearance.c src/appearance.c
	./build/test_appearance
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_theme_file tests/test_theme_file.c tests/fake_dos.c src/theme_file.c src/appearance.c src/config.c
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_themes tests/test_themes.c tests/fake_dos.c src/theme_file.c src/appearance.c src/config.c
	./build/test_themes
	./build/test_theme_file
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -DBEAM_HOST -o build/test_recovery tests/test_recovery.c src/recovery.c src/locale_support.c src/beam68_strings.c
	./build/test_recovery
	$(HOST_CC) -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -o build/test_rexx_commands tests/test_rexx_commands.c src/rexx_commands.c src/plugin_diagnostics.c
	./build/test_rexx_commands
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_plugin_catalog tests/test_plugin_catalog.c tests/fake_dos.c src/config.c src/appearance.c src/plugin_catalog.c src/plugin_diagnostics.c
	./build/test_plugin_catalog
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -o build/test_plugin_package tests/test_plugin_package.c tests/fake_dos.c src/plugin_package.c src/config.c src/appearance.c
	./build/test_plugin_package
	$(HOST_CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/fake_ndk -Iinclude -Isdk/include -DBEAM68_NO_LOCALE -o build/test_sdk tests/test_sdk.c tests/fake_plugin_ports.c sdk/src/beam68plugin.c src/plugin_job.c src/protocol.c src/plugin_common.c src/calc_engine.c src/plugin_locale.c src/beam68plugins_strings.c
	./build/test_sdk
	python3 tests/test_build_provenance.py
	python3 tests/test_archive_safety.py
	python3 tests/test_package_tool.py
	python3 tests/test_installer.py
	python3 scripts/make-catalog.py --check
	python3 tests/test_catalogs.py
	python3 tests/test_guides.py
	python3 tests/test_public_web.py
	python3 tests/test_web_releases.py
	python3 tests/test_public_source.py
	python3 tests/test_release_tool.py
	python3 tests/test_info_icons.py

clean:
	rm -f Beam68 Beam68Prefs Beam68Ctl CreateIcons Plugins/Calc/Calc Plugins/Clock/Clock Plugins/Memory/Memory $(NEW_PLUGINS) build/test_core build/test_search_rank build/test_narrowing build/test_search_scope build/test_aliases build/test_config build/test_plugin_settings build/test_history build/test_query_history build/test_plugin_health build/test_favorites build/test_actions build/test_item_index build/test_icon_info build/test_whdload build/test_hidden build/test_sources build/test_preset build/test_catalogue build/test_cache build/test_plugin_job build/test_plugin_reply build/test_plugin_common build/test_scan_status build/test_completion build/test_music_control build/test_launcher_ui build/test_appearance build/test_theme_file build/test_themes build/test_recovery build/test_rexx_commands build/test_plugin_catalog build/test_plugin_package build/test_sdk build/test_wb_drop build/test_hotkey_capture

# Distribution archives: every bundled plugin as its own package, plus the SDK.
PLUGIN_DIRS = $(sort $(dir $(wildcard Plugins/*/*.plugin)))
plugin-packages:
	@mkdir -p dist/plugins
	@for d in $(PLUGIN_DIRS); do python3 sdk/tools/package-plugin.py $$d -o dist/plugins || exit 1; done

catalogue: plugin-packages
	python3 scripts/make-catalogue.py --base-url $(CATALOGUE_BASE_URL)

CATALOGUE_BASE_URL ?= http://example.org/beam68/

sdk-archive:
	python3 scripts/make-sdk-archive.py

.PHONY: all check-toolchain native-syntax host-test clean plugin-packages sdk-archive catalogue

# Header/catalog changes must invalidate every affected executable.
Beam68 Beam68Prefs Beam68Ctl Plugins/Calc/Calc Plugins/Clock/Clock Plugins/Memory/Memory $(NEW_PLUGINS): $(wildcard include/*.h sdk/include/*.h)
