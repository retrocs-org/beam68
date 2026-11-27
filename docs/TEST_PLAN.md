# Beam68 validation checklist

Use an isolated AmigaOS test installation and independent copies of settings.
Record the tested build, OS version, screen mode and available memory.
See BUILD_STATUS.md for completed checks and outstanding coverage.

## Current 0.2 regression checks

Wikipedia searches/summaries and Music Library searches should run on explicit
Enter without another SYSTEM guard. Music Library PLAY and other SYSTEM plugins
still require confirmation. The isolated installer runner must preserve its
seeded temporary settings on update and remove them with Remove settings.

- Novice uninstall with Remove settings, then Novice install: only Calculator,
  Clock, Memory, File Finder, System Commands, and Assigns and Volumes are ON.
  Repeat with an existing temporary Use configuration and its backup.
- A settings-preserving update retains explicit plugin choices.
- Use and Save apply plugin changes to the running launcher; failures leave
  Preferences open with feedback.
- Check all six Preferences sections, General/Appearance subpages, Application
  shortcuts explanation, and separate plugin settings Save/Cancel.
- Check finite and unlimited index depth, WHDLoad enable/disable and shared roots.
- Check editable/restorable palettes, default Mocha and Rose.
- Enable Wikipedia explicitly, test search, summary, language, HTTPS failure and
  OpenURL. Test Aminet progress, cancel and launcher-window handoff.
- Follow the earlier feature-specific checks below where applicable; historical
  0.1 headings record when those checks were added.

## Custom theme, recovery and no-results checks

1. In Appearance, select Rose and enter five comma-separated `RRGGBB`
   values. Confirm the preview changes immediately, Save persists
   `THEME=CUSTOM` plus `CUSTOM_COLORS`, and malformed or incomplete colors
   keep Preferences open with a clear error.
2. Force successively smaller index allocations. Beam68 must halve the
   requested capacity down to 16 entries, report the active low-memory cap,
   and remain usable. A reload that cannot allocate even 16 entries must keep
   the old index and configuration intact.
3. Corrupt or remove ENV: and ENVARC: configurations in turn. Confirm startup
   accepts the first valid file in ENV, ENVARC, ENVARC `.bak`, ENV `.bak`,
   then the shipped template order. Invalid backups must never be committed.
4. Verify two-line assistance while indexing, with an empty completed index,
   and with a populated index that has no query match. Confirm the suggested
   F2, F5, F7 and Ctrl+L actions work.
5. Enter `restart workbench` and verify the first Enter only arms the command.
   Change one character and verify confirmation is cancelled. Confirm with a
   second Enter, then verify Workbench closes/reopens while Beam68 remains
   resident. Repeat `restart amiga` only on a disposable test installation and
   verify query/history state is flushed before the cold reboot.
6. Display Beam68.info and Beam68Prefs.info on four-color Workbench. Confirm
   their search and settings designs are distinct, both selected states render,
   tool type remains WBTOOL and stack remains 32768 bytes.

## Final launcher polish checks (still 0.1)

1. Run `>version`, confirm surrounding whitespace is stripped, the separate
   console opens, and F8 recalls `>version` after hiding and reopening Beam68.
   Repeat with the longer case-insensitive `shell:` form and with commands
   disabled in General.
2. Enter a query with at least six results, verify `1/6` changes with selection,
   then press Ctrl+L and confirm the query and results clear.
3. Run a slow, failing and empty-reply test plugin. Progress, duplicate-run,
   error and timeout text must name the plugin; empty `TEXT=`/`ERROR=` must be
   rejected and completed `.part` companions removed.
4. In Appearance, cycle through all six theme choices and confirm the compact
   selector, short Theme details text and wide launcher example update without
   clipping. Confirm Up/Down changes the theme and verify the colors on RTG.
5. On a test filesystem, occupy or make the normal `.bak` backup
   unavailable, press Save, and confirm the live config is replaced while the
   old config is retained in the first free `.b01` through `.b99` file.

## Recent items, query history, console, indexing and plugin health

1. With an empty query, verify pinned favorites appear first in pin order,
   followed by distinct recently launched indexed applications. Unindexed or
   duplicate history paths must not appear.
2. Successfully launch an application, run a shell command and dispatch a
   plugin query. Press F7 for older entries and Shift+F7 for newer entries;
   verify wrap/boundary behavior, deduplication, the 12-entry cap and
   persistence across a clean restart. Refused or failed dispatches must not
   be recorded.
3. Run successful, failing and interactive AmigaDOS commands. Verify the
   separate AUTO/CLOSE/WAIT console opens when output is produced, keeps
   important output readable, and does not leave unmanaged console processes.
4. Force F2 reindexing and verify drawer, entry and application counters update
   monotonically while input remains responsive. Confirm the completion status
   matches the final index and that a cancelled/restarted scan resets counters.
5. Run a successful plugin, an error plugin and a timeout plugin. Reopen
   Preferences Plugin Details and verify persistent success/error/timeout
   counts plus the last outcome. Restart from WBStartup and confirm relative
   `PROGDIR:` manifests and executables still resolve.

1. Compile with amiga-gcc and AmigaOS 3.2 NDK, inspect compiler diagnostics and executable HUNK/M68000 CPU requirements.
2. Run `make host-test` first; test `Beam68`, `Beam68Prefs` and `CreateIcons` with both Shell and Workbench startup.
3. Use `Execute scripts/Install-Beam68` on a temporary fresh OS3.2 volume; check all icon and file locations, and ensure existing preferences are never overwritten.
4. Reboot and check `SYS:WBStartup/Beam68Start` correctly loads the real `SYS:Utilities/Beam68/Beam68` tool; verify no duplicate instance and proper termination in Commodities Exchange.
5. Check Ctrl+Alt+Space on US/Swedish keyboard maps and under common public-screen layouts; use Preferences to switch to a different valid hotkey and verify hotkey is updated live.
6. Save settings in Prefs (General/Indexing/Plugins pages) with Use and Save, kill launcher, reboot, test ENV and ENVARC precedence, manual Ed edits, comment/unknown key preservation and `.bak` files.
7. From Indexing, test ASL Browse and manually typed paths; from Plugins, select a manifest with Browse. Then add/remove directories on a large HDD and a floppy; test low memory, filesystem errors, deep paths, duplicate icons, invalid `.info`, HUNK files without icons and interrupted scanning.
8. Change MAX_ITEMS from 256 to 512 and back during runtime, verifying allocation failure leaves previous configuration intact.
9. Launch Workbench applications and Shell HUNK binaries from fuzzy-search results; check icon tooltypes, current directory and stack; return to launcher and dismiss via Esc.
10. Verify live calculator preview on each key without process spawning. Execute Calculator, Clock, Memory; test malformed user input, paths with spaces, long result lines and slow plugins.
11. Enable `INDEX_CACHE=YES`, start Beam68 a second time and verify it loads cached items without a scan. Edit indexing directories/depth/options and confirm automatic cache invalidation. Corrupt/truncate `PROGDIR:Beam68.index`, check fallback to a fresh scan; compare F2 and `Beam68Ctl REINDEX` behavior after adding/removing programs. Try cache disabled and test disks with little free space and write-protected ENVARC:.
12. With a selected app, press Tab to toggle Launch and Open drawer; ensure Enter opens the containing drawer without updating history. Check key-up does not toggle twice. Verify special paths like `SYS:Tools/Ed`, root-level items and paths containing spaces.
13. Try a low memory 68000/OS3.2 configuration (e.g. A500 with 1-2 MB) and an A1200/OS3.2 configuration; inspect screen layout at 640x200, 640x256 and smaller screens. Confirm buttons are inside the window and listviews are usable.
14. Check preferences from F5 when installed into SYS:Prefs, via icon launch, and when Beam68 is not running.
15. Launch programs repeatedly and check search ranking, `HISTORY=NO`, history file parsing and loss of pending counts after hard reset. Test after forced Ctrl+C, Workbench shutdown and Commodities Kill; ensure named port and CxObjects are removed cleanly.

