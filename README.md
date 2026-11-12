# Beam68 0.1

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
- Use 13 bundled plugins for calculations, files, clipboard, dates, memory,
  system tools, ARexx macros and music-player controls.
- Choose fonts, spacing, compact layouts and themes, including a custom
  five-color palette and four portable theme files.
- Configure applications, indexing, favorites, plugins and appearance in
  Beam68Prefs, with temporary **Use**, permanent **Save** and live reload.
- Automate through ARexx and the `Beam68Ctl` Shell utility.
- Use English, German or Swedish interfaces, with AmigaGuide help.

## Requirements

AmigaOS 3.0 or later and a Motorola 68000 or later CPU. The binaries target
68000 and require no FPU; AmigaOS 3.2 and a 68020 are recommended. Memory use
depends on the index size and enabled plugins.

Opening drawers through Workbench requires workbench.library V44 or later.
Optional integrations require their corresponding applications, such as
WHDLoad, OpenURL, Directory Opus or a supported music player. See
[build status](BUILD_STATUS.md) for testing coverage and outstanding hardware checks.

## Install

1. Download and unpack the Amiga LHA package.
2. Open **Install** in the Beam68 drawer. Choose a destination, plugins and
   whether to start Beam68 with Workbench.
3. Open **SYS:Prefs/Beam68Prefs** to choose the drawers to index and your shortcut.
4. Start Beam68 and press **Ctrl+Alt+Space** to show the launcher.

The default installation is `SYS:Utilities/Beam68`. Preferences is installed
in `SYS:Prefs`; persistent settings are kept in `ENVARC:Beam68`. The installer
also provides update and uninstall options. Stop a running launcher before a
manual reinstall. For a Shell installation, run `Execute scripts/Install-Beam68`
from the unpacked project drawer.

The graphical installer uses the standard AmigaOS Installer utility. It is
included with AmigaOS 3.1 and later; AmigaOS 3.0 users need to supply it separately.

## Everyday use

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
| Tab | Accept a completion or cycle result actions |
| F1 / Help | Browse commands and help |
| F2 / F3 / Shift+F3 | Reindex / reload settings / reload plugins |
| F5 / F6 | Open Preferences / pin or unpin a favorite |
| F7 / Shift+F7 | Browse query history |
| Esc | Cancel a plugin wait or close the launcher |

Settings are plain text in `ENV:Beam68/Beam68.cfg` for the current session and
`ENVARC:Beam68/Beam68.cfg` for persistent changes. Use **Capture** in Preferences
to change the hotkey. **Use** applies settings for the session; **Save** retains them.

Shell commands are available through `shell:` or the compact `>` prefix and
can be disabled in Preferences. Built-in restart commands and SYSTEM plugins
require confirmation.
Native plugins run with the user's access; capability labels describe behavior
and do not isolate a plugin. Install code only from sources you trust.

## Plugins and themes

Bundled plugins: Calculator, Clock, Memory, File Finder, System Commands,
Assigns and Volumes, Task Manager, Clipboard Tools, Date Utilities, Workbench
Windows, Developer Tools, ARexx Runner and Music Controls.

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
