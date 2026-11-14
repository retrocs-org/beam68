# Beam68 0.2 — ARexx automation

Beam68 optionally registers a public ARexx host port named **`BEAM68`**.
It uses the OS 3.x Exec/ARexx message protocol; it is separate from the
private `BEAM68.CONTROL` Exec port.

The resident launcher remains usable without `rexxsyslib.library` (ARexx is
simply unavailable). When another application already owns a port named
`BEAM68`, Beam68 does not replace it. No polling is performed to enable the
port later: restart Beam68 after correcting an ARexx startup problem.

## Commands

| Command | Effect / return text |
| --- | --- |
| `SHOW` | Opens the search window. |
| `HIDE` | Hides the window; commodity stays running. |
| `RELOAD` | Applies configuration and revalidates plugin entries. |
| `RELOADPLUGINS` | Reloads only plugin manifests without restarting Beam68. |
| `REINDEX` | Starts a cooperative fresh directory scan. |
| `STATUS` | Returns `version=0.2 items=<n> scanning=<0/1> plugins=<n> errors=<n> busy=<0/1> outstanding=<0..3>`. |
| `INDEX` | Returns detailed indexing counters: `scanning`, `roots`, `entries`, `apps`, `unreadable`, `ioerrors`, `overlong`, `capped`, `cached`. |
| `VERSION` | Returns `0.2`. |
| `PLUGINS` | Returns configured, loaded, and failed manifest counts. |
| `PLUGIN 1` | Status of the first configured manifest (1–16). |
| `REQUESTS` | Outstanding count, limits and whether a preview is waiting. |
| `REQUEST 1` | Slot 1 details (1-3): plugin name, serial, mode, transport, age, deadline and request state. |
| `TIMING` | Startup, window-open, complete search and input/result painting durations, maxima, sample counts and index pool bytes. |
| `TIMING RESET` | Clears interactive samples and window/search/narrowing counters; retains startup, cache and scan measurements. |
| `HELP` | Lists commands. |
| `QUIT` | Requests shutdown after the message has been replied to. |

Plugin status is **not** a filesystem audit: it reports the last load/reload
result, and a disabled plugin does not count as an error. The number passed to
`PLUGIN` is the 1-based order of the repeated `PLUGIN=` lines in Beam68.cfg.
If a plugin has an invalid manifest, its name/trigger may be unavailable.
Possible states:

- `ACTIVE`: executable resolved and trigger assigned
- `DISABLED`: disabled using `PLUGIN_DISABLED=`
- `MANIFEST_INVALID_OR_MISSING`: unreadable manifest or invalid fields
- `EXECUTABLE_MISSING`: executable could not be locked
- `EXECUTABLE_PATH_INVALID`: executable could not be resolved into a safe path
- `TRIGGER_CONFLICT`: an earlier loaded plugin already owns the same prefix

An example reply for `PLUGIN 2`:

```text
2 DISABLED path=PROGDIR:Plugins/Clock/Clock.plugin
```

## Script examples

Run the included `scripts/Beam68Status.rexx` from Shell using ARexx's `RX`
command when RexxMast/ARexx is available. Or write your own:

```rexx
/* Control a running instance without opening Preferences. */
IF ~SHOW('P','BEAM68') THEN DO
  SAY 'Beam68 ARexx port is not available.'
  EXIT 10
END
ADDRESS 'BEAM68'
'REINDEX'
'STATUS'
SAY RESULT
```

Commands are case-insensitive, reject trailing arguments except `PLUGIN n`, `REQUEST n` and `TIMING RESET`,
and accept no executable paths or arbitrary AmigaDOS input. Unknown commands
return RC=5. Requests that cannot be fulfilled (such as a nonexistent
configured plugin index) return RC=10. A shutdown-port message is replied to
with RC=20. Result text is returned only when requested through ARexx.

## Testing

`make host-test` validates the pure command parser and diagnostic formatter;
it does **not** exercise native RexxMsg handling. To validate native behavior,
compile with the AmigaOS 3.2 NDK and exercise `RX` scripts under supported
AmigaOS; verify response codes and that a script awaiting reply is not hung
when Beam68 quits. Test startup without a Rexx system library too.

### Detailed index diagnostics

`INDEX` is a read-only command and does not initiate scanning. `roots=x/y`
shows configured *root directories started*, rather than nested folders
completed. `unreadable` counts directories that could not be opened/examined;
`ioerrors` counts failed `ExNext()` operations other than normal
`ERROR_NO_MORE_ENTRIES`. `overlong` is the number of skipped paths. `capped`
shows whether `MAX_ITEMS` stopped the scan, and `cached=1` means the current
index was restored from disk. Use `REINDEX` to force a live scan.

## Responsiveness measurements (0.2)

