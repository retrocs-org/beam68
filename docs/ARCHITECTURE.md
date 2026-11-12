# Beam68 architecture — 0.1

```text
Workbench launch (WBStartup/Beam68Start Project)
     \ Default Tool -> SYS:Utilities/Beam68/Beam68
      \  Commodities input filter / hotkey
       \  Beam68 main loop (Intuition + Exec)
        |             |            |
        |             |            +-> Async external plugins (hex request + T: result)
        |             +-> DOS icon/HUNK scanner -> RAM index -> fuzzy + bounded history ranking
        +-> BEAM68.CONTROL named Exec port
                       ^
                       | one-way versioned Exec message
                  Beam68Prefs (GadTools UI)
                       |
                       +-> Use: ENV:Beam68/Beam68.cfg
                       +-> Save: ENVARC:Beam68/Beam68.cfg + ENV copy
                       +-> Queries.txt / PluginHealth.txt persistent state
```

## Separate programs

Beam68 is the only resident process; it holds the index and Commodities broker. Beam68Prefs creates a window only while needed. Plugins execute as separate AmigaDOS processes. `CreateIcons` is a native installer helper; it is **not** needed at runtime. This keeps the idle resident process small.

## ENV: and ENVARC: behavior

Config load preference: ENV: -> ENVARC: -> PROGDIR: fallback. This follows the AmigaOS preferences Use/Save convention without requiring binary IFF settings. The installer copies the sample into ENV and ENVARC only if missing. A write streams preserved user comments and third-party config keys to a `.new` file and writes owned settings canonically. The original is renamed to `.bak` before `.new` replaces it. If a host-backed filesystem keeps an occupied `.bak` name, the writer selects the first free `.b01` through `.b99` backup instead. If replacement fails, restoration is attempted; the chosen backup remains available for manual recovery. Query history is stored in `ENVARC:Beam68/Queries.txt`; plugin outcome counters are stored in `ENVARC:Beam68/PluginHealth.txt`. The last settings save is not transactionally atomic across separate ENV: and ENVARC: volumes.

## Control protocol

Name: `BEAM68.CONTROL`. Message fields: `struct Message` + 32-bit magic `0x42453638`, version `1`, command enum (Reload=1, Reindex=2, Show=3, Quit=4).

A sender allocates the entire message in public memory, atomically finds and sends to the port inside `Forbid()` / `Permit()` and then returns immediately. The launcher frees the message after receipt. There is **no reply port or synchronous wait**. The message must never contain process-private pointers. Only one launcher can register the named port; shutdown removes it from the system's public list before deleting its backing MsgPort.

**No security boundary** is provided: any local program that knows the protocol could send a control command, as is common for classic-Amiga message ports.

## Configuration reloading

The launcher starts reading current config into its static `next_cfg`. Numeric and path fields are validated; Commodities uses `ParseIX` to check a new hotkey expression before it is committed. If maximum index capacity changes, a new buffer is allocated first; failure leaves the existing buffer intact. Then the filter is updated in place (`SetFilterIX`), config and index are swapped, plugin manifests reloaded, and search results updated. `FreeVec` releases the old buffer after reindexing. F3 and Prefs use the same path. Erroneous hotkey syntax leaves the existing filter untouched.

## Text index cache and result actions (still 0.1)

`INDEX_CACHE=YES` stores a bounded index in `PROGDIR:Beam68.index` (next to
the launcher). It is deliberately not in ENVARC:, because the Startup-Sequence
copies all of ENVARC: into RAM on every boot. Schema 4 stores kind, root and
a %XX-escaped path per line (the name is derived from the path), plus a
schema marker, an indexing-configuration hash and an ending checksum. Old
schema-3 files in ENVARC:/ENV: are deleted at startup. All entries must pass validation
before the launcher accepts them. Malformed/truncated/old-format data
causes a normal directory scan. Atomic-style `.new`/`.bak` replacement
protects the previous file. Cache contents are derived data: if directory
contents change, F2 or `Beam68Ctl REINDEX` forces a fresh scan. The cache
is not a filesystem monitor; nested directory timestamps are not watched.

