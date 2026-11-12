# Beam68 development

## Build

Use a configured vbcc/NDK toolchain or AmigaDev GCC/libnix:

```sh
BEAM68_CC=/path/to/m68k-amigaos-gcc ./scripts/build-amiga.sh all
BEAM68_VBCC=/path/to/vbcc-ndk ./scripts/build-amiga.sh all
```

The build targets 68000. GCC uses `-m68000 -mcrt=nix20`; vbcc expects an
`aos68k` configuration with local include and library paths. Compiler and
licensed operating-system files are not included.

`BUILD-PROVENANCE.json` records source, compiler options and executable hashes.
`python3 scripts/build-provenance.py verify` checks that the files agree.

## Checks

`make host-test` runs 38 portable C programs plus Python checks. PHP CLI is
required for website rendering checks; set BEAM68_PHP if it is not on PATH. In a source-only
checkout without built executables, use `BEAM68_SOURCE_ONLY=1 make host-test`.
This mode checks package metadata with explicit fixtures; it does not certify
native executables. Normal distribution checks still require runtime files.

For memory checks on macOS:

```sh
make HOST_CC='cc -fsanitize=address,undefined -fno-omit-frame-pointer -g -Wno-deprecated-declarations' host-test
```

The deprecation override covers legacy C89 declarations in the host SDK.
GitHub workflows run portable and sanitizer checks. A manually enabled native
runner can also build and package with a locally provisioned toolchain.

`BEAM68_CC=/path/to/m68k-amigaos-gcc scripts/build-native-qa.sh` builds test tools
under build/. Run them only in an isolated AmigaOS test installation:

- Beam68ReleaseQA checks preset recovery, independent configuration persistence
  and a synthetic matcher workload in newly reserved T:Beam68QAxx drawers.
- Beam68RexxProbe TIMING, INDEX or PLUGINS reads live launcher diagnostics.
- Beam68SlowHost provides a test-only DOPUS.1 port with one deliberately late
  reply. It executes no host commands and refuses to replace an existing port.

See docs/TEST_PLAN.md for manual checks. A passing build or portable test does
not establish behavior on every supported OS version or physical system.

## Stage and package

`./scripts/build-install-amiga.sh /path/to/staging` performs a fresh build and
stages runtime files under a Beam68 subdrawer. It requires an explicit path or
BEAM68_STAGE_DIR. Choose a separate destination for isolated tests.

`python3 scripts/package-release.py` verifies native hashes and creates
Beam68-0.1.zip beside the project, including source, runtime files and public-web.
It does not publish a release or deploy the website. Build output under build/
is excluded from release archives and ignored by Git.

## Unified release workflow

Use `python3 scripts/release.py plan`, then `build`, `check` and `prepare` to
produce reviewed source/LHA/website packages under outputs/. Publication is
separate and requires an explicit confirmation flag. See docs/RELEASING.md for
external project settings, 1Password/environment credentials, source-only pushes,
tag checks, staged assets, receipts and interrupted-upload recovery.