The host test harness covers string matching, calculator/protocol behavior, configuration parsing/saving, bounded history, parent-path actions, and text-index cache validation with a fake DOS filesystem. It **cannot** validate AmigaOS-specific libraries or gadgets.

## Additional responsiveness regressions (0.1)

16. On a slow floppy or high-latency drive, trigger F2/REINDEX with many
    drawers, type a new query, hide/reopen the popup and open Beam68Prefs
    while indexing. Check progressive updates, final cache, no leaked locks
    after RELOAD/REINDEX/QUIT, and that final results are complete.
17. Write a third-party plugin which sleeps ~5 seconds before writing
    `TEXT=OK\n` into its supplied result path. While it runs, press Escape,
    invoke the launcher hotkey, and change query. A second plugin request
    should report that another is running. Verify success appears only for
    the original query, and nothing waits synchronously.
18. Test a plugin that never delivers a result. The launcher should time out
    in roughly 30 seconds and remain usable. Verify a late child response
    cannot appear as a subsequent plugin result. Check `T:` for orphaned
    files and stop any runaway children manually; the host does not kill them.
19. Send Commodities Disable/Kill and Ctrl+C while timer work is pending;
    verify timer.device requests are aborted/drained and all directory
    FileInfoBlocks/locks freed cleanly. Verify no periodic timer requests
    remain at idle and no allocations grow across repeated reindexes.
20. Compile on AmigaOS 3.2 NDK v47 with actual `dos/dostags.h` macros
    `SYS_InName`/`SYS_OutName` and `devices/timer.h`'s
    `struct timerequest`/`tr_time` fields; host tests don't cover those.

## Launcher usability and appearance regressions (still 0.1)

21. At 640x200, 640x256 and larger Workbench screens, open Beam68 and verify the popup remains below the Workbench screen title bar and inside the screen. Resize with the bottom-right native size gadget, check refresh correctness and resize limits. Close/reopen and verify temporary dimensions are retained until Beam68 exits.
22. In Beam68Prefs -> Appearance, set `WINDOW_WIDTH=400` and `WINDOW_HEIGHT=120`, Use, hide/reopen the launcher and check dimensions. Save, reboot and verify ENVARC: persistence. Restore `WINDOW_HEIGHT=0` to autosize. Check comment and unknown-key preservation after saving.
23. Search for a long name, move cursor Left/Right, use Ctrl+Left/Right for beginning/end, insert in the middle, press Backspace and Delete. Check cursor tracking with >60 characters in a narrow window. Confirm typing `=12*(4+2)` still calculates immediately after edits.
24. Resize the launcher to show only one or two result rows. Use Down/Up through eight configured search matches and verify that the list scrolls with the highlighted selection. Single-click visible entries and double-click an application; check opening drawers with Tab followed by Enter still works.
25. Inspect display on alternate Workbench palettes and custom font sizes. The launcher uses `GetScreenDrawInfo()` to pick `TEXTPEN`, `FILLPEN`, `FILLTEXTPEN` and `BACKGROUNDPEN` and should remain readable on custom pens and low-depth screens. Confirm screen color changes while a popup is open are not assumed live (close/reopen to refresh pens). ASCII-only editing remains intentional for now.
26. Set `ICON_PREVIEW=YES` in Beam68Prefs -> Appearance. With a standard 4-color planar Workbench icon, select several entries and confirm a single preview appears in wide windows and never obscures the text or footer. Confirm `ICON_PREVIEW=NO` disables icon I/O; test small windows, icons over 48x48, corrupt `.info`, missing shell icons, NewIcons/ColorIcons/PNG icons, and RAM-disk/floppy-backed icons. After many hide/show cycles, verify no `DiskObject` leaks. Test on 68000 with a real v47 `icon.library` and alternative icon.library versions. Check that only the selected icon is loaded.
27. Test custom Workbench palettes where text/background/highlight pen numbers are non-standard and where background is nonzero. Verify caret, title, result rows, selection text and status remain legible. Also test switching public screens after hiding the launcher; each popup should obtain/release a fresh `DrawInfo`.

**Native 68000 binary and actual Intuition/GadTools testing remains required.** Portable geometry/edit tests do not verify the 3.2 NDK, graphics clipping, event delivery, mouse timings or screen layouts.

## Favorites / abbreviation checks (still version 0.1)

28. With an indexed app selected, press F6. Verify both `ENV:Beam68/Favorites.txt` and `ENVARC:Beam68/Favorites.txt` are readable ASCII, and that unpinning works and preserves comments and `.bak` files.
29. Clear the search query. Only pinned indexed apps should appear, in pin order. Launch, open drawers, F6 unpin, mouse navigation, and optional selected-icon preview must work.
30. Test Beam68Prefs Favorites on 640x200: ASL Browse, Add, Remove, Cancel, Use (ENV only), and Save (both). An unrelated Preferences Save must not clobber a new F6 pin.
31. Test hand-edited comments, duplicate or invalid paths, a full 48-item list, read-only floppy/ENVARC failures and manual F3 reload.
32. Test `dp` -> Deluxe Paint, `wb` -> WorkBench and prefix/exact-name ranking with history/favorites bonuses. Unmatched pinned apps must never appear for nonempty queries.
33. Compile and run the new code on an actual AmigaOS 3.2 m68000 setup; host tests alone do not verify native GadTools, raw keys or AmigaDOS semantics.

## Plugin preferences and native build preflight (still version 0.1)

34. Add three plugin manifests and disable the middle one through Beam68Prefs.
    Check the UI shows `[OFF]`, use/save writes `PLUGIN_DISABLED=` without
    deleting `PLUGIN=`, and the launcher will not register the trigger.
35. Re-enable it without re-adding the manifest. Check reload from F3,
    Beam68Ctl RELOAD, and automatically after Prefs Save. Confirm disabled
    plugins with invalid/missing executable paths are skipped quietly while
    invalid *enabled* plugins are reported on reload.
36. Manually reorder `PLUGIN_DISABLED=` ahead of `PLUGIN=` in ENV/ENVARC,
    test mixed-case duplicate disabled lines, remove a manifest, and save:
    no stale disabled line should be written for a missing registration.
37. Run `make check-toolchain` from a host with actual AmigaOS 3.2 NDK headers,
    then `make`, verify all binary headers and the 68000 ISA, and run all GUI
    flows on a 640x200 screen and a 1MB-capable setup. The preflight only
    checks tool availability/headers; it does not prove binary correctness.


### ARexx host port verification (Beam68 0.1)

1. With ARexx available, run Beam68 and confirm `SHOW('P','BEAM68')` is true.
2. Run `RX scripts/Beam68Status.rexx`. Verify that the script exits and the
   `STATUS`, `PLUGINS`, and each `PLUGIN n` line are correct.