Result action mode cycles between Launch, Open containing drawer and an
optional application-specific command on the Tab raw-key *down* event, with
key-up events discarded. Commands expand `{path}` and `{drawer}` into quoted
AmigaDOS arguments. The action resets on new results or a changed selection.
The portable `actions.c` tests cover cycling, expansion and volume syntax.

## Incremental usability improvements (still 0.1)

The calculator preview calls the shared integer arithmetic engine within the launcher
while typing with the `=` trigger. The external plugin remains available on Enter,
and plugins use public Protocol 1 row messages.

A bounded 48-entry ASCII launch history is read from
`ENVARC:Beam68/History.txt` and kept in memory. Successful launches increase
a capped ranking bonus within the same match class (so exact/prefix matches
retain precedence). History is flushed after four launches or on orderly exit,
not per keystroke. Sudden shutdown can lose the last few launches. Turning
`HISTORY=NO` off means no counting or ranking, but does not erase stored history.
When the query is empty, indexed favorites are followed by distinct recent
applications from this launch history. A separate bounded 12-entry query
history is persisted in `Queries.txt`, deduplicated, and traversed older/newer
with F7 and Shift+F7. Only successfully dispatched searches, shell commands
and plugin requests are recorded; four pending changes or orderly shutdown
trigger a write.

The separate Preferences editor can invoke the OS ASL directory picker on the
Indexing page or select `.plugin` files on the Plugins page. `Browse` fills
the text gadget; `Add` confirms the addition. Manual entry remains available.

The shell action accepts either case-insensitive `shell:` or the compact `>`
prefix. A bounded helper strips surrounding spaces before execution; the last
successfully dispatched command is retained in resident memory for F8 recall.
Its `CON:` output window uses `AUTO/CLOSE/WAIT`, preserving readable output
while avoiding an unmanaged console after completion.
Ctrl+L shares the existing clear-query path. Search feedback includes the
one-based selected row and total result count.

Appearance keeps the five built-in palette definitions, adds a bounded Custom palette,
read-only description plus a compact launcher example drawn with the selected
palette's actual background, text and selection colors. It falls back to the
public-screen pens if a low-depth screen cannot allocate the full palette.
Plugin jobs retain their manifest name while active so progress, duplicate-run,
protocol error and timeout text identifies the responsible plugin. Empty
ARexx `TEXT=` and `ERROR=` replies are invalid; completed script requests
remove the result and `.part` companion. Native results arrive as messages. Successful, error and timeout outcomes update
bounded persistent counters keyed by manifest path. Preferences Plugin Details
shows those counters and the most recent outcome.

## GUI and 68000 considerations

Preferences uses classic `gadtools.library` (v39+) with a nine-section sidebar,
string/integer/checkbox controls and listviews. It manages directories, ordered
plugins, favorites, aliases/keywords and application actions without requiring
MUI/ReAction. Large bounded settings structures are globals rather than stack
objects. The launcher hides its window when idle.

## Known limitations

1. NDK ABI compilation, Workbench startup behavior and hotkey keymap compatibility require real-system tests.
2. A single blocking AmigaDOS filesystem operation can still stall input handling briefly; plugin execution itself is asynchronous.
3. Preferences uses a larger two-pane, nine-section layout intended for standard 640x200 and larger Workbench screens. Large title bars, unusual font settings, or unusually small screens can still prevent opening.
4. Current error feedback is a title change and system beep; a user-friendly error requester would improve the UI.
5. Config save preserves comments/unknown keys but canonicalizes owned keys and does not preserve their locations.
6. ENV/ENVARC Save is two sequential writes, not a cross-volume transaction.
7. Directory notifications are registered for configured roots. A change is
   debounced for 500 ms and starts a fresh cooperative scan, so filesystem
   bursts do not repeatedly rebuild the index. Handlers without notification
   support still require F2. Scanning remains in the main task rather than a
   shared-memory worker process.


## Cooperative scan and asynchronous plugins (0.1)

The resident process opens the built-in `timer.device`, keeps a single
`TR_ADDREQUEST` outstanding while work exists, and sleeps without timer
requests when idle. On each reply it processes at most 12 directory entries
from a bounded depth-first iterator stack. Each directory level owns its own
`Lock()` and `AllocDosObject(DOS_FIB)` pair, preserved between `ExNext()`
invocations. Cancelling a scan releases all outstanding locks and FIBs.
Reindexing restarts the in-memory index directly, so the popup may show partial
results until indexing completes. The final cache write still occurs on the
launcher task and may briefly block on slow media.
The launcher title/status line reports processed roots, directory entries and
discovered applications on every cooperative update, then reports final totals.

