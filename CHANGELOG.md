# Changelog

## Unreleased — sources fixes (version remains 0.1)

- `mod:` sends the module to an already running player over ARexx
  (MODULE_PORT, MODULE_AREXX; default HIPPOPLAYER / LOAD "{file}") and
  only starts MODULE_COMMAND when no player runs.
- `web:` reads any hotlist layout: several links per line, links spanning
  lines, and plain-text files with bare http(s) addresses.
- `prefs:` lists SYS:Prefs live, independent of the indexed drawers.
- Beam68Prefs Sources page for all source settings, with a Browse button
  for bookmark files; on 640x200 screens the nine section buttons use two
  narrow columns so they never reach Use/Save/Cancel.

## Unreleased — index sources and media (version remains 0.1)

- New item kinds and scopes: `mod:` (music modules, INDEX_MODULES,
  MODULE_COMMAND), `web:` (HTML bookmark files, BOOKMARK_FILES,
  URL_COMMAND), `snip:` (ENVARC:Beam68/Snippets.txt, copied to the
  clipboard), `script:` (REXX:, INDEX_REXX, run with RX), `prefs:`
  (SYS:Prefs programs), `stats:` (most launched, with counts).
- Sources are indexed on full rebuilds and cached (schema allows 2-digit
  kinds and the sources root); sample Snippets.txt installed.
- Tests: module names, bookmark HTML, snippets, command templates, cached
  bookmarks/snippets, new scopes.

## Unreleased — tidy-ups, performance, community (version remains 0.1)

- Beam68Prefs Extras page: favorite keys, WHDLoad command, Opus port,
  "Open drawers in Opus", RAM plugin cache, list sizes, presets, catalogue.
- Alias table allocated on demand; plugin health keys migrated from the
  flat layout; Shift+F2 full rebuild (refreshes WHDLoad titles); weekday
  names and MusicControls labels translated; Opus 5 selects the file.
- List sizes are settings: RESULTS_MAX, BROWSE_MAX, HIDDEN_MAX, RECENT_DOCS.
- Performance: plugin manifests read on first use; single-word search
  narrows over the previous matches (fuzz-tested); ARexx TIMING command.
  A binary cache was not added: TIMING provides the numbers to decide.
- ARexx script plugins via RX; live plugin previews (PREVIEW=YES);
  Developer Tools and Date Utilities preview.
- Preferences presets (export/import, all-or-nothing).
- Colour picker for the custom theme; four gallery themes in themes/.
- Plugin/theme catalogue: index format, scripts/make-catalogue.py,
  `make catalogue`, catalogue browser with install in Beam68Prefs
  (downloads via DOWNLOAD_COMMAND, default wget).
- TRANSLATING.md for community translations.
- Tests: narrowing soundness, presets, catalogue, themes, ARexx packages,
  migrated health keys.

## Unreleased — fixes and tidy-ups (version remains 0.1)

- Up to 32 results are kept and scrolled (search, browsing, `volume:`,
  `doc:`, `hidden:`), independent of the number of visible rows.
- One shared 4 KB I/O buffer replaces three (icon, slave and game-path
  readers): 8 KB less resident memory.
- Bundled plugins' replies localised through the SDK catalog functions
  (`Beam68Plugins.catalog`, 76 strings, German and Swedish); Beam68Ctl
  messages localised; plugin-package errors are codes, shown translated
  in Beam68Prefs. ARexx replies and Ctl STATUS stay English for scripts.
- Plugin Details show the protocol version and the plugin's guide.

## Unreleased — calculator, documents, volumes, hiding, Opus, plugin cache (version remains 0.1)

- Calculator: `$FF`/`0xFF` hex and `%1010` binary literals (32-bit, two's
  complement); results shown as decimal, hex and binary (preview and plugin).
- `doc:` recent documents (files and Workbench projects opened via Beam68,
  last 16, `ENVARC:Beam68/RecentDocs.txt`).
- `volume:` lists mounted volumes and assigns live from the DOS list;
  Right browses them.
- Shift+Del hides a result (`ENVARC:Beam68/Hidden.txt`); `hidden:` lists
  them and Shift+Del there shows one again.
- Directory Opus 4/5/Magellan: "Show in Directory Opus" action (offered only
  while its port exists), `OPEN_DRAWERS_IN_DOPUS`, `DOPUS_PORT`.