3. Disable one plugin in Preferences and press Save; verify the same manifest
   reports DISABLED without adding a load error.
4. Configure two manifests with the same trigger; verify the later one reports
   TRIGGER_CONFLICT while the first remains ACTIVE.
5. Run `HIDE`, `SHOW`, `REINDEX` and `RELOAD` from ARexx; verify the hotkey and
   scan remain responsive. Run `QUIT` and verify the Rexx script gets a reply.
6. Boot with ARexx unavailable and verify the Commodities launcher, hotkey
   and Preferences still work (only scripting should be unavailable).
7. Verify port name collision does not replace or delete another host's port.

## Plugin Preferences and catalog validation (0.1)

On a stock AmigaOS 3.2 install using SYS:Utilities/Beam68 and SYS:Prefs/Beam68Prefs:

1. Open Plugins in Beam68Prefs. Verify Calculator, Clock and Memory have readable names and printable triggers, not only `.plugin` paths. Check their state is `ACTIVE` if the executables are installed.
2. Select Clock, press **Details** and verify its executable, description and `ENVARC:Beam68/Plugins/Clock.cfg` path. Press **Settings** and verify C:Ed opens the existing ASCII file. Change `SHOW_SECONDS=YES`, save and run `@` in Beam68; the display should contain seconds. Set `NO`, retest.
3. Disable Clock, confirm `OFF`, and verify another enabled plugin can own its trigger. Enable it again, then use Up/Down to change priority and confirm the first enabled trigger wins.
4. Temporarily rename the Memory executable; confirm `EXECUTABLE_MISSING` and no crash. Restore it. Add a plugin sharing `@` with Clock; the later valid one should show `TRIGGER_CONFLICT`.
5. Move Beam68 and plugins to a different drawer; update `INSTALL_DIR` in Beam68.cfg in ENV:/ENVARC:, then reopen Beam68Prefs. Its plugin diagnostics should find executable files in the new location while the launcher still runs with its own PROGDIR:.
6. In General, verify Start is enabled only while Beam68 is stopped, and
   Restart/Stop only while it is running. Confirm Restart replaces the resident
   process and Stop removes it and releases the global hotkey.
7. In Appearance, cycle through each theme and confirm the selector, details and live preview stay in sync. Save Mocha, reopen the launcher, and verify its background, text and selected-row colors change; Workbench must restore the public-screen pens.
8. With shell commands enabled, enter `shell:dir SYS:` and verify a separate output console opens. Disable the option, reload, and verify the same query is refused.
6. Verify the plugin view on a low-resolution 640x200 Workbench, including listview, diagnostics, enable/disable, ordering, timeout, ASL Browse and Ed startup.
7. Confirm unknown manifest keys are ignored, invalid or overlong path fields never execute shell content, plugin Settings uses Ed with a quoted absolute path, and an invalid `SETTINGS=` field rejects the manifest.

Native compilation and target-system verification remain required.

## Applications, aliases and actions (0.1)

1. Add `SYS:System/Shell` with keywords `terminal cli console`, action label
   `Open CLI`, and command `{path}`. Use `terminal` in the launcher and confirm
   Shell is selected even though its real name does not contain that word.
2. Press Tab through Launch, Open drawer and Open CLI; verify a fourth Tab
   returns to Launch. Run Open CLI and verify the quoted executable starts.
3. Test `C:List {drawer}` with an application inside a path containing spaces.
   Incomplete label/command pairs and overlong records must be rejected.
4. Verify Use writes ENV only, Save writes ENV and ENVARC, and the older
   two-column path/keywords alias format remains readable.


## Timeout and scan diagnostics (still 0.1; native verification pending)

1. On a licensed AmigaOS 3.2 environment, open the Plugins Preferences page,
   set `PLUGIN_TIMEOUT=5`, press Use,
   close and reopen, and verify the value persisted to ENV.
2. Make a test plugin intentionally slow. Confirm that Beam68 remains
   responsive to the hotkey and Escape, reports timeout after roughly 5 sec
   of poll time, and does not mistake any late output for the next invocation.
3. Set the timeout to 120, test persistence using Save, and verify the setting
   in ENVARC. Check values 0, 4 and 121 are rejected by Preferences.
4. Start `REINDEX` via ARexx, query `INDEX` repeatedly, verify the root and
   application counters change. The command should not change scan activity.
5. Configure a missing directory, reindex, and confirm `unreadable>=1` with
   cache invalidation; verify no `ERROR_NO_MORE_ENTRIES` false-positive.
6. Configure a path longer than the index path capacity and confirm `overlong`
   increases, with no partial cache retained. Remove the bad path and rescan.
7. Enable cache, run one clean scan, restart, and verify `cached=1`; force F2
   and confirm `cached=0` and fresh scan statistics.
8. Test disk read errors (e.g. unavailable floppy) and 68000 performance,
   without assuming `ExNext()` itself is asynchronous. Timer work only bounds
   the number of entries processed per update.

## Workbench drag-and-drop verification (Beam68 0.1)

1. Launch `SYS:Prefs/Beam68Prefs` while Workbench is active and switch to **Indexing**. Drag a Workbench drawer icon onto the Preferences window. Verify its full drawer path appears in the list, and that `Cancel` discards it, `Use` writes to `ENV:`, and `Save` writes to `ENV:` and `ENVARC:`.
2. Drag a normal file onto **Indexing**; it must be rejected. Drag a `.plugin` manifest to **Plugins** and verify it can be enabled/disabled; another file type and a drawer must be rejected. Drag an indexed Workbench tool icon onto **Favorites** and verify the new pin is shown after saving and the launcher reloads.
3. Drag two icons at once; verify both are accepted when capacity permits, duplicates are ignored, and rejection beeps do not crash or leak memory. Test very long paths, invalid file icons and deleted drawers.
4. Switch between all six pages repeatedly; verify each AppWindow is unregistered before its Intuition window is closed and is re-registered when returning to a drop-enabled page. Close Preferences with a Workbench drop queued; each AppMessage must be replied, and Workbench's WBArg locks must not be unlocked by the application.
5. Run Preferences without Workbench or without available AppWindow registration. Verify ASL Browse, manual Add/Remove and Use/Save still operate without drag-and-drop.
6. Run `make native-syntax` with an AmigaOS 3.2-compatible 68k compiler/NDK and resolve API signature errors before doing target-system checks. **Not run in this host environment.**

## Native reliability regression checks (version remains 0.1)

1. Set MAX_ITEMS low and index a directory containing more candidates
   than the cap. Confirm the `INDEX` report says `capped=1`, and
   `PROGDIR:Beam68.index` is not retained as a complete cache.
   Raise the limit and restart; verify a full rescan occurs.
2. Verify a successful, uncapped scan writes the cache and a clean
   restart can load it. Test disk-read errors and missing roots too.
3. Invoke all 13 bundled native plugins repeatedly and verify Protocol 1
   text, error and selectable-result messages. Check previews, cancellation,
   timeout and late-message rejection without a text-file fallback.
4. Test the HelloRexx script's Protocol 1 text transport separately. Verify
   atomic publication and refusal of empty, truncated or unterminated replies.
   A native manifest declaring VERSION=2 must be rejected.
5. Reinstall on a system with a previously running Beam68 after first
   stopping it via Exchange or Beam68Ctl QUIT. Re-run the installer
   with one required build product removed: it should fail before any
   copy. Verify existing ENV:/ENVARC: files remain untouched.
