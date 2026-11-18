# Changelog

## 0.4

### Features
- Add separate, bounded read-only query histories for bundled plugins, recalled with F7 and Shift+F7.
- Pin read-only plugin searches and entity queries alongside application favorites; opening a saved query never executes it automatically.
- Offer Open drawer, View readme and Install after Aminet downloads, with explicit retry after interrupted transfers.
- Add an optional AmiHASS plugin to search Home Assistant entities, read states and confirm basic device actions through the AmiHASS service.
- Added searchable `health:` plugin outcomes, failure/timeout counts and explicit retry preparation.
- Added `scan:failures` with up to 16 session failure locations and root retry.
- Added Restore defaults to plugin settings; defaults remain pending until Save.
- Added enabled-plugin discovery through `help`, `help:` and prefix-only queries,
  with selectable examples and Aminet search, details, download and install-preparation suggestions.
- Added `scan:start`, `scan:status`, `scan:cancel` and Ctrl+F2 scan cancellation,
  retaining existing results and exposing scan progress and skipped/unavailable counts.

### Enhancements
- Keep the selected item stable when result order or index positions change, including plugin rows whose state text updates.
- Show Searching/Working, distinguish no matches from errors, and offer explicit retries for supported read-only plugin requests.
- Plugin imports now review file counts, sizes and a bounded file list before
  installation. Bare-manifest references also require review and reject duplicate IDs.
- New imports start disabled. Updates retain activation choices, bundled plugins
  stay managed by the Beam68 installer, and copied manifests are revalidated.
- Package review rejects linked, unreadable or oversized directory trees.
- Split Indexing into Directories and Scan options so controls fit short screens.
- Compact plugin controls and settings fields for 640x200 displays.
- Added dependency guidance for bundled integrations and actionable TCP/IP and
  Wikipedia downloader checks before network requests.
- Saving plugin settings notifies the running launcher without applying unsaved
  edits in the parent Preferences window. Cancel continues to leave files unchanged.

### Bugs
- Read saved Aminet settings before legacy ENV copies so an old download drawer cannot override Plugin Settings Save.
- Clear old plugin rows when recalling another query, and keep legacy plugin entries out of application-history recall.
- Save F6 favorite changes to temporary, active and permanent copies, and reclaim unpinned query entries without dropping unfinished scan results.
- Treat an empty Wikipedia search as no matches rather than a plugin failure.
- Fixed plugin updates enabling plugins the user had disabled.
- Prevented short-screen plugin controls from overlapping the bottom action row.
- Preserved depth and excluded-directory settings when switching Indexing views.
- Prevented background game indexing from caching an incomplete base scan.

## 0.3

### Features
- Added Wikipedia: Wikipedia searches, selectable articles, native summary windows, language settings and full-article opening through OpenURL.
- Added unlimited directory scan depth, with heap-backed traversal and link-loop avoidance.
- WHDLoad now discovers games in its own background worker using the shared directories, exclusions and depth. Enabling the plugin activates game indexing and launch actions.
- Added WHDLoad and DOpus plugins with their own settings and normal search-result action integration.
- Added Music Library: separate module searches with configurable music drawer and player command; removed music indexing controls from Sources.
- Independently editable palettes for every theme, with a Restore original palette
  control. The reddish Custom theme is now named Rose.

### Enhancements
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

### Bugs
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