- `PLUGIN_CACHE=YES`: plugins run from a RAM copy of their package drawer,
  refreshed when the original changes (true resident loading would need
  re-entrant plugins).
- Fix: Tab action cycling else-chain (found while adding the Opus action).
- Tests: hex/binary calculator, hidden list, recent documents, new scopes.

## Unreleased — Preferences/installer languages, browsing, public Protocol 1 (version remains 0.1)

- Beam68Prefs localised (190 strings: German, Swedish) with its own
  `Beam68Prefs.catalog` in LOCALE:Catalogs; buttons widened for longer
  labels; plugin-installer messages included. `make-catalog.py` now builds
  one catalog per program (`<Name>.cd`, `<Name>.<language>.ct`).
- The GUI installer follows `@language` (German, Swedish, English).
- Drawer browsing in the launcher (Right/Left/Esc, type to filter).
- Plugin public Protocol 1: up to eight OPEN/QUERY/TEXT rows via the public port
  BEAM68_REPLY; all bundled native plugins use the same transport;
  SDK `beam68_rows_begin/beam68_row/beam68_rows_send`.
- Tests: installer translations, both catalogs, Protocol 1 manifest validation and
  SDK message delivery.

## Unreleased — fixes and features round (version remains 0.1)

- Fix: switching WHDLoad indexing on/off with Use now rebuilds the index.
- `Beam68Ctl STATUS` (RC 0/5) and `QUITWAIT`; the GUI installer waits for
  the launcher to stop and aborts cleanly if it does not.
- Plugin installer reads LHA headers natively (levels 0/1/2) and applies the
  strict ZIP path rules; `C:LhA` is only needed for unpacking.
- WHDLoad: game title/copyright from the slave header (shown, searchable,
  cached), missing-Kickstart warning with confirm, `game:random`.
- Actions: Run with arguments, Information (WBInfo), Copy path.
- Amiga+V paste, Ctrl+1..9 and optional global `QUICK_LAUNCH` keys for
  favorites, mouse-wheel scrolling, underlined match highlighting.
- `GUIDE=` manifest key; Help on the Plugins page opens a plugin's guide.
  SDK: `beam68_catalog_open`/`beam68_str` for translated plugins.
- Build/release: `scripts/m68k-syntax-check.sh`, `.gitlab-ci.yml`,
  `scripts/make-aminet-release.py`.
- Tests: LHA headers, slave reader, tool types, highlight marks, titles in
  the index and cache, new actions.

## Unreleased — WHDLoad, languages and manual (version remains 0.1)

- WHDLoad support: game icons (Default Tool WHDLoad) and icon-less
  `.slave` files are indexed as GAME results, `game:` scope, launch via
  Workbench or `WHDLOAD_COMMAND SLAVE=… DATA=… PRELOAD`. New settings
  `INDEX_GAMES` and `WHDLOAD_COMMAND`, Indexing-page checkbox. Icons are
  classified by a small portable .info reader instead of GetDiskObject().
- The OS 3.0/3.1 WBStartup launcher handles icon-only projects.
- Localisation: locale.library catalogs for every launcher message
  (135 strings), German and Swedish translations, `catalogs/Beam68.cd`
  source, `scripts/make-catalog.py` builds header, defaults and IFF
  catalogs and checks placeholders. Both installers install the catalogs.
- AmigaGuide manuals: `Beam68.guide` and `sdk/Beam68PluginSDK.guide` with
  icons; the Help key in Beam68Prefs opens the manual at the current page.
- New tests: icon reader, catalogs, guides; config/scope/index tests extended.

## Unreleased — GUI installer (version remains 0.1)

- New `Install` script and icon for the AmigaOS Installer utility: install
  or update (destination drawer, plugin selection, Workbench autostart,
  stops a running launcher, keeps settings and icons, migrates the 0.1
  plugin layout) and uninstall (optionally keeping settings).
- Fresh installs write a configuration listing only the chosen plugins
  (`installer/Beam68-base.cfg`, derived from `Beam68.cfg` by
  `scripts/make-installer-cfg.py`).
- `$VER:` version strings in Beam68, Beam68Prefs and Beam68Ctl.
- The Shell installer also records `ENVARC:Beam68/InstallDir`.
- New static test for the installer script, its resources and icons.

## Unreleased — plugin packages and SDK (version remains 0.1)

- Plugins are self-contained package drawers (`Plugins/<Name>/`); manifests
  can use `PLUGINDIR:` for files inside their own drawer. All 13 bundled
  plugins were converted and are valid packages.
