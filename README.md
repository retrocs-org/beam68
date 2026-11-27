# Beam68 0.6

This source tree contains Beam68 0.6. The website identifies the latest published download.

A keyboard-driven application launcher for classic AmigaOS, with a resident
Commodities hotkey and a separate native Preferences application.

[Website](https://beam68.retrocs.org/) ·
[Downloads](https://beam68.retrocs.org/download.php) ·
[Source](https://github.com/retrocs-org/beam68)

## Features

- Find applications, files, drawers and games by name, abbreviation, alias or
  keyword, with multi-term and typo-tolerant matching.
- Narrow searches with scopes such as `app:`, `prefs:`, `file:`, `drawer:`,
  `game:`, `plugin:`, `recent:` and `fav:games`.
- Browse drawers inside the launcher, pin favorites and recall recent queries.
- Index selected drawers in the background, retain a persistent cache and
  refresh the index after filesystem changes.
- Launch Workbench tools, projects and WHDLoad games; cycle result actions to
  open a drawer, supply arguments, show information or copy a path.
- Use 19 bundled plugins for calculations, files, clipboard, dates, memory,
  system tools, ARexx macros, music, WHDLoad, DOpus, Aminet, Wikipedia and AmiHASS.
- Choose fonts, spacing, compact layouts and themes, with independently editable, restorable
  five-color palettes and four portable theme files.
- Configure applications, indexing, favorites, plugins and appearance in
  Beam68Prefs, with temporary **Use**, permanent **Save** and live reload.
- Open **About...** in Preferences (F2) to see both Preferences and running
  launcher versions.
- Automate through ARexx and the `Beam68Ctl` Shell utility.
- Use English, German or Swedish interfaces, with AmigaGuide help.

## Requirements

AmigaOS 3.0 or later and a Motorola 68000 or later CPU. The binaries target
68000 and require no FPU; AmigaOS 3.2 and a 68020 are recommended. Memory use
depends on the index size and enabled plugins.

Opening drawers through Workbench requires workbench.library V44 or later.
Optional integrations require their corresponding applications, such as
WHDLoad, OpenURL, Directory Opus or a supported music player. Aminet Browser
requires an active TCP/IP stack; its installation action opens downloaded archives
with your archive handler so you can follow the package's own installer. See
[build status](BUILD_STATUS.md) for testing coverage and outstanding hardware checks.

Wikipedia uses a configured curl executable with verified HTTPS support for
searches and article summaries. OpenURL is needed only to open a full article.
See [Wikipedia](Plugins/Wikipedia/README.md) for commands and settings.

For Aminet commands, configuration and installation behavior, see
[Aminet Browser](docs/AMINET_BROWSER.md).

## Install

1. Download and unpack the Amiga LHA package.
2. Open **Install** in the Beam68 drawer. Every mode installs all bundled plugins.
   Intermediate and Expert let you choose the destination and Workbench startup.
   Novice reuses the existing location and startup setting on updates, and starts
   Beam68 in the background afterward. New installations use
   `SYS:Utilities/Beam68` and enable Workbench startup.
3. Open **SYS:Prefs/Beam68Prefs** to choose the drawers to index and your shortcut.
4. Start Beam68 and press **Ctrl+Alt+Space** to show the launcher.

The default installation is `SYS:Utilities/Beam68`. Preferences is installed
in `SYS:Prefs`; persistent settings are kept in `ENVARC:Beam68`. The installer
also provides update and uninstall options. Stop a running launcher before a
manual reinstall. For a Shell installation, run `Execute scripts/Install-Beam68`
from the unpacked project drawer. The Shell installer reuses the remembered
installation location, stops the resident launcher before copying, preserves
settings and starts and verifies the installed version. To choose another
location, use `Execute scripts/Install-Beam68 DEST "SYS:Utilities/Beam68"`.

The graphical installer uses the standard AmigaOS Installer utility. It is
included with AmigaOS 3.1 and later; AmigaOS 3.0 users need to supply it separately.

## Everyday use

Fresh installations enable Calculator, Clock, Memory, File Finder, System
Commands, and Assigns and Volumes. All network plugins and other optional tools
start disabled. Enable them in Preferences → Plugins; updates preserve saved choices.

Type a name and press Enter. Try `prefs:`, `fav:games`, `find:readme`,
`=12*(3+4)`, `@` for the clock or `?` for available memory. Press F1 to explore
commands, plugin examples, aliases and shortcuts.

| Shortcut | Action |
| --- | --- |
| Ctrl+Alt+Space | Show or hide the launcher; configurable in Preferences |
| Up/Down | Select a result |
| PageUp/PageDown | Move a page |
| Home/End | Jump to the first or last result |
| Right/Left | Enter a drawer or return to its parent |
| Tab | Accept a completion or open the selected result’s action menu |
| F1 / Help | Browse commands and help |
| F2 / F3 / Shift+F3 | Reindex / reload settings / reload plugins |
| F5 / F6 | Open Preferences / pin or unpin a favorite |
| F7 / Shift+F7 | Recall older/newer queries for the current plugin, or application history |
| Esc | Cancel a plugin wait or close the launcher |

Temporary **Use** settings are read first from `T:Beam68/Beam68.cfg`.
Other session settings are in `ENV:Beam68/Beam68.cfg`, and
persistent settings are in `ENVARC:Beam68/Beam68.cfg`. Use **Capture** in Preferences
to change the hotkey. **Use** applies settings for the session; **Save** retains them. Both wait for the running launcher to confirm the reload.
Removing settings during uninstall also clears temporary configuration and backups.

Shell commands are available through `shell:` or the compact `>` prefix and
can be disabled in Preferences. Built-in restart commands and SYSTEM plugins
require confirmation.
Native plugins run with the user's access; capability labels describe behavior
and do not isolate a plugin. Install code only from sources you trust.

## Preferences

The six sections are General (Startup, Behaviour and Sources), Indexing,
Plugins, Appearance (Themes, Text and readability, Window layout), Favorites,
and Application shortcuts. Favorites pins programs and groups; Application
shortcuts adds extra search words and custom launch actions.

Indexing supports a finite depth or Unlimited. WHDLoad uses these same roots,
exclusions and depth when enabled. Music Library has its own music drawer.
Plugin settings open separately with their own Save and Cancel controls.
See [Preferences](docs/PREFERENCES.md) for configuration and defaults.

Wikipedia searches and article summaries run on explicit Enter without an extra
SYSTEM confirmation. Music Library searches behave the same way; playing a
selected module still requires confirmation. AmiHASS reads states without an extra
prompt and confirms device actions in its own requester. Other SYSTEM plugins retain their
confirmation flow. These rules describe trusted bundled integrations, not a
security boundary for native plugins.

## Plugins and themes

Bundled plugins: Calculator, Clock, Memory, File Finder, System Commands,
Assigns and Volumes, Task Manager, Clipboard Tools, Date Utilities, Workbench
Windows, Developer Tools, ARexx Runner, Music Controls, Music Library, Aminet Browser, WHDLoad, DOpus, Wikipedia and AmiHASS.

All bundled native plugins use **Protocol 1**, with up to eight selectable
text, path or query results delivered in one message. ARexx scripts use the
text transport within the same protocol. The SDK includes C and ARexx examples,
build rules and a package validator.

Install plugin packages through **Beam68Prefs → Plugins**. Select, edit,
import or export themes through Preferences. The bundled portable themes are
Amber, GreenScreen, MagicGrey and Paper.

- [Plugin SDK](sdk/README.md) and [extension reference](docs/PLUGIN_SDK.md)
- [Theme examples](themes/)
- [ARexx commands](docs/AREXX.md)
- [Translation guide](TRANSLATING.md)

## Build and contribute

Use a configured AmigaDev GCC/libnix or vbcc/NDK toolchain:

```sh
BEAM68_CC=/path/to/m68k-amigaos-gcc ./scripts/build-amiga.sh all
```

[DEVELOPMENT.md](DEVELOPMENT.md) describes toolchains and tests.
[RELEASING.md](docs/RELEASING.md) covers source pushes, verified packages,
external settings and explicitly confirmed publication.
[PRE-PUBLISH.md](docs/PRE-PUBLISH.md) lists the remaining operator checks.

The complete application manual is [Beam68.guide](Beam68.guide). The Help key
in Preferences opens the manual at the current section.

## License

Beam68 is distributed under the [MIT License](LICENSE).

Preferences groups startup, behaviour and sources under General, and theme,
readability and layout settings under Appearance. Indexing offers Unlimited
depth. Enable WHDLoad to index games in the background from the shared directories.

Fresh installs enable Calculator, Clock, Memory, File Finder, System Commands
and Assigns and Volumes. Network and optional plugins start disabled. Saved
plugin choices are preserved on update. Enable Wikipedia for `wiki:Amiga`;
its Settings select language, HTTPS-capable curl and OpenURL executables.

Type `help` to discover enabled plugins, or enter a
plugin prefix such as `aminet:` to select an example before running it. Use F1
for full command help. `scan:status` reports indexing progress; `scan:cancel`
or Ctrl+F2 cancels indexing while retaining existing results. F2 or `scan:start`
starts another scan. Indexing Preferences separates directories from scan options.

Plugin Import reviews identity, version, access and package contents before
copying. New imports start disabled; updates preserve activation choices.
Plugin settings offer Restore defaults, with changes applied only by Save.

Use `health:` to inspect plugin outcomes and error/timeout counts. Enter prepares
a retry for an enabled plugin and clears quarantine; review and press Enter again
to execute. Disabled plugins remain disabled. `scan:failures` lists up to 16
failed locations from this session; Enter retries the associated configured root
while retaining existing results. Failure locations are session history.

AmiHASS integration (disabled by default) searches Home Assistant entities with
`ha:kitchen`, reads states and confirms basic device actions. Install, configure
and start AmiHASS first; connection settings and credentials stay in AmiHASS.
See [plugin instructions](Plugins/AmiHASS/README.md).

### Saved plugin searches

F7 and Shift+F7 recall read-only searches for the current bundled plugin,
separately from application history. Each plugin keeps up to 12 entries under
`ENVARC:Beam68/PluginQueries/<plugin-id>.txt`. Typing previews are not recorded.
Device actions, downloads, music playback and ARexx macro execution are excluded.
History recall only fills the query field; press Enter to run it.

Press F6 on a read-only plugin query or a selectable search/entity row to pin it.
Saved queries appear alongside application favorites and in `fav:` groups.
Selecting one, including through a favorite shortcut, opens Beam68 and fills
its query field. It never executes automatically. Enable the plugin before use.
Actions and unknown third-party commands cannot be pinned as query shortcuts.

Plugin requests display Searching/Working, a distinct no-match response, or an
error with a selectable Retry for supported read-only commands. Result selection
stays on the same item when indexing or plugin responses reorder the list.

Aminet download completion offers Open drawer, View readme and Install for
supported archives. Install opens your archive handler after checking the file
signature; follow the package's own instructions. View readme saves a companion
readme without overwriting an existing one, then opens MultiView. Failed
transfers remove partial data and offer an explicit restart; cancellation closes
the transfer without opening anything. Files already present are preserved.

## Search interface in 0.5

Click All, Apps, Files, Drawers or Plugins above the results to change search
scope while retaining the search text. Category buttons are hidden when the
window is too short; typed scopes remain available.

Tab, right-click or clicking the result footer opens an action menu for an
indexed result. Up/Down selects an action; Enter or clicking executes it.
Number keys choose a listed action directly. Escape dismisses the menu without
hiding Beam68. The footer describes the selected result’s Enter action.
Plugin suggestions, completions and drawer browsing retain their own controls.

Appearance → Window layout → Two-line rows displays a name above its location
or description. Disable it for the original compact columns. Compact mode and
short windows automatically use single-line rows. No additional index metadata
is collected for the detailed display.

## Completion, readiness and clipboard history

Matching local names appear as an inline suggestion at the end of the query.
Ctrl+Right accepts the suggestion explicitly; Enter keeps its normal result
behavior. Suggestions preserve a selected search scope and do not issue network
requests, change the clipboard or execute commands.

Plugins → Details shows local dependency readiness. Check readiness repeats the
local probe; AmiHASS provides an explicit, read-only Test connection using its
service STATUS reply. The check never starts services or controls devices.

Aminet downloads show average speed, estimated time remaining when size is
known, and downloaded bytes with an activity indicator when size is unknown.
Existing cancel, retry and completion actions remain explicit.

Clipboard Tools can record an optional session history with entry, text-budget
and expiry limits. Recording starts only with `clip:history on` and is off by
default. `clip:history words` searches; selecting an entry and pressing Enter
restores it. `clip:history clear` erases entries; `clip:history off` stops and
clears recording. History is temporary on RAM:, not saved preferences.
See [Clipboard Tools](Plugins/ClipboardTools/README.md) for limits and commands.

## Feedback, ranking and conversions in 0.6

Indexing shows the current drawer, scanned entries and indexed items in a banner.
Its bar measures completed configured roots, not a guessed file percentage.
Click the banner or press Ctrl+F2 to cancel; existing results remain available.
`scan:failures` explains recent unreadable locations and offers root retries.

Exact names and prefixes retain priority over looser matches. Favorites and
bounded launch frequency improve ordering within a match class; equal scores
sort by name and path. `history:reset` asks before clearing learned ranking and
recent applications; pinned favorites, aliases and plugin query histories remain.
Disabled or unavailable plugin prefixes explain the problem and direct you to
F5 -> Plugins. Missing dependencies and failed requests keep their error and
explicit retry action, while successful empty replies explain that no matches
were returned. No network plugin is enabled automatically.

Calculator also converts units locally: `=1.5 MiB to KiB`, `=90 min to h`,
`=1 in to cm`. Supported units are B/KB/MB/GB (decimal), KiB/MiB/GiB (binary),
ms/s/min/h/day, mm/cm/m/km/in/ft/yd/mi, mg/g/kg and C/F/K temperatures. Input accepts up to three
fractional decimal digits; results show up to twelve, with `~` for truncated
non-exact values. Incompatible dimensions and 32-bit intermediate overflows are
rejected. Arithmetic retains its existing integer semantics; no FPU is needed.

Plugin settings automatically use pages when a plugin has more fields than fit
on the current screen. Moving between pages preserves pending edits; Save writes
all pages, Cancel discards them, and Restore defaults stays pending until Save.

## Incremental updates and file actions in 0.6

Filesystem notifications coalesce changes in configured roots and up to 64
visited subdrawers. A watched subdrawer is rescanned independently; sibling
entries remain available and are not marked stale. Missing watched drawers fall
back to a root refresh; cancellation and I/O errors retain existing entries.
Handlers without notification support, deeper unwatched drawers and external
plugin indexes may still need F2 for a full rescan. No idle filesystem polling
is added. Partial/cancelled scans cannot certify an incomplete index cache.

Results keep their selected path or plugin row value across updates. If that
identity disappears, Enter and the Actions menu are blocked until you explicitly
select another result or change the query. Updating labels alone does not change
an actionable plugin row's identity. An adaptive bounded top-results collector preserves match
ordering while reducing work for large sets of similarly named applications.

The file Actions menu retains Copy path and Information and adds Open with.
Choose an executable in the native file requester; the file is passed as a
Workbench argument, including spaces, without changing its default tool.
Exclude drawer applies to the selected drawer or a file's parent. Confirm Use
for the current session, Save to keep it and the current running settings, or
Cancel. Matching entries disappear immediately; siblings remain indexed.

Plugin Settings provides Browse beside known executable and drawer fields:
Wikipedia's curl/OpenURL, WHDLoad, the Music Library drawer and Aminet's download
drawer. Browse only fills a pending value; Save and Cancel keep their usual
meaning. Command templates remain editable text. Dependency details include the
configured executable or service name; local checks do not contact a server.