6. Run `make native-syntax` against a real 3.2 NDK (all translation
   units) and build/link with `-m68000`; verify on supported AmigaOS systems.
   Host tests alone cannot prove native ABI or runtime correctness.

## Keyboard shortcuts (0.1 native verification)

- On the target Amiga keymap, test the F1/Help searchable command browser,
  example insertion with Enter, filtering, Ctrl+U and
  Ctrl+Backspace search clearing, Shift+Up/Down page movement and
  Ctrl+Up/Down first/last result selection. Verify no duplicate key events,
  and that an empty result list is handled safely.
- With at least six results, press Up on the first row and Down on the final
  row; selection must wrap. Check PageUp/PageDown and Home/End, then refine a
  query and confirm the previously selected application remains selected if
  it is still present.
- Search using two separate terms that occur in one application's name,
  aliases or keywords. Then test insertion, deletion, substitution and
  adjacent-letter transposition typos; each one-edit query should still find
  the expected application without outranking an exact match.
- In General, verify the Launcher field and button enablement across Stopped,
  Starting, Running, Restarting and Stopping, including a failed or timed-out
  start.

## Scoped search, screens, access and theme files (0.1)

1. Index mixed tools, files and drawers. Verify each scope returns only its
   declared kind; test case-insensitive prefixes, blank scoped queries,
   filename `*`/`?` filters, excluded path boundaries and cache reload.
2. Move between two named public screens, resize/reposition on both, hide and
   reopen, and restart Beam68. Verify each screen restores its own valid
   geometry and a low-resolution screen selects the compact row count.
3. Verify custom font fallback, font size, row spacing, high-contrast
   selection, larger Preferences gadgets and reduced-color palette fallback.
4. Add Tools, Games, Internet and Development favorites. Verify `fav:games`
   accepts an optional trailing query and legacy ungrouped lines load as Main.
5. Open F1, search commands/plugins/aliases/shortcuts, move with Up/Down and
   press Enter. Verify the example is inserted without executing it.
6. Inspect plugin capabilities in Preferences. A `SYSTEM` plugin must require
   a second unchanged Enter; edits or Escape cancel. Other declarations remain
   informational and protocol-1 manifests without them still load.
7. Export a custom palette, import it, and drop it on Appearance. Corrupt the
   version, color count and hex values; the current palette must remain active
   and a failed save must restore its previous file.


## Audit fixes: native verification still required

These changes compile cleanly for `-m68000` against the OS 3.2 NDK headers
and pass all portable host tests, but were not run on an Amiga. Verify:

1. Build with `scripts/build-amiga.sh` (vbcc now uses `-O2`; fall back to `-O`
   if anything misbehaves).
2. From a Shell with `Stack 4096`, run Beam68 and Beam68Prefs directly and via
   F5 / Start / Restart: no crash, plugins (FileFinder, ARexxRunner) work.
3. Start Beam68, wait 10 s, reset the machine: the next boot is NOT safe mode.
   Crash it within 8 s of starting (e.g. kill from a debugger): the next start
   is safe mode once, the one after is normal. `restart amiga` never leads to
   safe mode.
4. Type quickly on a 68000 profile with 512 items: no lag build-up; Up/Down
   and Left/Right redraw without full-window flicker.
5. F2 and saving a file into an indexed drawer: results stay visible during
   the rescan; only the changed root is rescanned.
6. `PROGDIR:Beam68.index` is written; old `ENVARC:Beam68/Index.txt` is gone.
7. Choose a FONTS: font with the new Choose... button: the launcher uses it.
8. Move the launcher on Workbench and on another public screen: positions are
   restored per screen.
9. Exchange: Disable makes Ctrl+Alt+Space reach other programs again.
10. OS 3.1 (workbench.library V40): launch tools and projects; a drawer result
    reports that V44 is required. Quit Beam68 while a launched tool still runs.
11. ClipboardTools `clip:show`, then copy text in another program: no hang.
12. MusicControls with a frozen player returns within ~10 s.


## Plugin packages: native verification

1. Fresh install with `Execute scripts/Install-Beam68`: all 13 plugins load
   from `Plugins/<Name>/`. Upgrade over a 0.1 install with the flat layout:
   the old files are removed and the existing `Beam68.cfg` keeps working.
2. Build the SDK example (`sdk/Example`, `make vbcc`), `make package`, copy
   the archive to the Amiga. With `C:LhA` installed, drop it on the Plugins
   page: consent shows name/version/author/capabilities; after Use it runs.
3. Repeat with a `.zip` and `C:UnZip`, and with an unpacked drawer via Add.
4. Without `C:LhA`: a clear message, nothing changed.
5. Bump `PLUGIN_VERSION` and reinstall: reported as an upgrade, replaced in
   place, settings in `ENVARC:Beam68/Plugins/` kept. Install an older
   version: reported as a downgrade.
6. A package whose trigger is taken installs disabled.
7. Hand-craft archives with `../x` and `/x` entries: refused before
   extraction. A package with `COMMAND=PROGDIR:...` or a non-HUNK
   executable: refused.
8. Remove → Delete files: the drawer and its `.info` are gone.
9. Copy a package drawer into `Plugins/` by hand and open Preferences: it is
   listed, disabled.


## GUI installer: native verification

Run each with Installer 42.x (OS 3.1/3.2) and, if available, Installer 43.3:

1. Fresh system: double-click **Install**, keep defaults. Beam68 is in
   `SYS:Utilities/Beam68` with 13 plugin drawers, Beam68Prefs in `SYS:Prefs`,
   `SYS:WBStartup/Beam68Start` points at Beam68 (check its Default Tool), and
   "Start it now" opens the hotkey launcher.
2. Install into `Work:` with 5 plugins: only those 5 are listed in
   `ENVARC:Beam68/Beam68.cfg`, it contains `INSTALL_DIR=Work:Beam68`, and
   Beam68Prefs finds the plugins.
3. Update with the launcher running: it is stopped, binaries replaced, the
   user's settings, icon positions and third-party plugins kept; plugins
   newly selected are appended once to both ENV: and ENVARC: configuration.
4. Update over a 0.1 install with flat `Plugins/X` files: replaced by drawers.
5. Update and answer "No" to autostart: the WBStartup entry is removed.
6. Pretend mode makes no changes.
7. Uninstall keeping settings: everything except `ENVARC:Beam68` (minus
   `InstallDir`) is gone. Uninstall deleting settings: `ENVARC:Beam68` and
   `ENV:Beam68` are gone too.
8. "Version Beam68" in a Shell prints `Beam68 0.1 (27.10.2026)`.


## WHDLoad, languages and manual: native verification

1. Add a games drawer with WHDLoad installs (Games/A/Name/Name.info +
   .slave): `game:` lists them as GAME; Enter starts them with the icon's
   tool types. Delete one game's icon: the .slave appears and starts via
   WHDLoad (check that `DATA=` finds the game data).
2. On OS 3.1 (workbench.library V40) the same game icons start.
3. Locale prefs set to Deutsch, then Svenska: launcher status lines, footer,
   badges and F1 help appear translated, umlauts/å ä ö display correctly in
   topaz. Remove the catalog: English appears.
4. MultiView opens Beam68.guide; every link works. In Beam68Prefs, Help on
   each page opens the matching node.


## Fixes and features round: native verification

