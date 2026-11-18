# Beam68 architecture — 0.4

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
                       | versioned Exec control message
                  Beam68Prefs (GadTools UI)
                       |
                       +-> Use: T:Beam68/Beam68.cfg (temporary override)
                       +-> Save: ENVARC:Beam68/Beam68.cfg + session copies
                       +-> Queries.txt / PluginHealth.txt persistent state
```

## Current configuration and plugins

The current Preferences layout and activation policy are documented in
[PREFERENCES.md](PREFERENCES.md). There are 18 independently versioned bundled
plugins; only the first six essential local tools start enabled. Network and
other optional integrations are opt-in. Music Library, WHDLoad and DOpus own
their respective integrations. WHDLoad uses the shared indexing roots, depth and
exclusions through a background discovery worker. Unlimited traversal uses
heap-backed frames and avoids following directory links.

Use/Save request confirmed configuration reload through the control port. An
unchanged hotkey is retained; new hotkey expressions initialize IX_VERSION.
The installer removes temporary Use configuration and its backup when removing
settings or creating a fresh configuration. This prevents stale overrides.

The sections below include implementation history from 0.1; version labels on
those sections describe their introduction.

## Separate programs

Beam68 is the only resident process; it holds the index and Commodities broker. Beam68Prefs creates a window only while needed. Plugins execute as separate AmigaDOS processes. `CreateIcons` is a native installer helper; it is **not** needed at runtime. This keeps the idle resident process small.

## ENV: and ENVARC: behavior

Config load preference: T:Beam68/Beam68.cfg and its backup, then ENV:, ENVARC:, their backups and PROGDIR: fallback. This follows the AmigaOS preferences Use/Save convention without requiring binary IFF settings. The installer copies the sample into ENV and ENVARC only if missing. A write streams preserved user comments and third-party config keys to a `.new` file and writes owned settings canonically. The original is renamed to `.bak` before `.new` replaces it. If a host-backed filesystem keeps an occupied `.bak` name, the writer selects the first free `.b01` through `.b99` backup instead. If replacement fails, restoration is attempted; the chosen backup remains available for manual recovery. Query history is stored in `ENVARC:Beam68/Queries.txt`; plugin outcome counters are stored in `ENVARC:Beam68/PluginHealth.txt`. The last settings save is not transactionally atomic across separate ENV: and ENVARC: volumes.

## Control protocol

Name: `BEAM68.CONTROL`. Message fields: `struct Message` + 32-bit magic `0x42453638`, version `1`, command enum (Reload=1, Reindex=2, Show=3, Quit=4, Hide=5, ReloadPlugins=6, PreviewTheme=7, QueryVersion=8).

A sender allocates the entire message in public memory, atomically finds and sends to the port inside `Forbid()` / `Permit()` and then returns immediately. The launcher frees the message after receipt. Ordinary actions have no reply port or synchronous wait. The message must never contain process-private pointers. Only one launcher can register the named port; shutdown removes it from the system's public list before deleting its backing MsgPort.

QueryVersion is read-only and carries a bounded private reply-port name and serial, never a borrowed port pointer. About waits at most 25 ticks. The launcher allocates a separate version response and looks up the named recipient atomically before sending; after timeout the port is removed and late replies are discarded safely. Missing launchers and older/busy launchers are distinguished from a reported version. This does not change public plugin Protocol 1.

**No security boundary** is provided: any local program that knows the protocol could send a control command, as is common for classic-Amiga message ports.

## Configuration reloading

The launcher starts reading current config into its static `next_cfg`. Numeric and path fields are validated; Commodities uses `ParseIX` to check a new hotkey expression before it is committed. If maximum index capacity changes, a new buffer is allocated first; failure leaves the existing buffer intact. Then the filter is updated in place (`SetFilterIX`), config and index are swapped, plugin manifests reloaded, and search results updated. `FreeVec` releases the old buffer after reindexing. F3 and Prefs use the same path. Erroneous hotkey syntax leaves the existing filter untouched.

## Text index cache and result actions (still 0.1)

`INDEX_CACHE=YES` stores a bounded index in `PROGDIR:Beam68.index` (next to
the launcher). It is deliberately not in ENVARC:, because the Startup-Sequence
copies all of ENVARC: into RAM on every boot. Schema 5 stores kind, root,
a %XX-escaped path and optional title per line, plus a
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

Appearance keeps the five built-in palette definitions, adds a bounded Rose,
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

The project **application version is 0.4**. Keep it at 0.4 until explicitly requested otherwise.

## Custom themes, memory fallback and recovery (version 0.1)

Appearance defines five fixed roles: text, shadow, selected-row fill,
selected-row text and background. Fixed themes remain allocation-free.
The Rose theme stores the same five roles as strict 24-bit `RRGGBB` values
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
independent of the private control port `BEAM68.CONTROL`, and it
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

All 14 bundled native plugins send public Protocol 1 Exec messages through
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

## Search responsiveness in 0.3

Raw key releases and unmapped keys no longer split queued text into extra
searches. Result-dependent actions flush the pending query, including
Shift+Delete. The query header paints before ranking and body rendering.
Scans check for input between directory entries and refresh new matches on
the next scan tick while no results are visible. Optional disk icon previews
load only after a selection settles for approximately 100–150 ms.

Matcher scoring is unchanged; substring searches stop after the first best
word-boundary hit and typo-prefix checks inspect only the required prefix.

## Startup and index memory in 0.3

Duplicate launches return before loading configuration, libraries or catalogs;
the existing later resident check is retained. The diskfont library opens only
when a launcher window needs a font, and closes after acquiring the font.
Startup timing starts immediately after timer.device initialization, before
other library, configuration, metadata and index loading.

The index path pool starts at the lesser of 4 KiB or the configured capacity
budget (56 bytes per item), then doubles as needed up to that existing budget.
Items and the hash table retain their configured capacities. Unused pool bytes
are not cleared. Growth copies only live bytes and rebases every path, display
name and title before freeing the old pool. Borrowed string pointers must not
survive add/title operations. Allocation failure leaves existing entries valid
and follows the existing index-full behavior; growth temporarily needs both
old and new pool allocations. With the default 512-item setting, the initial
pool reservation falls from 28 KiB to 4 KiB; larger populated indexes grow.

## Low-memory recovery and rescan consistency in 0.3

Index allocation and growth failures preserve existing paths, hashes and
metadata. Existing titles are replaced by building a compact replacement
string pool before committing it; unchanged titles require no allocation.
Updated titles and kind upgrades refresh launcher metadata and invalidate
query narrowing. The configured pool and item limits still apply.

A completed scan snapshots its errors before reconciliation. Only a fully
walked, uncapped, error-free scan may remove stale entries or create a complete
cache. Otherwise reconciliation clears stale flags and retains unverified
entries. This favors retaining possibly outdated results over treating an
unreadable drawer as proof of deletion. The next successful scan can remove
them. Ordinary result indices and an argument-entry target are remapped during
compaction; removed targets become -1 rather than referring to another item.

Loading a cache into a populated index builds and validates a separate index,
then swaps it in. Corrupt, mismatched and out-of-memory reloads leave the live
index untouched. Initial cache loading uses the empty index directly and
rejects partial data on failure. Cache writes finish a .new file before moving
the prior cache to .bak and installing the replacement. On failure, restoration
is attempted; if restoration also fails, the backup remains for recovery.
A successful replacement retains a committed backup; interrupted-save
recovery and backup rotation are described below.

Host-only deterministic allocation failure injection covers initial allocation,
pool expansion, title replacement and cache loading. Cache tests also inject
write failures and both replacement rename failures. These hooks are absent
from the native binary; memory and undefined-behavior sanitizers cover the
same growth, compaction and rollback cases on the host.

## Plugin request lifecycle in 0.3

Text edits immediately detach the current plugin request, even when a later
edit restores the same text. Programmatic query changes are also checked before
searching. Close, explicit cancellation, plugin reload and committed settings
reload detach outstanding requests and clear preview state. A new invocation
uses a fresh 32-bit serial and unique result path. Replies require an active,
unexpired request, the current serial/query and an open window. Queued window
edits are processed before native replies, avoiding an obsolete result paint.
Plugin Protocol 1 and SDK message layout remain unchanged.

Native requests use only the public reply port, without polling or deleting
unused reply files. Headers/counts/error flags, row kinds, nonempty terminated
labels and bounded control-free values are validated before row copying.
Correlatable malformed native packets finish the request as errors; obsolete
packets are discarded without affecting current results or health. Atomic text
replies must contain exactly one complete valid line. Empty, truncated,
control-containing and extra-line replies fail instead of being displayed.

Timeouts use elapsed time from timer.device rather than counting delivered
scheduling ticks. Expired replies cannot revive a request. Preview failures and
timeouts contribute to the existing health/quarantine policy. Routine health
saves are deferred for about one second and yield to queued input; clean exit
and deliberate reboot still save pending counters immediately.

Cancellation never kills an unsupervised child or unlinks a file it could
still be writing. A crashed process without a reply is detected by deadline,
not by an OS process-exit notification. Abandoned text files may remain in T:
until cleanup/reboot. Children retain normal OS privileges and may continue
running; request detachment is not process isolation or termination. Optional
first-use package caching and OS filesystem/program-loading calls retain their
normal filesystem latency; plugin execution itself remains asynchronous.

## User data during 0.3 installation

Both installers treat an existing portable Beam68.cfg, Aliases.txt and each
bundled theme file as user-owned. New binaries and manuals update normally;
missing editable files receive defaults. GUI creation of missing ENVARC settings
uses a pre-existing portable configuration when present, then extends plugin
entries without replacing user choices. Missing ENV configuration inherits
ENVARC; aliases follow the same existing-data precedence. Plugin defaults seed
missing ENVARC settings from an installed package-local or legacy flat .cfg
before using shipped defaults. Persistent favorites and history files are not
installation targets, and their 0.1 formats remain readable and writable.

## Per-plugin preview controls in 0.3

Preferences -> Plugins -> Details -> Preview provides Toggle, Faster, Slower,
Delay... and Default. Controls edit the in-memory configuration; Use/Save retain their
existing session/permanent semantics. Delays range from 100 to 5000 ms in
100 ms UI steps, with a default of 400 ms. Hand-edited valid delays are rounded
up to the shared 50 ms timer tick. Manual Enter invocation is unaffected.
A manifest must still declare PREVIEW=YES: user preferences cannot opt an
action-only plugin into automatic execution.

PLUGIN_PREVIEW_DISABLED stores a manifest path. PLUGIN_PREVIEW_DELAY stores
milliseconds|manifest-path. Both may precede or follow PLUGIN entries, accept
legacy flat-path migration and use case-insensitive path matching. A second
read pass resolves overrides only when these keys occur; ordinary configs
need no extra pass. Unknown plugin overrides and invalid delays are ignored.
Saving emits canonical keys for installed entries and drops removed entries.

Runtime storage is one unsigned short delay and one disabled byte for each
of 16 configured manifests (48 bytes on the target), with no heap allocation.
Preferences swaps/shifts these fields with plugin paths, initializes newly
added slots and keeps policy across package updates. Scheduling checks the
current override both when arming and when firing a preview; settings reload
already detaches the preceding request and clears the old timer.

## Bounded plugin requests in 0.3

The visible BeamPluginJob and a three-slot outstanding-request table have
separate lifetimes. Ending the visible job never implicitly frees the table
entry. Launch admission checks the table before cache/program preparation;
reservation happens just before asynchronous spawn, with rollback on failure.
Serials skip zero and any outstanding serial at wrap. Preview admission stops
at two occupied slots, leaving the third available to manual invocation.

The latest blocked preview is represented by one waiting flag and the current
query, with no per-keystroke queue or allocation. Replies, close/reload and
edits cannot reinstate a previous queued query. Capacity release schedules a
fresh delayed preview only if the window remains open and its current plugin
still permits previews.

Terminal native packets release matching slots independently of visible-job
acceptance; late packets cannot update rows/status/health. Correlatable but
truncated or unsupported-version messages may fail a visible job without
proving completion, so its slot stays held. Atomic ARexx final-file presence
signals completion of detached work. Known final files are inspected only at
500ms intervals and only after input, never by globbing or scanning T:.
Timeouts retain slots. There is no automatic expiry-based recycling, which
would allow a permanently hung plugin to accumulate unlimited invocations.
Native-only detached requests need no timer polling.

The table and three bounded reply paths use less than 1 KiB on the native
target, without heap allocation. This bounds uncompleted protocol requests,
not hostile third-party code or process lifetime after a terminal response.
Actual process-exit supervision remains separate from request completion.

## Interrupted cache-save recovery in 0.3

Loading tries the fully validated primary cache first, then .old, then .bak.
Every candidate must pass the same schema, indexing-settings fingerprint,
capacity, row validation, checksum and end-of-file checks. .old contains the
latest previous primary while a pre-existing .bak is retained during save.
Neither .new nor partially parsed rows are accepted as a committed cache.
A lone .new from an interrupted first save therefore causes a normal scan.

Allocation/pool-capacity failure aborts recovery without treating the file as
corrupt or attempting backup promotion. Populated indexes remain transactional:
no failed candidate replaces live results. Initial startup reuses the empty
index while trying candidates, so recovery needs no second full index.

After a backup is validated, filename repair is best effort. An invalid primary
is moved aside to .new; the chosen backup is renamed to primary. If promotion
fails, restoration of the prior filename is attempted and the validated backup
is retained. Even when file repair is refused, validated contents remain usable
in memory. A validated primary permits discarding an uncommitted .new and
cleaning up interrupted backup rotation; repair failure never invalidates the
in-memory load.

Saving keeps existing .bak data until the new primary has committed. If .bak
already exists, the previous primary moves to .old; failed installation rolls
that move back. After commit, .old rotates to .bak. Failed final rotation leaves
.old recoverable. An unresolved .old prevents another save from overwriting
recovery state until a successful load repairs it. All cache files stay next
to the launcher rather than in ENV/ENVARC; recovery adds disk storage, not a
second persistent RAM index. Filesystem corruption can still require a rescan.

## Request visibility in 0.3

REQUESTS reports the count, admission limits and latest-preview waiting flag.
REQUEST 1..3 reports a stable slot snapshot: captured plugin name, serial,
mode, transport, age and original timeout. Names/timestamps/policy are copied
at launch, so details never borrow pointers into a reloaded plugin table.
Completing/reusing a slot clears or replaces its captured name. Request state
is derived at read time; it does not assert that a child process is alive or
has exited, and diagnostic reads never release slots or alter health.

Names are bounded and stripped of control characters in diagnostic output.
Query arguments and manifest paths are not added to per-request records.
The limit footer names holding plugins, and Beam68Status.rexx includes every
slot (including free holes). Age uses saturating unsigned-millisecond arithmetic
rather than a microsecond counter that would saturate after about 71 minutes.
No additional periodic wakeups, file I/O or heap allocation are needed for
these snapshots; new timing fields are read only on request.

## Direct preview-delay entry

Delay... opens a small native GadTools requester with a focused text field,
OK and Cancel. The field accepts exact whole-millisecond values in 100..5000;
invalid input stays visible for correction and never changes the old value.
Explicit OK/Return commits to the outer preview dialog; Tab does not commit.
Cancel, Escape and window close discard the local entry. The outer Preferences
Use/Save semantics remain unchanged, and entering a delay does not enable a
disabled preview. Field, buttons, validation feedback and controls are translated
in English, German and Swedish. The requester uses its own gadget list and
releases it on cancellation, window/allocation failure and successful commit.

The same bounded decimal parser handles numeric dialog input and delay entries
in configuration files. It rejects signs, fractions, units, out-of-range values
and overflow before modifying its output. The shared timer still rounds the
effective wait upward to its 50ms granularity; the exact requested value is
preserved in preferences.

## Editable theme palettes (0.3)

Mocha is the fresh-install default. Every theme, including Workbench and Rose,
can be edited in Appearance. Overrides are independent per theme and persisted
as THEME_COLORS_0 through THEME_COLORS_5, each with five RRGGBB roles. Restore
original palette clears only the selected override; Workbench resumes following
the public-screen pens and Rose restores its shipped reddish palette. Use applies
session settings, Save persists them, and Cancel restores the launcher preview.
The legacy CUSTOM theme/configuration spelling remains compatible with Rose.

Installer completion starts and verifies the resident without sending SHOW. Its
final confirmation reads the active hotkey through Beam68Ctl INSTALLINFO and
formats the configured Commodities expression. A temporary environment variable
passes the label to Installer and is removed immediately after reading it.

Plugin selection uses GadTools' persistent highlight bar. Priority up/down moves
plugins in matching order; Toggle changes enabled state. Use/Save applies plugin
changes without a dedicated Reload button. Details uses one screen-sized wrapped
document with scrolling on smaller screens; Typing preview controls automatic
requests after a typing pause. Installer startup uses WAITREADY, which responds
immediately for a ready resident and only polls while waiting for one to start.