The plugin launcher resolves its own startup directory through `GetProgramDir`,
`NameFromLock` and `AddPart`, so manifest and executable `PROGDIR:` paths do not
depend on the caller's current directory. It starts a separate Shell process with `SYS_Asynch` and
separate `NIL:` streams via `beam_system()` (`src/dos_compat.c`): V47
`SYS_InName`/`SYS_OutName` on OS 3.2, explicitly opened `SYS_Input`/`SYS_Output`
handles on older dos.library. The
result filename contains a task identifier, time and a sequence number. The
launcher receives native row messages through BEAM68_REPLY and checks their
version and request serial. Only ARexx scripts use the polled TEXT=/ERROR=
file transport, which rejects control characters and empty payloads. One
outstanding plugin at a time is supported.
After about 30 seconds the wait expires but the AmigaDOS child is not killed,
so abandoned files and runaway third-party processes remain an OS-level
limitation. Native manifests and reply messages both declare public Protocol 1.

The project **application version is 0.1**, regardless of source changes.

## Custom themes, memory fallback and recovery (version 0.1)

Appearance defines five fixed roles: text, shadow, selected-row fill,
selected-row text and background. Fixed themes remain allocation-free.
The Custom theme stores the same five roles as strict 24-bit `RRGGBB` values
in `CUSTOM_COLORS`; invalid text is rejected before Use or Save. Preferences
obtains temporary pens for its launcher example and releases them immediately.
The resident launcher applies the custom roles through the same palette path
and falls back to Workbench pens if the screen cannot supply every color.

The item array remains one bounded allocation. `allocate_index()` retries a
failed request at half capacity down to a minimum useful 16 entries. Startup
continues with the first successful capacity and reports the reduced limit.
Reload allocates first and only swaps after success, so complete allocation
failure preserves the current index, configuration and hotkey.

Active settings are parsed into a temporary default-initialized structure and
validated before use. Read precedence is ENV, ENVARC, ENVARC `.bak`, ENV
`.bak`, then the shipped project configuration. A malformed higher-priority
file therefore cannot leak partial values into the next recovery candidate.

`recovery.c` also owns bounded two-line no-results messages. The launcher
distinguishes an in-progress scan, a completed empty index and a populated
index with no match, offering actions appropriate to each state without new
runtime allocation.

## Guarded restart commands and Workbench icons (version 0.1)

`actions.c` recognises only the exact case-insensitive commands
`restart amiga`/`restart:amiga` and `restart workbench`/`restart:workbench`.
A bare `restart` is still searched normally. The launcher requires a second
unchanged Enter before execution; editing the query, hiding the window or
pressing Esc disarms it. Workbench restart releases Beam68's public-screen
window and lock, calls `CloseWorkBench()`, then `OpenWorkBench()`. Full Amiga
restart flushes pending query, launch and plugin-health state before
`ColdReboot()`.

The shipped `.info` files are generated classic WBTOOL DiskObjects, not PNG or
NewIcons data. Each is 54x22, two bitplanes, transparent color zero, a 32768
byte stack and explicit normal plus `GFLG_GADGHIMAGE` selected artwork. Beam68
uses a launcher/search motif; Beam68Prefs uses a two-pane controls motif. The
installer copies both DiskObjects without replacing later user customizations.

## Popup input/display (version 0.1)

`src/launcher_ui.c` is deliberately independent of AmigaOS libraries. It bounds in-line ASCII text edits and supplies row hit-testing, visible-row calculation and scroll adjustment with no allocations. `src/main.c` implements Intuition's native resizable window, cursor drawing/scrolling, keyboard mappings and `IDCMP_MOUSEBUTTONS` with Intuition `DoubleClick()`. On each show it locks the currently active public screen, restores that screen's bounded geometry from `ENV:Beam68/WindowStates.txt`, and enables the compact policy when configured or when the screen is small. Saved defaults remain `WINDOW_WIDTH` and `WINDOW_HEIGHT` (`0` = computed automatic height) in Beam68.cfg. Screen bounds account for the title bar. Preferences is a separate GadTools program with a persistent nine-section sidebar and page-specific right pane. Launcher and Preferences drawing were verified on AmigaOS 3.2.1 in runtime checks.

