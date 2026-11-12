# Beam68 0.1

A lightweight Spotlight/Launchy-style application launcher for **classic AmigaOS 3.x** (3.0 and later; 3.2 recommended) and **Motorola 68000+** computers (68020 recommended). Designed for Workbench, with a resident hotkey launcher and a separate native Preferences application.

One 68000 binary runs on every CPU from the 68000 to the 68060; no
CPU-specific builds are needed. On workbench.library V44+ (OS 3.1.4, 3.5,
3.9, 3.2) programs are opened through Workbench itself; on OS 3.0/3.1 Beam68
starts tools and projects with its own WBStartup launcher (opening drawers
needs V44).

## Features

- Search indexed applications, files, drawers, plugins, recent items and volumes by name, abbreviation, aliases or keywords. Use `app:`, `file:`, `drawer:`, `game:`, `plugin:`, `recent:`, `volume:` or `fav:<group>` to limit a query. Multiple terms and one-edit typo matching are supported.
- Global Commodities hotkey; active-public-screen placement; remembered size and position per screen; compact low-resolution layouts; configurable launcher font, row spacing, high-contrast selection, larger Preferences gadgets and reduced-color rendering.
- Incremental indexing with live drawer, entry and application counts; persistent text cache; configurable scan directories and limits; automatic smaller-index retries when memory is tight.
- Automatic AmigaDOS directory monitoring with debounced background index refreshes after filesystem changes.
- Separate **Beam68Prefs** for `SYS:Prefs`, with a nine-section two-pane sidebar, highlighted selections, press-to-capture hotkeys, native Browse, drag-and-drop, and state-aware Start/Restart/Stop controls.
- Distinct palette-safe Workbench tool icons for Beam68 and Beam68Prefs, each with normal and selected artwork.
- Ordered third-party plugins with diagnostics export, live reload, icons/categories, capability declarations, native settings pages, timeout controls, execution timing, persistent health history and automatic quarantine after three consecutive failures. Includes Calculator, Clock, Memory, eight native utilities, a guarded ARexx Runner and Music Controls for HippoPlayer, AmigaAMP and EaglePlayer 2.
- Per-application actions with safe `{path}` and `{drawer}` command placeholders.
- Editable text configuration, temporary **Use**, permanent **Save**, and live reload.
- ARexx commands and the `Beam68Ctl` Shell utility.
- Explicit `shell:<command>` execution with a compact `>` form, AUTO/CLOSE/WAIT console lifecycle and a Preferences safety switch.
- Persistent, bounded query history with F7/Shift+F7 navigation.
- Context-aware no-results help, plus validated ENV/ENVARC backup recovery when active settings are missing or damaged.
- Startup-crash detection that starts the next run once without plugins or custom themes. An ordinary power-off or reboot does not trigger it.

## Build and install

Build with a configured vbcc/NDK toolchain (set `BEAM68_VBCC`), or select an AmigaDev GCC/libnix compiler with `BEAM68_CC`:

```sh
./scripts/build-amiga.sh
```

For a GCC build:

```sh
BEAM68_CC=/path/to/m68k-amigaos-gcc ./scripts/build-amiga.sh all
```

The build targets 68000 and records source/binary hashes in
`BUILD-PROVENANCE.json`. The release packager checks that record, refusing stale
or missing executables. Direct `make CC=m68k-amigaos-gcc` builds remain available;
use the build script for a complete verified release build.

Copy the built project to the Amiga and, from its project directory, run:

```text
Execute scripts/Install-Beam68
```

This installs the launcher in `SYS:Utilities/Beam68`, Preferences in `SYS:Prefs`, and a startup project in `SYS:WBStartup`. **Quit an existing Beam68 before reinstalling.** Open `SYS:Prefs/Beam68Prefs` to configure it.

## Installing, updating and uninstalling

Double-click **Install** in the Beam68 drawer. It uses the standard AmigaOS
**Installer** utility (included with AmigaOS 3.1 and later; for 3.0 get
Installer 43.3 from Aminet) and offers:

- **Install or update**: choose where the `Beam68` drawer goes (default
  `SYS:Utilities`), which bundled plugins to install, and whether Beam68
  starts with Workbench. A running launcher is stopped first. An update
  keeps all settings in `ENVARC:Beam68`, keeps your icons, adds newly chosen
  plugins to your configuration, converts a 0.1 flat plugin layout and leaves
  third-party plugins alone. Beam68Prefs always goes to `SYS:Prefs`.
- **Uninstall**: stops the launcher and removes the Beam68 drawer (with all
  plugins), `SYS:Prefs/Beam68Prefs` and the startup entry, and asks whether
  to keep your settings.

The installer remembers the chosen drawer in `ENVARC:Beam68/InstallDir`.
For scripted installs the Shell installer remains: `Execute scripts/Install-Beam68`.

## Manual, languages and WHDLoad

- **Manual:** `Beam68.guide` (AmigaGuide, installed next to Beam68) covers
  everything from first steps to the configuration file. In Beam68Prefs the
  **Help** key opens it at the current page. Plugin authors get
  `Beam68PluginSDK.guide` in the SDK.
- **Languages:** the launcher uses locale.library catalogs. German
  (`deutsch`) and Swedish (`svenska`) are included and installed in
  `Beam68/Catalogs/`; English is built in. Translators start from
  `catalogs/Beam68.cd` (standard CatComp/FlexCat format); run
  `scripts/make-catalog.py` to rebuild. Launcher, Preferences and plugin strings include German and Swedish
  catalogues; the graphical installer includes those languages too. English
  remains the fallback, including some new diagnostic messages.
- **WHDLoad:** add your games drawer as an indexed directory. Game icons
  whose Default Tool is WHDLoad appear as `GAME` results and start exactly as
  from Workbench; icon-less `.slave` files start through `WHDLOAD_COMMAND`
  (default `WHDLoad`). `game:` searches only games; `INDEX_GAMES=NO` or the
  Indexing page turns it off.

## More in this release

