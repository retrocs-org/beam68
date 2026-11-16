# Beam68 0.2

Beam68 0.2 is a released version. Downloads are available on the website.

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
- Use 18 bundled plugins for calculations, files, clipboard, dates, memory,
  system tools, ARexx macros, music, WHDLoad, DOpus, Aminet and Wikipedia.
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
| Tab | Accept a completion or cycle result actions |
| F1 / Help | Browse commands and help |
| F2 / F3 / Shift+F3 | Reindex / reload settings / reload plugins |
| F5 / F6 | Open Preferences / pin or unpin a favorite |
| F7 / Shift+F7 | Browse query history |
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
selected module still requires confirmation. Other SYSTEM plugins retain their
confirmation flow. These rules describe trusted bundled integrations, not a
security boundary for native plugins.

## Plugins and themes

Bundled plugins: Calculator, Clock, Memory, File Finder, System Commands,
Assigns and Volumes, Task Manager, Clipboard Tools, Date Utilities, Workbench
Windows, Developer Tools, ARexx Runner, Music Controls, Music Library, Aminet Browser, WHDLoad, DOpus and Wikipedia.

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
