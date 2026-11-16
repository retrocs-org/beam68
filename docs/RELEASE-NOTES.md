# Beam68 0.2

Beam68 is a keyboard-driven application launcher for classic AmigaOS,
with a global hotkey and a separate native Preferences application.

## Features

- Find applications, files, drawers and WHDLoad games from the keyboard.
- Use favorites, aliases, recent items and searchable command help.
- Use bundled plugins, including an opt-in Aminet Browser.
- Adjust indexed drawers, shortcuts, fonts, colors and portable themes.

## Installation

Download `Beam68-0.2.lha`, unpack it, open the Beam68 drawer and double-click
Install. Choose Novice for automatic setup, or Intermediate/Expert to customize
it. Updates preserve saved settings. Press Ctrl + Alt + Space to open Beam68.
Read Beam68.guide for the full manual.

Requires AmigaOS 3.0 or later and a 68000 or later CPU. Aminet Browser needs a
TCP/IP stack and is disabled by default.

## Source and Licence

- [Source repository](https://github.com/retrocs-org/beam68)
- [Tagged source and release](https://github.com/retrocs-org/beam68/releases/tag/0.2)
- [Website](https://beam68.retrocs.org/)

Beam68 uses the MIT licence. See LICENSE in the archive for the full text.

## Version Changelog

### 0.2

#### Features

- Added Wikipedia 0.1: Wikipedia searches, selectable articles, native summary windows, language settings and full-article opening through OpenURL.

- Added unlimited directory scan depth, with heap-backed traversal and link-loop avoidance.
- WHDLoad now discovers games in its own background worker using the shared directories, exclusions and depth. Enabling the plugin activates game indexing and launch actions.

- Added WHDLoad 0.1 and DOpus 0.1 plugins with their own settings and normal search-result action integration.

- Added Music Library 0.1: separate module searches with configurable music drawer and player command; removed music indexing controls from Sources.

- Independently editable palettes for every theme, with a Restore original palette
  control. The reddish Custom theme is now named Rose.

- Aminet Browser plugin with search, package details, paged readmes, confirmed
  downloads and archive-opening installation preparation.
- Preferences About window showing the Preferences build and the actual version
  of the running Beam68 launcher.
- Per-plugin preview controls, including enable/disable and direct numeric delay
  entry with validation and Cancel support.
- Outstanding-request diagnostics showing the plugin, request age and timeout state.
- Resettable responsiveness measurements and a report script for repeatable
  startup, typing and result-display checks.
- Installer QA package for separate installation, update and uninstall tests,
  with file checks, settings-preservation checks and a readable report.
- Release-script `--output` option for separate artifact directories, logs,
  manifests and publication receipts for each release.

#### Enhancements

- Removed redundant SYSTEM confirmation for bundled Wikipedia search/summary and Music Library searches; music playback and other SYSTEM actions retain confirmation. Updated Wikipedia to 0.2 and Music Library to 0.7.
- Refreshed release-readiness documentation for the 18-plugin 0.2 inventory and current packaging requirements.

- Renamed Applications to Application shortcuts, added an explanation, and updated translations and help.
- Refreshed the PHP website with a complete modern midnight-blue redesign with horizontal navigation, centred app showcase, new overview structure, image-led gallery, download panels and documentation layouts, shorter pages, clearer downloads, current defaults and documentation. Simplified the marketplace plugin list to names and descriptions. Added practical plugin feature summaries and an expanded FAQ; removed older-version and About screenshots. Added fresh Wikipedia search and summary, File Finder, Date Utilities, Calculator and Memory captures with feature-based captions instead of version labels.

- Fresh installations enable six essential local tools; network and optional plugins start disabled. Existing saved choices are preserved during updates.

- Hide the launcher after Aminet hands off to its download window, without hiding a newer search or stopping Beam68.

- Added an Aminet download progress bar, filename, transferred and total sizes, and percentage, with redraw and cancellation support.

- Consolidated Preferences into six sections. General contains Startup, Behaviour and Sources; Appearance contains Themes, Text and readability, and Window layout. Fixed overlapping theme and readability controls.

- Plugin settings open in their own window with all fields, native checkboxes and choices, independent Save and Cancel, and validation before writing. Main Preferences edits remain open and unchanged.

- Simplified Extras; removed preset and catalogue shortcuts. Plugin Settings and Typing preview controls are disabled when unsupported. Increased plugin capacity to 32.

- Added direct plugin Import and limited Remove to non-bundled plugins, with
  bundled identities and canonical paths protected even when manifests are unreadable.

- Aligned plugin headings with row text and numbered priority in matching order.
- Kept selected plugin rows highlighted, simplified controls to Toggle and
  explicit priority buttons, and replaced paged Details with one larger view.
- Removed fixed post-install startup delays; readiness is checked immediately.

- Aligned plugin name, independent version, state and key columns in Preferences.
  Bundled plugins start at 0.1, with native version tags and a version-bump build guard.

- Installed all bundled plugins automatically at every installer user level;
  advanced setup focuses on destination and autostart, with plugin choices in Preferences.

- Earlier display of search results during indexing, with more frequent input checks.
- More responsive typing through coalesced input, immediate query feedback and
  less repeated matching work.
- Lower startup memory use through on-demand index string storage.
- Earlier duplicate-launch detection and deferred font-library loading.
- Deferred optional icon previews; native plugin requests avoid reply-file polling.
- Background plugin requests are bounded, manual requests retain capacity and
  queued previews are coalesced. Routine plugin-health saves wait for idle time.
- Startup timing includes library, configuration and metadata loading.
- Beam68Prefs can be found with ordinary preferences and settings searches.
- Novice installation uses automatic defaults and reuses the existing installation
  location. Novice uninstall offers a choice to keep or remove settings.
- The Shell installer stops the resident before copying, starts the installed
  launcher and checks its reported version before showing the search window.
- System Commands is enabled by default. Aminet Browser is installed but disabled
  until enabled in Preferences; updates retain saved plugin choices.

#### Bugs

- Isolated temporary settings in the Installer QA package so test cleanup cannot remove the live session configuration; expanded checks to cover every bundled plugin default.

- Fixed fresh graphical installations enabling every plugin by omitting default-disabled entries.
- Clear temporary Use configuration and its backup when removing settings or creating a fresh configuration, preventing old activation choices from overriding fresh defaults.

- Fixed live settings reload being rejected by an uninitialised hotkey expression version. Unchanged hotkeys are retained, and Preferences waits for confirmation before closing after Use/Save.

- Avoided recursive live ENV: settings removal during uninstall on linked
  environment setups; remove live files before their saved backing tree.
- Kept launcher shutdown visible until final cleanup and settings writes complete.
- Started the launcher in the background after installation and displayed the
  actual configured shortcut in the final confirmation, including preserved settings.
- Restored Mocha as the default palette for new installations.

- Fixed stack corruption when Preferences returns from its private application stack.
- Fixed Novice mode being unavailable in the installer and QA uninstaller.
- Fixed the Installer QA menu's Quit action.
- Preserved portable settings, aliases, snippets, additional plugins and edited
  bundled themes during updates. Missing settings inherit existing user settings
  and plugin-local configuration.
- Corrected legacy installation-location handling and known startup links when
  updating older layouts.
- Detached obsolete plugin requests after query changes, window closure or plugin
  reloads, preventing replies for older queries from replacing current results.
- Improved recovery after plugin timeouts, crashes, malformed replies and preview
  failures, with elapsed-time deadlines and stricter reply validation.
- Preserved existing index entries and titles after low-memory failures.
- Made cache reloads transactional, with rollback after failed replacement and
  recovery from validated backups after interrupted saves.
- Retained unverified entries after incomplete rescans and remapped selections
  after completed scans.
- Refreshed changed game and bookmark titles during rescans.