1. Toggle "WHDLoad games" and press Use: games appear/disappear at once.
2. `Beam68Ctl STATUS` returns 0/5; the installer update stops a running
   launcher via QUITWAIT; with a requester open it aborts with a message.
3. Install an LHA plugin made by Amiga LhA (levels 0, 1 and 2 if possible)
   and a hand-made one with `../` names (refused before unpacking).
4. Games show their full title; a game needing a missing kick image warns
   and starts on the second Enter; `game:random` changes per query.
5. Tab to "Run with arguments" on `Ed`, type a file name, Enter: Ed opens
   it. Information opens the Workbench Info window; Copy path pastes in Ed.
6. Amiga+V, Ctrl+1..9, mouse wheel (OS 3.2 / NewMouse), underlines on
   topaz and a proportional font; `QUICK_LAUNCH=control alt` global keys.
7. A plugin with `GUIDE=`: Help on its row opens the guide.


## Languages, browsing and Protocol 1 selectable results: native verification

1. Locale Deutsch/Svenska: every Beam68Prefs page, button and requester is
   translated and fits (Start/Restart/Stop, Use/Save/Cancel, Add/Remove,
   Details column, Accessibility labels at 640x256 with topaz 8).
2. Run the Installer with Locale Deutsch and Svenska: all prompts translated.
3. Search a drawer (e.g. "utilities"), press Right: its contents list; type
   to filter; Enter opens a tool; Left goes up; Esc returns.
4. `find:readme` shows up to eight rows; Enter opens one in MultiView;
   Esc returns. Start the same FileFinder from an older 0.1 launcher: one
   line of text (fallback).


## Calculator, documents, volumes, hiding, Opus, plugin cache: native verification

1. `=$FF+%1010` previews `265 $109 %100001001`; Enter gives the same.
2. Open a text file via browse or a file result, then `doc:` lists it.
3. `volume:` shows DF0:, RAM:, Work: and assigns (LIBS:, ENV: ...); mount a
   disk and type again: it appears. Right browses an assign.
4. Shift+Del on a result: it disappears; `hidden:` shows it; Shift+Del there
   restores it; survives a restart.
5. With DOpus 5 running: Tab offers "Show in Directory Opus" and it opens a
   lister; with DOpus 4 the ScanDir fallback works; without Opus the action
   is skipped. `OPEN_DRAWERS_IN_DOPUS=YES` opens drawer results in Opus.
6. `PLUGIN_CACHE=YES`: inspect the instance/request-slot drawers under
   T:Beam68/PluginCache. Executable and supporting-file edits refresh the copy;
   measure startup on the target storage because full validation adds reads.


## Fixes and tidy-ups: native verification

1. `drawer:` with many matches, browsing SYS:C and `volume:` show more than
   the visible rows; Down scrolls to entry 32; mouse clicks hit the right row.
2. Locale Deutsch: `find:xyz`, `?`, `=1/0`, `system:` usage texts and
   `Beam68Ctl` (no arguments) are German; `Beam68Ctl STATUS` still prints
   running/stopped.
3. Install a package with `COMMAND=PROGDIR:x`: the refusal is translated.
4. Plugin Details show "Protocol: 1" for File Finder and every bundled plugin.


## Tidy-ups, performance and community: native verification

1. Extras page at 640x256 (English, Deutsch, Svenska): nothing overlaps;
   values survive Save; QUICK_LAUNCH change takes effect after Use.
2. `rx "address BEAM68 'TIMING'"` after a search: plausible times; type
   "multi" letter by letter and see `narrowed=` increase; results are the
   same as without narrowing (compare with a fresh query).
3. Boot with a cold cache: first popup loads plugins (status OK), later
   popups are instant.
4. Shift+F2 empties then rebuilds; F2 keeps results while updating.
5. Install the HelloRexx example (RexxMast running): `;Amiga` answers, the
   preview appears while typing; with RexxMast stopped a clear message.
6. `dev:hex:255` shows its answer before Enter.
7. Import a plugin archive on Plugins; verify its settings and metadata are available.
8. Edit colours: sliders move the swatch; OK updates the custom palette.
9. Catalogue: build with `make catalogue`, serve the files over HTTP,
   install wget, Load the index URL, install a plugin and a theme. Without
   wget: a clear message. A local index path works without wget.


## Index sources and media: native verification

1. INDEX_MODULES=YES with a modules drawer as DIR: `mod:` lists them;
   Enter plays in HippoPlayer (and with MODULE_COMMAND changed to
   EaglePlayer or AmigaAMP).
2. BOOKMARK_FILES pointing at a NetSurf hotlist and an IBrowse HTML export:
   `web:` finds bookmarks by title; Enter opens them via OpenURL.
3. `snip:sig` + Enter, then Amiga+V in an editor pastes the text.
4. `script:` lists REXX: scripts; Enter runs one in a console.
5. `prefs:` shows only SYS:Prefs programs; `stats:` orders by launches.
6. Edit Snippets.txt, press F2: changes appear; restart: still cached.


## Sources fixes: native verification

1. HippoPlayer running: `mod:` + Enter loads into it (no second copy). Not
   running: it starts. With EaglePlayer/AmigaAMP configured on the Sources
   page, the same.
2. A hotlist with two links per line, a link split over lines, and a plain
   text file of URLs: all appear under `web:` with sensible titles.
3. Remove SYS:Prefs from the indexed drawers: `prefs:` still lists Locale,
   ScreenMode, Beam68Prefs ...
4. Sources page at 640x256 and on an NTSC 640x200 screen (two-column
   section buttons), in English, Deutsch and Svenska.

## Version 0.2 responsiveness

- Type ahead while indexing: the complete query should paint before ranking;
  raw key releases and modifiers must not trigger intermediate searches.
- Type a query and immediately use Enter, arrows, Tab or Shift+Delete: the
  action must apply to results for the latest query.
- During an initial scan, search for an application discovered early: its
  result should appear on the next scan update, without the normal 500ms
  refresh throttle while the result list is empty.
- Enable icon previews and type or change selection rapidly: results should
  appear before the optional icon loads; no old icon should appear for the
  newly selected item. Close the window before the preview timer fires.
- Compare ARexx TIMING search readings and typing on a real 68000 with the
  same index and configuration. Host benchmarks do not measure native UI
  latency or bound the duration of an individual filesystem operation.

### Startup and memory checks for 0.2

- Launch twice: the second invocation exits without reading configuration or
  loading the index, leaving the resident instance available.
- Check startup TIMING on the same machine and index; note that 0.2 now measures
  library/configuration/metadata loading previously omitted from the metric.
- Start with a small and then a large index; verify pool growth preserves
  bookmark titles, WHDLoad names, hash lookups, cache reload and rescans.
- Repeat on low memory: a failed pool expansion must preserve existing results
  and must not save a partial scan as a complete cache.
- Open and close the launcher using a disk font and with diskfont.library
  unavailable: font fallback and cleanup must continue to work.

### Low-memory and rescan recovery in 0.2

- Run the portable item-index/cache tests with allocation failure injection.
  Initial allocation must clean up; growth/title failures must preserve old
  paths, titles, hashes and selections, and a later retry must succeed.
- Reindex while a drawer is unreadable or memory is exhausted. Existing entries
  not revisited must remain searchable, and no incomplete scan may be cached.
- Select an item after an entry that will be deleted, then finish a clean scan:
  selection must remain on the same surviving item after index compaction.
  Repeat while entering arguments; a removed target must not become another app.
