# Beam68 0.2

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
