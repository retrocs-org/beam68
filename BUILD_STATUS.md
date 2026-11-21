# Beam68 validation status

## Current version: 0.5

Prepared 21 November 2026. The full host suite, sensitive-input/source checks,
translation freshness, independent plugin versions and strict m68000 native
build passed. Native Workbench 3.2.1 / 68060 checks covered result menus,
categories, inline completion, compact/detailed rows, dependency readiness,
read-only AmiHASS status tests, all eight plugin settings transactions, clipboard
recording lifecycle and Aminet completion fixtures. Preferences bounds and
launcher geometry were checked on small screens.

The actual 0.4-to-0.5 installer upgrade preserved settings. Both uninstall modes,
reinstallation and fresh defaults passed, including stopping the active test
clipboard recorder. Fresh defaults enable six of 19 plugins, keep network and
Clipboard Tools off, and retain Ctrl+Alt+Space. Reports are under
outputs/testing/0.5-release/. Physical 68000/minimum-memory and every translated
layout remain outside this validation. The historical results below apply to 0.4.

## Released 0.4 validation

Updated 18 November 2026. The application version is 0.4; release preparation and the final installer lifecycle checks passed.
The 0.3 validation results below are historical. On 18 November, native 0.4
640x200 gadget geometry and all seven declared plugin-settings transactions
passed. A SHA-256-verified published 0.3 package was installed and upgraded to
0.4; native assertions confirmed user files, an edited theme, an additional
plugin and autostart preservation. See docs/NATIVE_TEST_RESULTS.txt for scope. Plugins have
independent versions; the current inventory contains 19 bundled plugins.

The final 0.4 installer passed Novice fresh install, resident update, uninstall
keeping settings, reinstall, uninstall removing settings and fresh reinstall in
a disposable Workbench 3.2.1 / 68060 guest. Final defaults contained 19 plugins,
exactly six essentials enabled, network plugins disabled and Ctrl+Alt+Space.
The resident reported 0.4 and shut down cleanly. These results do not extend
the physical-machine or minimum-memory validation scope.

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
  recovery, interrupted cache saves and request admission. Those earlier network
  checks were deferred; later live AmiHASS checks are described below.
- Portable regressions cover bounded requests, stale replies, timeouts, low-memory
  recovery, interrupted cache-save recovery and confirmed Use/Save reloads.

On 18 November, direct native programs verified stable result identities,
separate plugin query histories, safe query favorites and cache compaction,
Aminet completion/retry/cancellation fixtures, and AmiHASS behavior and transport
recovery. Direct live AmiHASS checks passed authenticated connection, discovery,
authorized light control and confirmed restoration using the corrected service.
Fixture tests simulate external viewer/installer launches; they do not certify
all third-party archive handlers. See docs/NATIVE_TEST_RESULTS.txt for details.

## Installer checks and limits

Fresh installs must enable exactly Calculator, Clock, Memory, File Finder,
System Commands, and Assigns and Volumes. All network and other optional plugins
start disabled. Updates preserve explicit settings. Removing settings clears
T:Beam68 configuration and its backup, preventing an old session override.

The isolated Installer QA package redirects saved, live and temporary settings
into its own RAM tree. It checks the bundled registrations and all default activation
states. Its file-operation tests deliberately do not certify real launcher
startup/shutdown. The real 0.3 graphical installer was exercised through Amiga Workbench's
supported disposable test mode on Workbench 3.2.1 / 68060 on 16 November.
Fresh Novice installation, update, uninstall keeping settings, reinstall,
uninstall removing settings and final fresh installation passed across resumed
test sessions. The checks verified installed files, a responding 0.3 resident,
shutdown before uninstall completion, settings retention/removal, and all 18
plugin defaults. The final clean installation enabled exactly six essential
plugins and restored Ctrl+Alt+Space. The user confirmed opening, Escape hiding
and reopening with that shortcut. Remove-settings uninstall returned normally
and the subsequent installation completed without the reported crash.

This validates the exercised Novice maintenance paths; Intermediate/Expert
choices, upgrade from every older layout and small-display Preferences review
remain separate checks. The saved desktop was not used as the test target.

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
