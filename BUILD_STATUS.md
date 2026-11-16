# Beam68 validation status

## Current version: 0.2

Updated 16 November 2026. The application version remains 0.2. Plugins have
independent versions; the current inventory contains 18 bundled plugins.

## Automated release checks

The release build runs portable C/Python checks, translation freshness, the
sensitive-input scan, independent plugin-version checks, native 68000 compilation
with warnings treated as errors, SDK validation, archive inventory and checksum
verification. Publishing requires a clean committed checkout and release metadata
matching the exact source fingerprint. Rebuild after changing source or release
documentation; an older output directory is not ready merely because its binaries
still run. Use one output directory for build, check, prepare and publish.

The latest results are recorded in the selected output's release/build.log and
build.json. These local checks do not publish packages or change tagged releases.
No automatic GitHub Actions or GitLab CI configuration is included.

## Native observations

- The running launcher and native Preferences have been exercised during 0.2
  development, including actual running-version reporting and plugin loading.
- Live Wikipedia search and its native summary window were exercised on
  16 November. File Finder, Date Utilities, Calculator and Memory results were
  captured from the running application. Screenshots describe features and are
  cropped to application windows; they are not certification of every workflow.
- Earlier native Aminet checks covered search, readme parsing, complete archive
  transfer, response-size rejection and cancellation cleanup. The progress
  window and launcher handoff were also observed during development.
- Disposable Workbench 3.2.1 / 68060 recovery tests passed on 16 November: disk-full
  writes, failed renames, truncated presets, settings reload/removal, allocation
  recovery, interrupted cache saves and request admission. Live network checks
  were deferred because the current LAN connection is unavailable.
- Portable regressions cover bounded requests, stale replies, timeouts, low-memory
  recovery, interrupted cache-save recovery and confirmed Use/Save reloads.

## Installer checks and limits

Fresh installs must enable exactly Calculator, Clock, Memory, File Finder,
System Commands, and Assigns and Volumes. All network and other optional plugins
start disabled. Updates preserve explicit settings. Removing settings clears
T:Beam68 configuration and its backup, preventing an old session override.

The isolated Installer QA package redirects saved, live and temporary settings
into its own RAM tree. It checks all 18 registrations and all default activation
states. Its file-operation tests deliberately do not certify real launcher
startup/shutdown. The final-build GUI install/update/remove/reinstall cycle and small-display
Preferences review remain incomplete because the desktop window was inaccessible
on 16 November. Prepared QA tools are in Builds. Record their actual results
before claiming native validation.

Physical 68000/minimum-memory testing, every supported OS version and translated
layout, third-party players, Directory Opus interoperability, real full disks and
interrupted installations remain separate validation work. Capability labels do
not isolate native code. See docs/TEST_PLAN.md for the current regression checklist.

## Website and release assets

The PHP website has current feature descriptions, a simple plugin inventory,
feature-captioned application captures and automatic published-release downloads.
Its source is public-web/. Deploy it manually to https://beam68.retrocs.org/.
Website updates do not replace already published 0.2 application packages.

GitHub and GitLab receive the versioned LHA and plugin SDK ZIP, each with a
SHA-256 file. Aminet receives Beam68.lha and Beam68.readme without a version in
the filenames. All bundled native plugins use public Protocol 1. Publication
instructions and recovery procedures are in docs/RELEASING.md.
