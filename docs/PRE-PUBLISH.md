# Before publishing Beam68 0.4

Work from the reviewed Git checkout. Commit the final source and documentation
before publication. The app stays at 0.4 unless its version is explicitly changed.
Version 0.4 is a new release. Build and verify new packages for its own tag;
keep published 0.3 assets and release history unchanged.

## Automated gates

- Run the release build with a fresh output directory, then check that same
  directory. Review its release/build.log and build.json.
- Sensitive-input scanning excludes local settings, editor state, build output
  and publication receipts, and refuses common credential formats and unsafe
  paths. A clean scan cannot prove that no possible secret exists in history.
- Plugin versions, translation catalogs, native build hashes, SDK packages,
  archive contents and checksums must match. Website checks cover rendered PHP,
  navigation, metadata, screenshots and offline release-service behaviour.
- Publishing requires a clean committed checkout and explicit confirmation.
  Never reuse stale build metadata after source or documentation changes.

## Final native checks

1. Using the final installer, test Novice install, update while the resident is
   running, uninstall with Remove settings, and reinstall. Verify six enabled
   local plugins, every network plugin off, no crash, no stale temporary settings,
   background startup and the actual configured shortcut.
2. Confirm Use and Save change the running configuration. Review Preferences on
   a smaller display, including Appearance and independent plugin settings.
3. Wikipedia search/summary and Music Library searches should not need a second
   SYSTEM confirmation. Music playback and other SYSTEM actions retain their
   confirmation. Network requests remain manual and optional plugins stay off
   by default.
4. Use the isolated Installer QA package for file-operation checks. Its saved,
   environment and temporary settings all stay under RAM:Beam68InstallerQA; it
   never stops the live launcher. This is additional coverage, not a substitute
   for actual startup/shutdown testing.

Record actual outcomes and any limitations in BUILD_STATUS.md. Do not claim
physical 68000/minimum-memory certification from host tests or screenshots.

## Publication review

Check the exact commit/tag, repository permissions, asset names, checksums and
release description. GitHub/GitLab releases are normal stable/latest releases.
Use Features, Enhancements and Bugs in the shared version changelog. Aminet uses
a real uploader contact; submission success still awaits moderation. Deploy the
website separately, after reviewing its manual-upload archive.

See [RELEASING.md](RELEASING.md) for commands and replacement/receipt recovery.
Build/check/prepare and automated tests do not push source or publish assets.

## AmiHASS dependency

Use the stable AmiHASS 0.1 release published 18 November 2026 or later. Its
tagged source includes the all-area catalogue template correction required for
entity discovery. Older local builds bearing 0.1 may lack this correction;
update from the published package when catalogue requests fail. The plugin is
optional and disabled by default. See NATIVE_TEST_RESULTS.txt for verification
scope and the distinction between the live test build and published binary.
