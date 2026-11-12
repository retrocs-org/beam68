# Beam68 0.1 — ARexx automation

Beam68 optionally registers a public ARexx host port named **`BEAM68`**.
It uses the OS 3.x Exec/ARexx message protocol; it is separate from the
existing private `BEAM68.CONTROL` one-way Exec port.

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
| `STATUS` | Returns `version=0.1 items=<n> scanning=<0/1> plugins=<n> errors=<n> busy=<0/1>`. |
| `INDEX` | Returns detailed indexing counters: `scanning`, `roots`, `entries`, `apps`, `unreadable`, `ioerrors`, `overlong`, `capped`, `cached`. |
| `VERSION` | Returns `0.1`. |
| `PLUGINS` | Returns configured, loaded, and failed manifest counts. |
| `PLUGIN 1` | Status of the first configured manifest (1–16). |
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

Commands are case-insensitive, reject trailing arguments except `PLUGIN n`,
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