- New manifest keys: `ID`, `PLUGIN_VERSION`, `MIN_BEAM68`, `AUTHOR`,
  `LICENSE`, `URL`. Plugins requiring a newer Beam68 are not loaded.
- Beam68Prefs installs `.lha`/`.lzh`/`.zip` packages and plugin drawers
  (drop or Add): archive path screening, manifest/executable validation,
  consent dialog with capabilities and upgrade/downgrade, staged copy and
  swap, automatic enable unless the trigger/prefix is taken. Remove can
  delete installed files. Installed packages are discovered on startup.
- Old flat `PROGDIR:Plugins/X.plugin` configuration entries are migrated.
- Plugin SDK (`sdk/`): `beam68plugin.h`/`.c`, HelloWorld example, vbcc/gcc
  Makefile, `package-plugin.py` validator/packer, author guide.
  `make sdk-archive` and `make plugin-packages` build distribution archives.
- New tests: package rules, SDK protocol round-trip, packaging tool.

## Unreleased — correctness fixes (version remains 0.1)

### Fixed: crashes and blockers
- Beam68Prefs, FileFinder and ARexxRunner no longer overflow a 4 KB Shell
  stack when started from the launcher (F5), Preferences or a Shell; Beam68
  and Beam68Prefs switch to a 16 KB stack when started with less.
- Runs on AmigaOS 3.0 and later: workbench.library V44 is optional (built-in
  WBStartup launcher on 3.0/3.1) and child processes get explicit streams on
  pre-V47 dos.library.
- Removed the OS4-only `SYS_Error` tag that two plugins passed a string to.
- The recovery marker now guards only startup: a normal power-off or a
  `restart amiga` no longer forces safe mode on the next boot.

### Faster on 68000
- Per-keystroke search no longer compares favorites/history paths; ranking
  metadata is precomputed and most non-matching items are rejected by a
  character mask. Typed-ahead keys are coalesced into one search.
- `TextFit()` text clipping and partial redraws (query line / result list).
- O(1) duplicate detection while scanning; scans use a time budget instead
  of 12 entries per tick and yield to input.
- Icon type read from the DiskObject header instead of GetDiskObject();
  executables opened only when their protection bits allow execution.
- Rescans keep existing entries searchable and only rescan the changed root.

### Memory
- Pooled index: ~40 KB instead of ~157 KB at 512 items. Slimmer launcher
  plugin table, pointer-based help entries and a temporary reload config save
  ~65 KB of resident memory. Icon stack 16 KB instead of 32 KB.
- Index cache moved from ENVARC: (copied into RAM at every boot) to
  `PROGDIR:Beam68.index`, ~half the size and without a .bak copy.

### Other fixes
- Fonts load with diskfont.library; Preferences has a font requester.
- Window positions are keyed by public screen name, not title-bar text.
- Exchange "Disable" really releases the hotkey.
- Soft and hard drawer links are no longer followed while indexing.
- ClipboardTools finishes reads correctly; TaskManager walks task lists under
  Disable(); WindowSwitcher re-validates the window; MusicControls times out.
- Multi-word queries of scattered letters no longer outrank substring hits.
- 2 s FFS flush delay before reboots; longer Restart wait in Preferences.
- SystemCommands prefix is now `SYSTEM:` (no clash with typed `SYS:` paths).
- `INDEX_FILES` now defaults to NO.
- Host tests build with `-Werror` on current GCC; new pooled-index test.

## 0.1 — Initial release (in development)