Run from an Amiga Shell with Beam68 0.2 and RexxMast running:

```text
RX scripts/Beam68Responsiveness.rexx T:Beam68-small-01.log
```

The script asks for machine details, captures startup/index status, resets
interactive measurements, opens the launcher, and guides three typing tests.
It records numeric diagnostics, not query contents. Existing reports are never
overwritten. It does not restart Beam68, change configuration or reindex.
Repeat at least five times for both a small and large index, using identical
CPU/display/font/icon settings and recording the actual item count. Restart
Beam68 before each startup measurement. Keep warm-cache and initial-index runs
separate; do not compare scan-active and idle typing as if they were equivalent.

All duration fields end in `us` (microseconds):

- `startup`: timer initialization to resident-ready, including OS library,
  configuration, metadata and cached-index loading. Process loading and the
  timer initialization itself are outside this interval.
- `window`: last hidden-to-visible open through initial painting, including
  first-use plugin manifests and font loading. SHOW on an already open window
  does not create a new window sample.
- `search`: last complete search, including completion/scoped/empty paths.
- `inputpaint` / `inputmax` / `inputs`: last, maximum and count for text-edit
  batches from first dequeued edit to completion of query-header drawing.
- `resultpaint` / `resultmax` / `results`: same batch start to completion of
  result-area drawing, including searches and normal rendering work.
- `firstmatch` / `firstmax` / `matches`: last edited query to its first painted
  nonempty result list, including a wait for indexing if needed. No-match
  queries have no first-match sample. A new edit replaces an outstanding
  first-match wait; hiding the window discards incomplete measurements.
- `items`, `narrowed`, `poolused`, `poolallocated`: item count, narrowed-search
  count, live string bytes and allocated string bytes (not total process RAM).

Typing intervals start when Beam68 dequeues an edit; they exclude prior OS
input-queue delay. Drawing completion is not display scanout. These metrics
measure ordinary search results, not plugin completion latency. Zero sample
counts mean no measurement, not instantaneous performance. Last and maximum
are reported; they are not an average, median or percentile. Timing overhead
is included. Compare repeated runs on the same target before drawing conclusions.

## Outstanding plugin requests

STATUS distinguishes `busy` (the visible request) from `outstanding` (all
uncompleted requests, including detached or timed-out ones). The limit is
three. Automatic previews start only when fewer than two slots are occupied,
reserving the final slot for manual Enter invocation. When previews are
blocked, only the latest current query is retained and reconsidered after
capacity becomes available; intermediate queries are not queued.

Cancellation, closing and reload detach UI state without freeing an
uncompleted slot. Timeouts also retain the slot: a timeout is not evidence
that the process stopped. A native completion reply or an atomically published
ARexx result releases its slot; late completions do not display results or
change current plugin health. A failed process launch releases immediately.
Native completions use message signals; detached ARexx files are checked at
most twice a second and yield to queued input.

A plugin that never responds can occupy a slot indefinitely. Ordinary search
and typing remain available; after confirming hung plugin processes have
stopped, restart Beam68 to reset its request tracking. Restarting the launcher
does not terminate existing plugin processes. The cap bounds outstanding
Protocol 1 requests in one resident session, not every process or subprocess
that third-party code can start, nor work continuing after a terminal reply.

## Identifying occupied request slots

`REQUESTS` returns a summary such as:

```text
outstanding=2 limit=3 automatic_limit=2 waiting=1
```

Read `REQUEST 1`, `REQUEST 2` and `REQUEST 3` for a snapshot of every slot.
Slots are stable table positions; free slots can occur between occupied slots.
A populated slot includes the plugin name captured at launch (retained across
manifest reload), serial, preview/manual mode, native/file transport, age in
milliseconds and the launch-time timeout. No query arguments or manifest paths
are added to these per-request replies. Nonprinting name characters are replaced
with spaces and names are bounded to the configured name length.

`state=active` identifies the visible request; `state=detached` means its UI
request was cancelled or superseded; `state=timedout` means its original deadline
has passed. These are request states, not OS process-liveness assertions.
A timed-out slot stays occupied until completion. `state=free` means no request
is tracked in that slot. Ages are computed on demand and saturate at the native
unsigned-millisecond limit; reading diagnostics adds no periodic polling.

The request-limit footer includes names of holding plugins. The included
Beam68Status.rexx prints the summary and all three slots alongside existing
status information. To capture it from Amiga Shell:

```text
RX scripts/Beam68Status.rexx >RAM:Beam68-Status.txt
```

Other sections of that report retain their existing configured-path details.
The new commands are read-only and never free slots, retry jobs or kill tasks.

The Shell command `Beam68Ctl VERSION` prints the version reported by the running
launcher. It returns a warning when the launcher is stopped or cannot answer;
it does not substitute the control utility's own build version.