- Change a game/bookmark title and rescan. Confirm search metadata, displayed
  text, cache reload and subsequent compaction reflect the new title.
- Inject cache write, backup rename and install rename failures. Verify the
  previous cache remains loadable after rollback. Corrupt or mismatched reloads
  must leave an already populated live index intact.

### Plugin lifecycle in 0.2

- Start a delayed reply for query A, edit to B, then back to A. Deliver A's
  reply after starting the new request: it must not paint rows/status or
  finish the new request. Repeat with previews and atomic ARexx text replies.
- Type while a plugin sleeps, crashes or returns nothing. Ordinary search
  and navigation remain usable; a new query can issue a fresh request.
- Set PLUGIN_TIMEOUT=5. Delay timer delivery and send a reply after five
  seconds: it must expire based on elapsed time, not the number of ticks.
- Send zero/excess rows, unknown kinds/version, invalid flags, unterminated
  buffers and control characters. Current malformed requests fail cleanly;
  old malformed requests cannot affect the current one. Test empty, incomplete
  and multiline published text files as well.
- Close the window, reload plugins or apply valid settings during a request.
  Reopen/type and verify late replies cannot restore old rows or previews.
- Repeatedly fail a preview and confirm the existing quarantine threshold
  applies. Confirm health saves persist after idle time and clean shutdown.
- Repeat request races on AmigaOS; portable state/validation tests and native
  compilation do not substitute for OS message-port scheduling coverage.

### Upgrade from 0.1 to 0.2

- Exercise both installers with existing portable Beam68.cfg/Aliases.txt and
  edited bundled themes. Their bytes must remain unchanged; missing files
  must receive defaults. Keep separately edited ENV and ENVARC files intact.
- With ENVARC settings absent, seed from the installed portable configuration;
  with ENV absent, seed from ENVARC. Preserve comments, unknown keys, custom
  plugin manifests and disabled-plugin choices.
- With no ENVARC plugin settings, inherit an installed package-local or legacy
  flat plugin .cfg before using shipped defaults. Existing ENVARC plugin
  settings always win. Verify third-party plugin drawers remain untouched.
- Load legacy and grouped favorites, usage/query history, flat-layout plugin
  health records, aliases and custom theme files. Save and reopen using 0.2.
- Run the GUI Installer and Shell script on a copy of a 0.1 installation to
  verify actual Installer/Shell execution and stopped-resident upgrade behavior.
  Host procedure tests and format fixtures do not execute AmigaOS Installer.

### Per-plugin preview controls in 0.2

- Select a PREVIEW=YES plugin, open Details -> Preview and toggle it off.
  Use: typing must not launch a preview, while Enter still runs the plugin.
- Set a longer delay, Save, close/reopen Preferences and restart Beam68.
  Verify the delay and disabled state persist independently for each plugin.
- Reorder, remove and add plugins; overrides must follow the same manifest
  and a new plugin must not inherit a removed plugin's settings.
- Check Default restores enabled/400 ms. A PREVIEW=NO plugin must explain
  that previews are unavailable and remain manual-only.
- Check 100 ms and 5000 ms limits, delayed configuration keys before PLUGIN,
  old flat paths, invalid delay text and stale unknown-manifest overrides.
- Verify requesters on 640x200 and larger displays in English, German and
  Swedish. Confirm Cancel without Use/Save leaves saved settings untouched.

### Bounding outstanding plugin requests in 0.2

- Start two delayed previews and edit away from each. No third automatic
  invocation may start; manual Enter must still have one slot available.
- Fill all three slots, then type/edit/close/reopen/reload repeatedly. Ordinary
  search remains usable, and no extra plugin request starts at capacity.
- Let a request time out: the visible job ends but outstanding count does not
  fall until completion. A timed-out process must not create unlimited retries.
- Deliver old native replies in reverse order. Each releases only its serial;
  duplicate replies cannot release a newer request or restore obsolete rows.
- Repeat with atomic ARexx final files while the window is hidden. Capacity
  should return through bounded idle polling, with no partial-file deletion.
- Fail process startup and confirm no slot leaks. Send truncated/wrong-version
  packets and check capacity remains held until recognizable completion.
- While previews are waiting, change the query or disable/reload the plugin.
  Only the current eligible query may run after capacity returns.
- Verify on AmigaOS that protocol completion and process cleanup behave as
  documented; the portable stress test does not supervise actual child tasks.

### Interrupted cache replacement in 0.2

- With a valid primary, leave partial .new and older .bak files: primary wins.
- Simulate shutdown after primary moved to .bak (or .old when .bak exists):
  startup must recover the latest validated committed candidate, not .new.
- Corrupt primary and .old, retaining valid .bak: recover titles, paths and
  kinds from .bak. Reject mismatched, truncated and checksum-invalid backups.
- Leave only a complete .new: rebuild normally because it was never committed.
- Inject failures in both promotion renames. The backup must remain available;
  validated data can load in memory even when filename repair fails.
- Inject index/pool allocation failure with a valid smaller backup present.
  Do not promote/delete files merely because the primary could not fit in RAM.
- Test actual abrupt shutdown/restart on a disposable AmigaOS installation;
  host tests simulate rename states and do not certify filesystem power safety.

### Outstanding-request visibility in 0.2

- Start preview and manual requests; inspect REQUESTS and REQUEST 1..3.
  Verify plugin names, mode, transport, launch-time timeout and increasing age.
- Edit, close and reload while work remains outstanding. Captured names must
  survive reordering/reload, and state must change from active to detached or
  timedout without implying that the OS process exited.
- Deliver a completion, then reuse that slot for another plugin: the new serial
  and name must replace the old values; a freed slot must report only free.
- Fill capacity and verify the launcher footer identifies holding plugins and
  Beam68Status.rexx prints all slots, including holes. Confirm diagnostics do
  not launch, cancel, release or otherwise alter outstanding work.

### Direct preview-delay entry

- Open Plugins -> Details -> Preview -> Delay...; type 750 and accept with
  OK, then repeat with Return. The outer dialog must show 750 ms.
- Test 100 and 5000, leading/trailing spaces and leading zeroes. Reject blank,
  99, 5001, negative, fractional, suffixed and oversized numeric values without
  altering the previous value. Invalid text remains editable for correction.
- Type another value then Cancel, Escape or close the requester. The earlier
  value must remain. Tab must not unexpectedly accept the field.
- Change the delay while previews are disabled: they must stay disabled.
  Use/Save, reopen Preferences and confirm the exact numeric value persists.
- Verify allocation failure cleanup and the requester on 640x200 and larger
  screens, in all three languages. Native interaction verification is required;
  parser tests and a linked build do not establish focus/keyboard behaviour.

### Isolated native 0.2 checks

Build with BEAM68_CC set and `sh scripts/build-native-qa.sh`. The resulting
build/Beam68ReleaseQA reserves a fresh T:Beam68QAxx drawer and never reads or
writes active settings. It exercises native allocation failure/retry, title
preservation, cache backup recovery with a failed promotion, request admission
limits and preview-delay parsing. A failed check reports its line and returns
20; the test index allocations are freed on every helper exit. Test files are
retained in the reserved drawer for inspection.

