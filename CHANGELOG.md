# Changelog

## 0.2

### Features

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

### Enhancements

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
  location. Novice uninstall removes programs while retaining saved settings.
- The Shell installer stops the resident before copying, starts the installed
  launcher and checks its reported version before showing the search window.
- System Commands is enabled by default. Aminet Browser is installed but disabled
  until enabled in Preferences; updates retain saved plugin choices.

### Bugs

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

## 0.1 — First public release

### Launcher and search

- Resident Commodities launcher with a configurable global hotkey.
- Application, file, drawer, volume, assign and WHDLoad game indexing.
- Multi-term, abbreviation, alias, keyword and typo-tolerant search.
- Scoped queries, grouped favorites, recent applications and query history.
- In-launcher drawer browsing and a searchable command/help browser.
- Command, assign, drawer and plugin-example completion.
- Result actions for launching, supplying arguments, opening drawers,
  displaying information, copying paths and custom commands.
- Clipboard paste, favorite shortcuts, mouse-wheel scrolling and match highlighting.
- Optional Shell commands with a separate output console and command recall.
- Bookmarks, music modules, text snippets, recent documents and ARexx script sources.
- Directory Opus integration and Workbench tool/project launching.

### Preferences and appearance

- Separate native Beam68Prefs application with nine settings sections.
- Hotkey capture, keyboard navigation and launcher Start/Restart/Stop controls.
- Editable text settings with session-only Use, persistent Save and live reload.
- Configurable index roots, filename filters, excluded drawers and scan limits.
- Workbench and custom palettes, font selection, row spacing and compact layouts.
- Per-public-screen window placement and remembered geometry.
- High-contrast selection, larger controls and reduced-color rendering options.
- Portable theme import/export, a theme gallery and four bundled theme files.
- Preset import/export for complete settings profiles.
- Native normal/selected Workbench icons for the launcher and Preferences.

### Plugins and automation

- Thirteen bundled plugins: Calculator, Clock, Memory, File Finder, System
  Commands, Assigns and Volumes, Task Manager, Clipboard Tools, Date Utilities,
  Workbench Windows, Developer Tools, ARexx Runner and Music Controls.
- Calculator support for decimal, hexadecimal and binary expressions and results.
- Music controls for HippoPlayer, AmigaAMP and EaglePlayer 2.
- Public Protocol 1 with selectable text, path and query result rows.
- ARexx script plugins through the Protocol 1 text transport.
- Plugin package installation/removal, ordering, settings pages and live reload.
- Plugin help, examples, icons, categories and capability declarations.
- Live previews, timeouts, execution timing, diagnostics and health history.
- Plugin quarantine, safe startup and confirmation for system-changing actions.
- ARexx application control and the Beam68Ctl Shell utility.
- Plugin SDK with C/ARexx examples, build rules and package validation.
- Plugin and theme catalogue support in Preferences.

### Installation, documentation and distribution

- Graphical installation, update and uninstall, with selectable plugins and
  optional Workbench startup integration.
- Shell installation and build/staging tools.
- English interface with German and Swedish catalogues and installer translations.
- AmigaGuide application and SDK manuals, plus developer and translation guides.
- Responsive PHP website with screenshots, FAQ, downloads, developer documentation
  and a plugin/theme marketplace listing the bundled additions.
- Verified Amiga LHA and plugin SDK packaging with checksums and build records.
- Release automation for source pushes, GitHub, GitLab and optional Aminet submission.