- Added resident launcher with configurable global hotkey.
- Added Workbench tool/project indexing, fuzzy search and index cache.
- Added favorites, search history and keyboard shortcuts.
- Added separate Workbench Preferences application.
- Added editable text configuration and live settings reload.
- Added Workbench startup integration and drag-and-drop configuration.
- Added third-party plugin support with Calculator, Clock and Memory plugins.
- Added plugin management, per-plugin settings and timeouts.
- Added optional Workbench icon preview and resizable search window.
- Added ARexx scripting and Shell control utility.
- Added a lightweight public website compatible with classic Amiga browsers.
- Added native Workbench tool icons for Beam68 and Beam68Prefs.
- Added press-to-capture global hotkey configuration in Beam68Prefs.
- Added a larger, responsive Beam68Prefs window with a persistent settings sidebar.
- Added local-project plugin discovery when the configured install drawer is absent.
- Added keyboard navigation throughout Preferences.
- Added native alias, keyword and application-action management.
- Added ordered plugin management with diagnostics, settings, enable/disable and timeout controls.
- Added launcher action cycling with quoted path and drawer placeholders.
- Added state-aware Preferences Start, Restart and Stop controls for the resident launcher.
- Added highlighted Appearance theme selection with Workbench and four Catppuccin-inspired themes.
- Added opt-out `shell:<command>` execution in a separate AmigaDOS output console.
- Added live launcher lifecycle status with starting, running, restarting, stopping and stopped feedback.
- Added wrapping row navigation, page and edge shortcuts, and stable selection while refining a query.
- Added multi-term and one-edit typo-tolerant application search.
- Added compact `>` shell commands, whitespace cleanup and F8 command recall.
- Added named plugin progress, error and timeout feedback with stricter empty-reply validation.
- Added short descriptions for every launcher theme in Appearance.
- Added Ctrl+L query clearing and selected/total result position feedback.
- Added a live launcher example for the selected Appearance theme.
- Added persistent settings backups compatible with host-backed filesystems.
- Added favorites-first recent applications when the search query is empty and persistent F7/Shift+F7 query history.
- Added AUTO/CLOSE/WAIT shell-console lifecycle feedback.
- Added live drawer, entry and application indexing progress.
- Added persistent plugin success, error and timeout health counters in Plugin Details.
- Added launch-context-independent PROGDIR resolution for plugins started during Workbench boot.
- Added an editable five-color Custom theme with strict validation and live launcher preview.
- Replaced the space-heavy Appearance theme list with a compact cycle selector and wider live preview.
- Added immediate launcher theme updates from Preferences, cancel rollback and recovery-safe automatic restart on Use or Save.
- Added low-memory index allocation retries with a visible reduced-capacity status and safe reload rollback.
- Added validated configuration recovery from ENV, ENVARC and their backup files.
- Added context-aware no-results help for active indexing, empty indexes and unmatched searches.
- Added guarded `restart workbench` and `restart amiga` launcher commands with double-Enter confirmation.
- Added distinct two-plane normal/selected Workbench artwork to both shipped tool `.info` files.
- Added native File Finder, System Commands, Assigns/Volumes, Task Manager, Clipboard, Date, Workbench Window and Developer Tools plugins with readable command prefixes and guarded destructive actions.
- Added Escape cancellation for active plugin waits and safe-mode startup without plugins or custom themes.
- Added plugin-only live reload, execution timing, last-success feedback and APP/PLUGIN/ACTION/SHELL/RECENT result badges.
- Added shell, assign, drawer and plugin query completion.
- Added optional plugin category, icon, example and help metadata.
- Added compact diagnostics export and duplicate/ambiguous hotkey validation in Preferences.
- Added a guarded exact-name ARexx Runner plugin with user-configured macro mappings.
- Added AmigaDOS drawer notifications with debounced cooperative index refreshes.
- Added a visible, keyboard-selectable completion menu for commands, assigns, drawers and plugins.
- Added persistent unclean-exit detection with automatic recovery safe mode.
- Added automatic quarantine after three consecutive plugin errors or timeouts, with Preferences recovery.
- Added declarative native per-plugin settings pages while retaining the text-editor fallback.
- Added Music Controls for HippoPlayer, AmigaAMP and EaglePlayer 2 with auto detection, explicit targeting, transport, volume, status and a preferred-player setting.
- Added scoped `app:`, `file:`, `drawer:`, `plugin:`, `recent:`, `volume:` and grouped `fav:` searches with configurable file patterns and excluded drawers.
- Added current-public-screen placement, remembered geometry per screen and automatic compact layouts for lower resolutions.
- Added launcher font, row-spacing, high-contrast, larger-gadget and reduced-color accessibility settings.
- Added a searchable F1 command browser covering commands, plugins, aliases, examples and shortcuts.
- Added plugin capability declarations and second-Enter confirmation for `SYSTEM` plugins.
- Added portable theme import/export, dropped-theme previews and invalid-theme rollback.
- Added safe discovery of shipped plugin manifests as disabled entries for older configurations.
- Added a Plugins-page switch for the built-in managed Shell Commands service.
- Added persistent plugin-row selection while using plugin action controls.
- Added a configurable portable build-and-stage script with an explicit destination.
- Added direct testing support for the shared `amigaos-dev_020` profile and its `Bin:` volume.