Beam68RexxProbe can query STATUS, VERSION, HELP, INDEX, TIMING, PLUGINS,
PLUGIN 1..32, REQUESTS or REQUEST 1..3 on a running launcher without RexxMast.
It rejects state-changing commands before sending them. Compile success is
not a native runtime pass. The direct-entry dialog still requires interactive
checks of focus, Return, Tab, Escape, Cancel and invalid-value feedback.

### Aminet Browser in 0.2

- `make host-test` covers path traversal/injection rejection, URL encoding,
  bounded search/readme parsing, six-result paging, malformed/truncated HTTP
  headers, response size limits and archive signature checks.
- `scripts/build-native-qa.sh` also builds Beam68AminetQA. Run it explicitly
  on an online test system: it reserves T:Beam68AminetQAxx, searches Aminet for
  Beam68, fetches a readme and downloads (without executing) its archive, then
  checks response-size rejection and cancellation cleanup. Results remain in
  that fresh drawer. It never reads or writes active Beam68 settings.
- Interactive checks: search and readme next/previous rows, details, download
  confirmation Cancel, close-to-cancel during transfer, one-download guard,
  disk-full recovery, existing destination refusal and archive-handler failure.
- Do not treat download completion as successful application installation.
  Installation preparation opens an archive; follow its own installer afterward.

### Preferences About and running version in 0.2

- About... is available on every page; F2 opens the same requester.
- Compare Preferences version with a running launcher reporting a different
  version. The returned value must be shown, never the local build as a guess.
- A stopped launcher says Not running; an older launcher without the query or
  a busy launcher says version unavailable after a bounded 25-tick wait.
- A timed-out request must not reference a freed reply port. Late replies,
  malformed requests, allocation failure and serial mismatch are covered by
  test_control; the normal launcher query is also checked in native runtime.
- Verify the About layout on small screens and all three languages.

### Full Shell installation and update in 0.2

- Update a legacy flat installation with a separate data drawer, then repeat
  the update while 0.2 is running. Both must complete and report running 0.2.
- Compare user settings, aliases, snippets and themes byte-for-byte. Preserve
  additional plugins and existing custom icons.
- Reuse the remembered application directory; migrate the legacy flat settings
  only when the canonical settings file is absent.
- Check a fresh installation and an explicit destination containing spaces.
- Retarget known old startup entries; preserve custom startup commands and keep
  autostart disabled when updating an installation without a startup entry.
- Refuse to copy when the running launcher does not acknowledge shutdown.
- Treat a failed launch or unanswered running-version query as an installation
  error instead of reporting success.

### Preferences exit and search discovery

- Run Beam68StackQA from a 4096-byte Shell: repeated private-stack calls must
  preserve arguments, return values and original task stack bounds.
- Open and close Preferences from both Shell and Workbench. Require a normal
  return to the caller, not merely completion of library cleanup.
- Search for preferences and settings with Beam68Prefs indexed; verify both
  ranking and the character-mask filter accept its built-in search words.
- Fresh defaults enable System Commands and leave Aminet Browser disabled.
  Updates retain saved plugin choices; a newly registered Aminet Browser is
  disabled until the user enables it.

### Interactive Installer QA package

Build with `BEAM68_CC=/path/to/m68k-amigaos-gcc python3 scripts/build-installer-qa.py`.
Open Beam68 Installer QA in the resulting `outputs/testing/Beam68-Installer-QA`
drawer. Install once with Novice, then repeat to verify update preservation.
Average and Expert retain plugin/startup choices; destination selection is
restricted to the QA tree. Novice uninstall removes test programs automatically and retains saved settings;
Average and Expert keep the removal and settings choices.
The report is `RAM:Beam68InstallerQA/report.txt`; the Installer transcript is
`RAM:Beam68InstallerQA/installer.log`.

After QA finishes, remove any staged `Builds:Beam68-Installer-QA` drawer and
its matching `.info` and `.uaem` sidecars from the host Builds directory.
Keep QA reports under `outputs/testing/`, not in Builds. The production
`Builds:Beam68` installer remains available for the user.

The generated copy redirects system, locale and settings destinations into
one dedicated RAM drawer, omits resident shutdown and does not start installed
applications. It exercises installer file operations; use the production
installer separately for application startup/shutdown and arbitrary destinations.
`CHECK` runs the native file checker from Shell without opening the menu.

## 0.4 regression checks

- `Beam68Prefs LAYOUTQA`: read-only native 640x200 gadget bounds/overlap check.
- `Beam68Prefs SETTINGSQA`: save/reload all declared bundled settings in a new
  temporary drawer; verify unknown keys survive. Normal Preferences is unchanged.
- `help`, `help:` and an enabled prefix such as `aminet:`: selectable examples,
  with no request launched merely by displaying suggestions. Preserve `?` Memory,
  `@` Clock and calculator trigger behavior.
- Missing TCP/IP or Wikipedia downloader: actionable local error; optional
  OpenURL/player requirements remain specific to the action that needs them.
- Ctrl+F2 / `scan:cancel`: retain results, close directory frames, clear pending
  scan work and do not persist incomplete results through game indexing.
- Published 0.3 to 0.4 graphical update: preserve config, aliases, snippets, edited
  themes, added plugins and autostart; verify actual running version and shutdown.

## Plugin import review and settings defaults

Build native `Beam68PluginImportQA` with scripts/build-native-qa.sh. Run it as
a synchronous command through the native test launcher in a disposable guest.
It uses a fresh T: tree and a test-only consent callback; production builds
always use the native review requesters. It checks both cancellation stages,
file review, initially disabled imports, duplicate-reference refusal, activation
preservation and rejection of a manifest changed during consent.

Host plugin-catalog tests validate canonical defaults and atomic rejection of
invalid defaults. In the dialog, check Restore defaults followed by Cancel, then
Restore defaults followed by Save for text, checkbox and choice settings.

## AmiHASS integration

Run `build/Beam68AmiHASSQA` for fake-service search, paging, read-only states,
confirmation cancellation, state confirmation, unconfirmed actions, unsupported
entities and missing-service handling. `build/Beam68AmiHASSTransportQA` exercises
the real SDK transport with an isolated local fake service: envelopes, concurrent
request rejection, malformed replies and abandoned late-response ownership.
Neither test contacts Home Assistant or controls real devices. For a live setup,
enable AmiHASS, search an entity, cancel an action, then explicitly confirm an
action on a suitable test device and inspect its reported state.

## Search, history, favorites and download completion

Host tests cover path selection after reordering/compaction, plugin row identity
with changing state labels, query-command safety, separate history files,
legacy history filtering, favorite-query pooling/cache round trips, and removing
an indexed query while preserving pending scan entries. Native syntax uses the
68000 target with warnings treated as errors.

`Beam68AminetCompletionQA` uses fixture transfers and simulated requester choices
in a disposable Workbench guest. It exercises interrupted transfer retry, archive
commit, all completion choices, companion readme fetch, cancellation cleanup and
cancelled initial consent; it never downloads or executes external code.

`Beam68AmiHASSLiveQA <explicit-light-entity> off` is opt-in and requires an
operator-authorized light that is initially off and a configured running service.
It checks live connection, discovery and states, briefly turns the light on,
then restores and verifies off even when an intermediate check fails. Never run
it automatically against an arbitrary entity. Credentials remain in the private
AmiHASS configuration; do not retain guest profiles or credential copies in
published test artifacts.

Run `Beam68PolishQA` directly with Amiga Workbench's isolated --test workflow.
It waits synchronously for QueryState, AminetCompletion, AmiHASS behavior and
transport QA programs and writes AutoRun:polish-proof.txt plus individual logs.
A shell Execute handoff by itself is not proof that its children completed.

