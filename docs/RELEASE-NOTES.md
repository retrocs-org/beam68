## Features
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

## Enhancements
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

## Bugs
- Read saved Aminet settings before legacy ENV copies so an old download drawer cannot override Plugin Settings Save.
- Clear old plugin rows when recalling another query, and keep legacy plugin entries out of application-history recall.
- Save F6 favorite changes to temporary, active and permanent copies, and reclaim unpinned query entries without dropping unfinished scan results.
- Treat an empty Wikipedia search as no matches rather than a plugin failure.
- Fixed plugin updates enabling plugins the user had disabled.
- Prevented short-screen plugin controls from overlapping the bottom action row.
- Preserved depth and excluded-directory settings when switching Indexing views.
- Prevented background game indexing from caching an incomplete base scan.