- **Result actions:** Tab cycles Launch → Run with arguments → Open drawer →
  Information (Workbench's Info window) → Copy path → custom action.
- **Keys:** Amiga+V pastes, Ctrl+1..9 starts favorites 1–9, the mouse wheel
  scrolls results; `QUICK_LAUNCH=control alt` adds global Ctrl+Alt+1..9.
- **Highlighting:** matched letters are underlined in the results.
- **WHDLoad:** real game titles from the slave are shown and searchable,
  missing Kickstart images are reported before starting, `game:random`.
- **Tools:** `Beam68Ctl STATUS` / `QUITWAIT`; `scripts/m68k-syntax-check.sh`
  checks every source against the real NDK with Debian's m68k GCC;
  `.gitlab-ci.yml` runs tests, the syntax check, a vbcc build and packaging;
  `scripts/make-aminet-release.py` builds `Beam68.lha` + `Beam68.readme`.

## Latest additions

- **Browse drawers:** Right on a drawer result lists its contents in the
  launcher; type to filter, Left for the parent, Esc to return.
- **Plugin Protocol 1:** all 13 bundled native plugins use the first public
  protocol: up to eight selectable rows, delivered as a single Exec message.
  Text, error, path and query results share the same SDK. ARexx scripts use
  the documented text transport within Protocol 1.
- **Translations everywhere it matters:** Beam68Prefs and the GUI installer
  are now available in German and Swedish too.

## Also new

- **Calculator:** `=$FF+%1010`, `=0x10*4`; results show decimal, hex and binary.
- **`doc:`** recent documents, **`volume:`** live volumes *and assigns*,
  **`hidden:`** plus **Shift+Del** to hide any result.
- **Directory Opus:** "Show in Directory Opus" action, and
  `OPEN_DRAWERS_IN_DOPUS=YES` to open drawers in Opus listers.
- **`PLUGIN_CACHE=YES`:** plugins run from a RAM copy (fast on floppy/CF).

## Community features

- **ARexx plugins** (`COMMAND=PLUGINDIR:x.rexx`) and **live previews**
  (`PREVIEW=YES`) for plugin authors; see `sdk/Example/HelloRexx`.
- **Plugin and theme catalogue** in Beam68Prefs -> Extras: point it at a
  catalogue index (`make catalogue CATALOGUE_BASE_URL=...` builds one from
  the plugin packages and `themes/`), select an entry, Install.
- **Theme gallery:** four extra themes in `themes/`, installed to
  `Beam68/Themes`; a colour picker edits the custom theme.
- **Presets:** export/import a complete setup as one `.preset` file.
- **Translations:** see `TRANSLATING.md`.

## Index sources

- **`mod:`** music modules in your indexed drawers (`INDEX_MODULES=YES`),
  played with `MODULE_COMMAND` (HippoPlayer by default).
- **`web:`** bookmarks from HTML hotlists (`BOOKMARK_FILES`), opened with
  `URL_COMMAND` (the OpenURL tool by default).
- **`snip:`** text snippets from `ENVARC:Beam68/Snippets.txt`, copied to
  the clipboard.
- **`script:`** ARexx scripts in REXX:, run with RX.
- **`prefs:`** the preference programs as a group; **`stats:`** your most
  launched items with their counts.

## Keyboard shortcuts

| Keys | Action |
| --- | --- |
| Ctrl+Alt+Space | Open/close launcher (configurable) |
| Type / Enter | Find and launch |
| Up/Down | Select result, wrapping at the first and last item |
| Shift+Up/Down or PageUp/PageDown | Move a page |
| Ctrl+Up/Down or Home/End | Jump to first/last result |
| Tab | Accept the highlighted completion, or cycle application actions |
| Ctrl+L, Ctrl+U or Ctrl+Backspace | Clear search |
| F1 or Help | Open searchable commands, plugins, aliases, examples and shortcuts |
| F2 / F3 / Shift+F3 | Reindex / reload settings / reload plugins only |
| F5 / F6 | Preferences / pin or unpin favorite |
| F7 / Shift+F7 | Older / newer query history |
| F8 | Recall the last successfully run shell command |
| Esc | Cancel an active plugin wait; otherwise close launcher |

In Beam68Prefs, use **F7/F8**, **Left/Right**, or Ctrl+arrows to switch sections and **Up/Down** to navigate lists. Left/Right still edits the cursor while a text or number field is active.

Type `=12*(3+4)` for an immediate calculator preview. Plugin triggers include `@` (Clock) and `?` (Memory).

The utility plugins accept readable commands: `find:name`; `avail`, `version`, `flush`, `reboot confirm`, or `shutdown confirm`; `volumes`, `assigns`, `diskfree:RAM:`, or `open:RAM:`; `tasks` or `task:break:name:confirm`; `clip:show|set:text|clear|upper|lower|save:path`; `date:today|add:N|weekday:YYYY-MM-DD|timestamp`; `windows`, `window:name`, or `screen:name`; and `dev:hex:N|dec:$FF|ascii:N|mask:0,3|prot:RWED`. Destructive system and task actions require the literal confirmation word.
Type `shell:dir SYS:` or the shorter `>dir SYS:` and press Enter to run an AmigaDOS command in a separate output console. Leading and trailing whitespace is removed. The console opens for output, closes after successful completion and waits when acknowledgement is needed. F8 recalls the last successful shell command; F7/Shift+F7 browse all successful queries. This executes the text as a local Shell command, so disable **Built-in Shell Commands** on the Plugins page on shared systems.

Type `restart workbench` to close and reopen the Workbench desktop, or
`restart amiga` for a complete cold reboot. Beam68 requires Enter twice for
either command; changing the query or pressing Esc cancels confirmation. A
plain `restart` remains an ordinary application search.

Use Up/Down to browse the visible completion menu and Tab to accept a command, assign, configured drawer or plugin example. Use `rexx:list` to list configured ARexx macros and `rexx:<name>` to run one. The query can select only exact names from `ENVARC:Beam68/Plugins/ARexxRunner.cfg`; it cannot supply a script path. Run `Beam68 SAFE` for a one-time safe startup, or create `ENV:Beam68/SafeMode` before launching. Safe mode uses the Workbench palette and loads no external plugins; remove the ENV marker to resume normally. Beam68 also enters this mode automatically when `ENVARC:Beam68/Unclean.marker` shows that the previous run crashed during startup. The marker exists only for the first ~8 seconds of a run (and is removed before a deliberate reboot), so switching the Amiga off normally never forces safe mode; one safe run after a startup crash is followed by a normal start.

Use `music:status`, `music:play`, `music:pause`, `music:stop`, `music:next`,
`music:previous`, `music:volume up`, `music:volume down`, `music:volume 0-100`
or `music:players` to control a running HippoPlayer, AmigaAMP or EaglePlayer 2.
Auto detection is the default. Prefix the command payload with `hippo`,
`amigaamp` or `eagle` to target one explicitly, for example
`music:amigaamp next`. The plugin's native Preferences page selects a preferred
player while retaining automatic fallback. The plugin sends native ARexx
messages directly; `rexxsyslib.library` and the selected player's ARexx port
must be available, but no `Rx` command-path setup is required.

## Configuration and plugins

All launcher settings are plain text in `ENV:Beam68/Beam68.cfg` (current session) and `ENVARC:Beam68/Beam68.cfg` (persistent). The included `Beam68.cfg` is the first-run template. In Beam68Prefs, click **Capture...**, press the desired shortcut, then choose **Use** or **Save**. Manual editing remains available; press **F3** to reload manual changes.

Beam68 starts as a resident Commodity and normally opens no window at launch. Press the configured global hotkey (the supplied default is **Ctrl+Alt+Space**) to show or hide the launcher.

```ini
HOTKEY=rawkey control alt space
DIR=SYS:Tools
DIR=SYS:Utilities
MAX_ITEMS=256
INDEX_CACHE=YES
SHELL_COMMANDS=YES
THEME=WORKBENCH
CUSTOM_COLORS=CDD6F4,585B70,F38BA8,1E1E2E,11111B
PLUGIN_TIMEOUT=30
INDEX_FILES=NO
INDEX_DRAWERS=YES
FILE_PATTERNS=*.info,*.guide,*
EXCLUDE_DIR=SYS:Trashcan
FONT_NAME=topaz.font
FONT_SIZE=8
ROW_SPACING=7
COMPACT_MODE=AUTO
PLUGIN=PROGDIR:Plugins/Calc/Calc.plugin
```

Preferences uses `INSTALL_DIR=SYS:Utilities/Beam68` to locate plugins. When an older configuration lists only the original examples, shipped manifests are discovered automatically and added disabled, so Music Controls and the utility plugins become visible without silently activating capabilities. The selected plugin row stays highlighted while Details, Settings and other actions are used. Indexing enables file/drawer records, accepts comma-separated `*`/`?` filename filters and semicolon-separated excluded drawers. Favorites stores a group with each path, so `fav:games` and similar queries remain editable text. Appearance uses a compact theme selector for Workbench, Latte, Frappe, Macchiato, Mocha and Custom, with live preview plus portable `.theme` import/export and invalid-palette rollback. Cancel restores the active theme; Use or Save keeps it. During crash-recovery safe mode, custom-theme preview waits and Use or Save cleanly restarts Beam68 to apply the theme. Plugins can reload without restarting Beam68 and export `RAM:Beam68-Diagnostics.txt`. Details shows declared `FILES`, `SHELL`, `AREXX`, `CLIPBOARD` and `SYSTEM` access; a `SYSTEM` plugin requires a second Enter before execution. A quarantined plugin is marked `QUAR`; **Enable/Disable** clears its quarantine. Plugins declaring `SETTING=` records open a native settings page; older settings files still open in Ed. Preferences rejects duplicate/ambiguous hotkey tokens before writing settings. Applications assigns aliases/keywords and an optional action to each indexed path. Choose **Use** or **Save** to reload the launcher. See `docs/PLUGIN_SDK.md` and `docs/AREXX.md` for extension APIs.

In General, the **Launcher** field clearly reports Stopped, Starting,
Running, Restarting or Stopping. **Start** is available only while Beam68 is
stopped. **Restart** and **Stop** are available only while it is running;
Stop terminates the resident Commodity and releases its hotkey.

Use **F9** for Start, **Shift+F9** for Restart and **F10** for Stop while the
General page is active.

## Plugin packages and SDK

Every plugin is a self-contained **package**: one drawer under
`SYS:Utilities/Beam68/Plugins/<Name>/` holding its `.plugin` manifest, the
executable and any extra files. Inside a manifest, `PLUGINDIR:` means that
drawer, so packages work wherever Beam68 is installed. The 13 bundled plugins
use exactly this format.

To install a third-party plugin, open **Beam68Prefs → Plugins** and drop its
`.lha`, `.lzh` or `.zip` archive (or an unpacked plugin drawer) on the
window, or Browse to it and click **Add**. Preferences checks the archive and
manifest, shows the plugin's author, licence, version and declared
capabilities, and asks before installing. A newer version of an installed
plugin (same `ID`) upgrades it in place. Archives are unpacked with
`C:LhA` or `C:UnZip` (both on Aminet); without them, unpack the archive
yourself and install the drawer. **Remove** can also delete the files.

Plugin authors: the stand-alone SDK in `sdk/` (`make sdk-archive`) contains
the `beam68plugin.h` helper library, a HelloWorld example with vbcc and gcc
builds, and `tools/package-plugin.py`, which validates a plugin drawer and
creates the distributable archive. See `sdk/README.md`.

Configurations from the earlier flat layout (`PROGDIR:Plugins/X.plugin`)
are migrated automatically.

## Website

The responsive PHP website is in `public-web/index.php`, alongside the source in the same repository. Its official address is **https://beam68.retrocs.org/** and the public repository is **https://github.com/retrocs-org/beam68**. The website is supplied for manual upload; no deployment is performed by the build scripts. Run `python3 scripts/package-release.py` to create one `Beam68-0.1.zip` project archive (no nested downloads).

## Testing

Run `make host-test` for portable C and Python checks. In a source-only checkout,
use `BEAM68_SOURCE_ONLY=1 make host-test`. See `DEVELOPMENT.md` for build,
validation and packaging instructions, and `BUILD_STATUS.md` for current limits.

## Stage a runtime package

`./scripts/build-install-amiga.sh destination-root` rebuilds and copies the
application, icons, configuration templates, plugin binaries and installer to
`destination-root/Beam68`. Alternatively set `BEAM68_STAGE_DIR`.

```sh
./scripts/build-install-amiga.sh /path/to/staging
```

On AmigaOS, open the staged Beam68 drawer and run `Beam68`, or use the Install
icon to install it. For the Shell installer, change to the application drawer
before running `Execute scripts/Install-Beam68`.

## Release notes

See `CHANGELOG.md` for changes and `BUILD_STATUS.md` for validation status.
Screenshot descriptions and dimensions are recorded in
`public-web/images/screenshots.json`. Plugin capability labels are declarations,
not operating-system access controls. Keep preset recovery folders and plugin
rollback backups until recovery is complete.

## Preparing and publishing releases

The unified `scripts/release.py` entry point provides source-only pushes, plan, build, check, prepare
and explicitly confirmed publish commands for GitHub, GitLab and optional Aminet
submission. See [the release guide](docs/RELEASING.md) for local configuration,
exact artifact manifests, checksum verification and resumable receipts.
