# Beam68 0.1 validation status

Updated 11 November 2026.

- All 17 runtime HUNK executables build for `-m68000` using AmigaDev GCC 16.2
  and libnix. `-Wall -Wextra -Werror` passes with classic string-pointer
  signedness compatibility warnings disabled.
- 38 portable C tests and Python checks pass, including memory instrumentation,
  bundled manifest loading, search ranking, archive validation, preset rollback,
  configuration, catalogue freshness, guides, icons and website links.
- Source-only checkout checks pass with `BEAM68_SOURCE_ONLY=1`. Release
  packaging always requires the real runtime files and matching build hashes.
- Runtime checks cover search, calculator output, command help, Preferences,
  compact plugin Details and all 13 bundled plugin manifests (zero load errors).
- Isolated native filesystem tests cover injected disk-full writes, failed commit
  renames, truncated preset input and independent settings save/reload/removal.
  These are controlled failure simulations, not power-loss certification.
- A synthetic positive-prefix matcher workload preserves its result checksums
  while avoiding unnecessary alias scoring. No universal throughput is claimed.
- Tests and native release builds run locally; repository uploads do not start
  GitHub Actions or GitLab CI jobs.

Physical 68000/minimum-memory testing, a full clean-OS GUI install/update/remove
cycle, all translated layouts, other supported OS versions, third-party players,
Directory Opus interoperability and every archive-extractor/filesystem combination
remain validation work. Actual full disks and interrupted installs need separate
checks. Capability declarations do not isolate native plugins.

The responsive PHP website in public-web/ uses shared page components, local
CSS and app-window images. PHP CLI rendering checks pass. It is ready for manual upload to
https://beam68.retrocs.org/. The repository address is
https://github.com/retrocs-org/beam68. Build scripts do not publish either one.

All bundled native plugins now use public Protocol 1 selectable-result messages.
The SDK validates routing and bounded serials, and reports delivery failures
without a file fallback. ARexx scripts retain the documented text transport.
Website captures retain the hashes of the build used when they were captured;
they are illustrative views and do not certify a newer executable.
