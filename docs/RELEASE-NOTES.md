# Beam68 0.3

Beam68 is a keyboard-driven application launcher for classic AmigaOS,
with a global hotkey and a separate native Preferences application.

## Features

- Find applications, files, drawers and WHDLoad games from the keyboard.
- Use favorites, aliases, recent items and searchable command help.
- Use bundled plugins, including an opt-in Aminet Browser.
- Adjust indexed drawers, shortcuts, fonts, colors and portable themes.

## Installation

Download `Beam68-0.3.lha`, unpack it, open the Beam68 drawer and double-click
Install. Choose Novice for automatic setup, or Intermediate/Expert to customize
it. Updates preserve saved settings. Press Ctrl + Alt + Space to open Beam68.
Read Beam68.guide for the full manual.

Requires AmigaOS 3.0 or later and a 68000 or later CPU. Aminet Browser needs a
TCP/IP stack and is disabled by default.

## Source and Licence

- [Source repository](https://github.com/retrocs-org/beam68)
- [Tagged source and release](https://github.com/retrocs-org/beam68/releases/tag/0.3)
- [Website](https://beam68.retrocs.org/)

Beam68 uses the MIT licence. See LICENSE in the archive for the full text.

## Version Changelog

### 0.3

#### Features

- Added Wikipedia: Wikipedia searches, selectable articles, native summary windows, language settings and full-article opening through OpenURL.

- Added unlimited directory scan depth, with heap-backed traversal and link-loop avoidance.

- WHDLoad now discovers games in its own background worker using the shared directories, exclusions and depth. Enabling the plugin activates game indexing and launch actions.

- Added WHDLoad and DOpus plugins with their own settings and normal search-result action integration.

- Added Music Library: separate module searches with configurable music drawer and player command; removed music indexing controls from Sources.

- Independently editable palettes for every theme, with a Restore original palette
  control. The reddish Custom theme is now named Rose.

#### Enhancements

- Removed redundant SYSTEM confirmation for bundled Wikipedia search/summary and Music Library searches; music playback and other SYSTEM actions retain confirmation. Updated Wikipedia to 0.2 and Music Library to 0.7.

- Refreshed release-readiness documentation for the 18-plugin inventory and current packaging requirements.

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

- Novice installation uses automatic defaults and reuses the existing installation
  location. Novice uninstall offers a choice to keep or remove settings.

- Added GitHub download counts for the application and plugin SDK to the website, using the existing cached release API response.

#### Bugs

- Regenerated translated catalogue version metadata for the 0.3 application.

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