The opt-in live test is also a direct executable. Its private Builds mount must
contain AmiHASS.cfg, AmiHASSService and curl. It copies these into the disposable
guest and starts that service; run it only in an isolated test guest, never in a
saved desktop. Retain redacted proof logs, then remove the disposable profile
and private credential copies. Use a current service with valid all-area
catalogue templates; old malformed templates fail discovery with HTTP 400.

## 0.5 result interface

Build the native test-only launcher harness with `make CC=/path/to/m68k-amigaos-gcc build/Beam68LauncherUIQA`
or `scripts/build-native-qa.sh`. Run it synchronously through Amiga Workbench’s
disposable --test guest. It includes the production renderer/event handler and
checks category clicks, query retention, action filtering, Tab/arrow/Enter,
Escape cancellation, row hit testing and compact fallback on 640x200/640x400.
Run Beam68Prefs LAYOUTQA for all Preferences control bounds and overlap checks.
Neither test requires QA files in the persistent Builds share.

Also review ordinary results visually, duplicate names from different drawers,
long names/paths, changing fonts and resized windows. Confirm that plugin rows,
completion acceptance and argument entry retain their existing controls. Selecting
a category must not run a command or send a network request. Cancelled action
menus must not launch anything. Clearing Two-line rows must persist through Save.


## Launcher feedback, ranking, small-screen settings and conversions

- Scan a drawer containing files that are not indexed: the current drawer and
  entry counter must still refresh. The bar counts completed roots only.
  Click the banner or Ctrl+F2 to cancel; retain existing results. Use
  scan:failures after unreadable drawers and confirm explicit retry.
- Query a disabled plugin prefix, then a missing executable: show the reason
  and direct the user to F5 Plugins. A successful empty reply means no matches;
  errors and timeouts retain explicit Retry without automatically replaying
  potentially mutating requests.
- Exact real names outrank aliases; prefixes outrank lower match classes even
  against frequent pinned items. Within a match class, favorites take precedence;
  usage is bounded. Equal scores sort by name/path across scan order changes.
  Cancel history:reset, then confirm it: retain pins and aliases, clear learned
  application usage and recents, including after restarting.
- Run Beam68Prefs LAYOUTQA 200, 256 and 400. Check disabled actions with no
  plugin selection, first/last priority rows, no-settings plugins and bundled
  plugin Remove. Open a settings plugin with more fields than fit the screen:
  navigate pages, edit each, Cancel without writing, then Save all pages. Stage
  Restore defaults and Cancel. String fields start focused and allow Tab cycling.
- Calculator: =1.5 MiB to KiB -> 1536 KiB; =90 min to h -> 1.5 h;
  =1 in to cm -> 2.54 cm; =1 GiB to B -> 1073741824 B;
  =1 s to min -> ~0.016666666666 min. Reject mixed dimensions and excess precision.
  Compare live preview and Enter output on a 68000-compatible build.


## 0.6 subtree, action and selection checks

- Use Beam68LauncherUIQA to create private sibling drawers, observe a real
  notification and scan one queued subtree. Confirm fewer entries visited,
  deleted/new files reconciled, siblings untouched and cancellation retained.
- Apply a session exclusion: immediately prune only its drawer and children,
  persist it in the disposable T: settings and retain siblings. Check volume
  roots, trailing slashes, full exclusion capacity and failed storage.
- Remove a selected indexed item and reorder/remove plugin reply rows. Enter
  must never silently operate on a replacement; explicit navigation acknowledges
  a new target. Changed labels with the same actionable value retain selection.
- OpenWithReceiver checks the actual WBStartup file argument and parent lock,
  including a missing-tool failure with no outstanding startup request.
- Compare PerformanceQA's 25-query, 2,000-item workload with fixed CPU, memory,
  screen configuration and font before/after optimizations. Timing is controlled-guest
  evidence, not a physical 68020 latency guarantee. Keep both raw logs.
- Verify =32 F to C -> 0 C; =-40 C to F -> -40 F; =0 C to K -> 273.15 K;
  =1 mm to km -> 0.000001 km. Reject negative Kelvin and incompatible units.
- Browse a plugin executable setting, Cancel its settings window and reopen:
  the previous path must remain. Save a chosen path, then check readiness.

## 0.6 cache, reload and offline-volume regressions

The native launcher UI QA also exercises:

- Two packages with the same drawer/executable names and different helper files,
  while the first request remains outstanding; cached paths and data stay isolated.
- Helper-only byte changes with the same size and timestamp, added nested files,
  removed helpers and unsupported/deep package fallback.
- Cancelled native replies arriving during a newer query, replies while hidden,
  three outstanding jobs and recovery after completion. Repeated detached atomic
  file replies retain their slots until publication and clean both final/partial files.
- Actual appearance configuration reload during indexing: retain the notification
  port, scan frames and plugin catalogue. Runtime changes cannot use that shortcut.
- A disposable assign removed between scans: retain old entries, report scan
  failure without a volume requester, reject unavailable launch targets and
  recover on explicit rescan after reconnecting the assign.

These fixtures do not certify a stalled network filesystem handler or physical
media removal. Unfinished third-party processes deliberately keep bounded slots;
there is no unsafe forced termination or deadline-based slot reuse.

## Large synthetic index benchmarks

Build Beam68PerformanceQA and Beam68LargeIndexQA with scripts/build-native-qa.sh.
The performance program accepts only 2000, 10000 or 50000 (default 2000).
The guarded runner invokes all three sequentially, checking each return code.
Use a disposable Workbench guest with 64 MiB of RAM for the complete stress run.
This test overrides MAX_ITEMS only inside its process; the released setting
remains capped at 2048. It does not create 50,000 actual filesystem entries.

Each workload fills the real compact index in reverse name order and runs 25
search/render samples, including broad, narrowed and no-match queries. Logs
record median/p95/max search and combined search+render latency, p95 rendering,
allocated index bytes, approximate free-memory delta and largest free block.
Synthetic build-to-ready and RAM-cache-restore-to-ready timings include the
launcher popup. They are not OS/process cold-start measurements, physical
key-to-display latency, disk scanning throughput or real-hardware certification.
The cache roundtrip must restore the exact item count. The runner writes its
proof and bench-2000.log, bench-10000.log and bench-50000.log under AutoRun:.

Use scripts/benchmark-summary.py --output summary.json with those three logs;
incomplete or failed runs are rejected. Compare timings on identical machine
settings; do not impose a universal timing limit across different CPUs.

## Case-sensitive packaging checks

scripts/check-path-case.py verifies each runtime path component against actual
directory entries even on case-insensitive development volumes. Plugin commands,
guides/icons, configuration registrations and fixed Installer source references
must use exact package spelling. Source includes are checked too. Release runtime
inventory validation enforces these references; ZIP/LHA writers and source
inventory validation reject case-colliding files or parent directories.

For an independent filesystem check, use a disposable case-sensitive APFS volume.
Confirm a mixed-case probe cannot be opened with lowercase spelling, extract the
actual LHA there, run check-path-case.py --runtime-only on its Beam68 drawer,
and copy source plus runtime inputs onto the volume. Run the source checker,
Installer and website tests, and a forced strict native build from that copy.
Record the archive checksum and outcomes; unmount the disposable volume afterward.