## Appearance and selected-icon policy (0.1)

`src/appearance.c` is a portable, allocation-free policy module: it selects pen numbers
from Intuition `DrawInfo` and decides whether a 52-pixel icon column can fit.
On window open, Beam68 locks the public screen and obtains `GetScreenDrawInfo()`;
on close it releases the `DrawInfo` before unlocking that screen.

The optional `ICON_PREVIEW=YES` setting does not affect search indexing or the
index cache fingerprint. Only the selected Workbench item can load an icon.
`main.c` retains at most one `DiskObject` and only attempts an `icon.library`
`DrawIconStateA` preview when the measured image fits 48x48 pixels.
On small windows, or when previewing is disabled, rendering is text-only.
For classic/68000 systems, previewing defaults to NO to avoid additional
per-selection floppy I/O and icon memory allocations.

The UI's palette is derived from the *active public screen*, not a fixed
Workbench four-color palette. Portable tests check that nonstandard pens are
preserved and that icon-column geometry stays within the content area. The
native icon drawing itself must be tested with the AmigaOS 3.2 NDK and real
public screens; host tests do not exercise icon.library or graphics.library.


## Favorites and acronym ranking (version 0.1)

`src/favorites.c` owns a fixed array of at most 48 paths and uses staged
AmigaDOS text-file writes. ENV: and ENVARC: are used with the conventional
Use/Save distinction; F6 commits a favorite immediately to both. The
launcher loads favorites once at startup or config reload, never on each
search keystroke. Nonindexed paths do not appear in results, even if
listed in Favorites.txt.

`src/search_rank.c` keeps exact and prefix matches above initials,
substring and fuzzy matches, then applies bounded favorite/history
tie-breaking bonuses only after a real name match. An empty search is
special: it displays indexed favorites in pin order. The separate
Beam68Prefs Favorites page uses the same storage layer without changing
settings in Beam68.cfg.


## Optional ARexx host (Beam68 0.1)

The public `BEAM68` Exec port accepts the standard ARexx `RexxMsg` format,
validates via rexxsyslib and replies before returning to the scheduler. It is
independent of the private one-way control port `BEAM68.CONTROL`, and it
accepts only the fixed command vocabulary in `src/rexx_commands.c`. Status
queries are read-only and use cached state rather than rescanning disks.
`rexxsyslib.library` is optional at startup; no mandatory ARexx dependency is
introduced. See `docs/AREXX.md`.

### Plugin catalog separation (0.1)

Beam68Prefs uses `src/plugin_catalog.c` to resolve configured `.plugin` paths against `INSTALL_DIR` instead of its own `PROGDIR:`, inspect manifest metadata, test binary availability via AmigaDOS `Lock()`, and identify trigger conflicts. It never loads or executes the plugin while probing. Disabled entries do not reserve triggers. The resident launcher maintains its own path resolution and standalone ARexx diagnostic port.

Plugin-specific preferences are opt-in via `SETTINGS=<absolute AmigaDOS path>` in protocol-1 manifests. Only C:Ed is started asynchronously by Beam68Prefs; every plugin is responsible for parsing its own file. This is intentionally not an executable GUI API.

## Scan diagnostics and plugin timeouts (still version 0.1)

The plugin job stores its deadline as a number of 50ms timer ticks selected at
launch (5–120 seconds, default 30). Later config reloads affect only new jobs.
The public plugin protocol is version 1. The native indexer samples IoErr()
immediately after a failed ExNext(), before performing further DOS operations,
and records only non-END failures. An unsuccessful/partial scan invalidates the
persistent index. The INDEX ARexx command formats a bounded 1-line status
through the portable `scan_status.c` module. A cached startup resets scan
counters and reports `cached=1`.

## Cache cap and plugin reply reliability (still 0.1)

The scan cache is committed **only** if scanning finished all configured
roots and no unreadable directories, I/O errors, overlong paths, or
`MAX_ITEMS` truncation occurred. Hitting the configured index limit can
produce useful partial results in RAM but must never turn those into a
claimed-complete persistent cache. `scan_status.c` owns a portable predicate
for this and includes unit tests for all rejection conditions.

