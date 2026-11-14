# Changelog

## 0.2 — In development

- Safe return from the private application stack when closing Preferences.
- Find Beam68Prefs with ordinary preferences/settings searches.
- Automatic Novice installation and opt-in Aminet Browser defaults.

- Preferences About window reports its build and the actual running launcher version.

- Independent Aminet Browser plugin: inline search and details, paged readmes,
  confirmed downloads and archive-opening installation preparation.

- Direct numeric preview-delay entry with validation and Cancel support.

- Outstanding-request diagnostics identify holding plugins, request age and timeout state.

- Recover validated index backups after interrupted cache replacement.

- Bound outstanding plugin requests, reserve manual-run capacity and coalesce queued previews.

- Per-plugin preview enable/disable and delay controls in Preferences Details.

- Upgrades preserve portable settings, aliases and edited bundled themes.
- Missing ENV/ENVARC settings inherit existing user settings and plugin-local configuration.

- Obsolete plugin requests detach on query edits, close and reload.
- Elapsed-time plugin deadlines, stricter reply validation and preview failure recovery.
- Native requests avoid reply-file polling; routine health saves wait for idle time.

- Low-memory failures preserve existing entries and titles.
- Transactional cache reloads and rollback on failed cache replacement.
- Incomplete rescans retain unverified entries; completed scans remap selections.
- Changed game and bookmark titles refresh during rescans.

- Resettable responsiveness diagnostics and an Amiga report script for repeatable typing tests.

- On-demand index string storage reduces startup memory reservations.
- Earlier duplicate-launch detection and deferred font-library loading.
- Startup timing includes library, configuration and metadata loading.

- More responsive typing with coalesced input and immediate query feedback.
- Earlier display of first matches during indexing and more frequent input checks.
- Deferred optional icon previews and reduced matcher work.

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