All 13 bundled native plugins send public Protocol 1 Exec messages through
BEAM68_REPLY, including text and error results. The launcher validates the
message version and request serial. ARexx scripts use a separate atomic
TEXT=/ERROR= transport: write a request-specific .part file and rename it
into place after Close. Timed-out scripts may leave late files under T:.
Pre-release plugin formats are not retained as compatibility versions.


## 68000 performance and OS 3.x compatibility (audit fixes)

- **Pooled index** (`src/item_index.c`): an item is ~20 bytes plus its path in a
  shared string pool; the name points into the path. An open-addressing hash
  makes duplicate detection O(1) during scans.
- **Mark-and-sweep rescans**: F2, REINDEX and filesystem notifications mark
  the affected roots stale, refresh re-found items in place and remove only
  vanished items when the walk completes. The index is never empty during a
  rescan, and a notification rescans only the root that changed.
- **Precomputed ranking metadata**: favorite position, recency, launch bonus,
  alias binding and a character-presence mask are computed per item on add,
  launch, pin and reload. The per-keystroke search does no favorites/history
  string comparisons and rejects most non-matching items with one mask test
  (sound: every match class tolerates at most one absent character).
- **Input coalescing and partial redraws**: a burst of typed keys costs one
  search; cursor moves redraw the query line, navigation the result list.
  Text is clipped with a single `TextFit()` call.
- **Scanner**: ~30 ms time budget per 50 ms tick, yielding at once on pending
  input; icon type read from the 49-byte DiskObject header; executables are
  opened only when their protection bits allow execution; links are skipped.
- **Stack**: Beam68 and Beam68Prefs switch to a private 16 KB stack when
  started with less (`src/stack_guard.c`); large buffers are static.
- **OS 3.0/3.1**: workbench.library V44 is optional; without it tools and
  projects are started with a WBStartup message (`src/wb_launch.c`).


## Plugin packages

- **Layout**: `<INSTALL_DIR>/Plugins/<Drawer>/<Name>.plugin` plus the
  executable and optional files. `PLUGINDIR:` in a manifest resolves against
  the manifest's drawer (`beam_plugindir_resolve()` in `src/config.c`), used
  by both the launcher (`load_plugins`) and Preferences (`plugin_catalog.c`).
- **Configuration** still lists manifests by path (`PLUGIN=`); paths are now
  stable per drawer. Old flat paths are migrated in `beam_config_read()`.
  Upgrades are matched by manifest `ID`.
- **Validation** (`src/plugin_package.c`, portable and host-tested): ID,
  version and `MIN_BEAM68`, `PLUGINDIR:`-only `COMMAND`/`ICON`, flat
  `ENVARC:Beam68/Plugins/` settings files, safe archive entry names.
- **Installation** (`src/plugin_install.c`, Preferences only): listing screen
  (strict for `unzip -Z1` names; parent-reference screen for LhA listings),
  extraction to `T:Beam68-Install`, single-drawer/single-manifest check, HUNK
  executable check, EasyRequest consent, copy to `<Name>.new` then rename
  swap with rollback, config update. External tools: `C:LhA`, `C:UnZip`.
- **SDK** (`sdk/`): stand-alone; its protocol library is tested against the
  launcher's own reply parser (`tests/test_sdk.c`).


## WHDLoad, localisation and documentation

- **Icon classification** (`src/icon_info.c`, portable): reads the on-disk
  DiskObject, skips DrawerData and both images, and returns the type and
  Default Tool. The scanner reads 50 bytes for tools and up to 2 KB for
  projects; a Default Tool of WHDLoad makes a GAME item.
- **Localisation**: `catalogs/Beam68.cd` is the source of truth;
  `scripts/make-catalog.py` generates `include/beam68_strings.h`,
  `src/beam68_strings.c` and `catalogs/<language>/Beam68.catalog` (IFF
  CTLG, ISO-8859-1) and fails on placeholder mismatches. `beam_str()` in
  `src/locale_support.c` uses GetCatalogStr() with the built-in English as
  fallback; host tests compile it with `BEAM_HOST`.
- **Manuals**: AmigaGuide files checked by `tests/test_guides.py`
  (balanced nodes, links, the per-page nodes Beam68Prefs opens).
