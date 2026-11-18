# Releasing Beam68

Use Python 3.9+ from the project root. The entry point is `scripts/release.py`.
It builds once, prepares an exact manifest, then publishes those same bytes.
Do not publish until release notes, destinations and every artifact are reviewed.
No command creates repositories, pushes branches, deploys the website, moves a
tag, overwrites a conflicting asset or rolls back another service's release.

## Contents

- [GitHub and GitLab release guidelines](#github-and-gitlab-release-guidelines)
- [Aminet submission rules](#aminet-submission-rules)
- [Configure locally](#configure-locally)
- [Committed source only](#committed-source-only)
- [Build, check and prepare](#build-check-and-prepare)
- [Credentials and publication](#credentials-and-publication)
- [Receipts and interrupted runs](#receipts-and-interrupted-runs)
- [Testing and limits](#testing-and-limits)
- [Build output](#build-output)
- [GitHub App installation credentials](#github-app-installation-credentials)
- [Submit an already published release to Aminet](#submit-an-already-published-release-to-aminet)

## GitHub and GitLab release guidelines

These rules apply to Beam68 tagged releases on GitHub and GitLab. Use
scripts/release.py for preparation and publication. Local configuration and
credential setup are covered below. Aminet follows the submission rules in this guide.

### Version and tag

- VERSION, the application, Preferences, package metadata and release notes must
  identify the same release. Beam68 remains 0.4 until a version change is agreed.
- Use the exact version as the public tag, for example `0.4`, without a `v`
  prefix. The default tag is VERSION; pass `--tag 0.4` explicitly for clarity.
- Both services must receive the same annotated tag object and source commit.
  Never move, force-push or recreate a published release tag.
- Release titles use `Beam68 <version>`. The release description contains the complete
  changelog for that version, not an "In development" placeholder.

### Release notes

CHANGELOG.md is the complete history. docs/RELEASE-NOTES.md contains only
that version's changes, with Features, Enhancements and Bugs headings.
Do not prepend an application title, description, installation instructions,
licence links or a Version Changelog heading. The first public version may
retain its original feature categories. Older versions stay in CHANGELOG.md.
The publisher verifies that the description matches the complete current entry.

GitHub and GitLab release titles are always `Beam68 <version>`, for example
`Beam68 0.4`. Git tag names remain `0.4`; filenames and download URLs use that tag.
Aminet keeps its separate readme layout with metadata, description, installation,
source/licence and the version changelog.

### Assets and filenames

Upload exactly these four assets, substituting VERSION in each name:

```text
Beam68-0.4.lha
Beam68-0.4.lha.sha256
Beam68-0.4-plugin-sdk.zip
Beam68-0.4-plugin-sdk.zip.sha256
```

- Repository assets include the version. Only Aminet uses stable filenames.
- The LHA contains the complete installable application, plugins, themes,
  documentation and licence. The SDK ZIP contains the standalone plugin SDK.
- Each checksum contains its SHA-256 digest, two spaces, exact filename and newline.
- Upload identical package and checksum bytes to both services. GitLab exposes
  its verified generic-package files through release asset links.
- Use the hosts' automatic tagged source archives. Do not upload an additional
  source ZIP, website ZIP, aggregate checksum file, readme, logs or manifests.
- Keep credentials, local configuration, build records and publication receipts
  out of public release assets. Do not add hosted CI configuration as a release
  prerequisite; current checks run locally.

### Preparation and publication

1. Finish code, documentation, translations, website information and changelog.
   Review repository contents for sensitive files. Commit all release inputs;
   publication requires a clean checkout.
2. Confirm both configured repository destinations and access. Push the reviewed
   source normally if needed; source pushing is separate from release uploading.
3. Build in a fresh version-specific output directory. Run the checks and inspect
   the packaged documentation, installer, native verification and SDK inventory.
   Portable checks do not replace testing on the target system.
4. Prepare the exact tag and release notes. Review the manifest and all four
   hashes. Any release-input change after building requires a new build.
5. Publish explicitly to the intended services, using the same output directory,
   tag and notes throughout. Keep receipts and verify the public release links.

Example for 0.4, with a fresh output directory:

```sh
python3 scripts/release.py source --to gitlab,github --dry-run
python3 scripts/release.py source --to gitlab,github --confirm-push
python3 scripts/release.py build --output outputs/release-0.4-new
python3 scripts/release.py check --output outputs/release-0.4-new
python3 scripts/release.py prepare --output outputs/release-0.4-new \
  --tag 0.4 --notes docs/RELEASE-NOTES.md
python3 scripts/release.py publish --output outputs/release-0.4-new \
  --tag 0.4 --notes docs/RELEASE-NOTES.md \
  --to gitlab,github --confirm-publish
```

This example is for a new tag; it does not replace the published 0.2 release.
Do not run it to bypass conflicting published assets. Confirm native testing
separately before publishing. Website deployment remains a separate manual task.

### Interrupted publication and corrections

Preserve manifests, checksums, logs and receipts. Resume the same prepared release
only after inspecting the failure. Matching existing assets may be reused;
conflicting assets or notes are deliberately refused. Do not delete receipts,
edit fingerprints or disable checks to force a retry. A completed publication on
one service is not automatically rolled back if another fails.

Prefer a new patch release for corrections to published application packages.
A notes-only correction can update the release description after review, without
changing package bytes or moving the tag. If replacing published packages is
explicitly authorized, treat it as a separate reviewed recovery operation:
retain originals and their hashes, explain the correction, verify identical
replacement bytes on every destination and preserve a replacement receipt.
The normal release command does not overwrite conflicting assets. Check whether
Aminet needs a separate submission whenever its distributed bytes change.

## Aminet submission rules

Aminet submissions always use stable, case-sensitive filenames:

- `Beam68.lha`
- `Beam68.readme`

Do not include a version in either filename. The readme's Version field identifies
the release. GitHub and GitLab assets retain their versioned names. The Aminet
LHA is an unchanged copy of the verified runtime archive; renaming must not
rebuild or modify its contents. Never submit the SDK ZIP as the application LHA.

### Readme format

Generate the readme with scripts/make-aminet-release.py through the release tool.
Use plain ASCII text, readable indentation and lines no longer than 76 columns.
Do not include Markdown links, heading markers or decorative Unicode separators.

Header fields, in order:

```text
Short:        Keyboard application launcher for OS 3.x
Author:       Beam68 contributors
Uploader:     frossmant@icloud.com
Type:         util/wb
Version:      <VERSION>
Architecture: m68k-amigaos >= 3.0.0
Requires:     AmigaOS 3.0+, 68000+; TCP/IP for Aminet Browser
Distribution: Aminet
URL:          https://beam68.retrocs.org/
```

Follow the header with a short description and these sections:

- FEATURES: a concise summary of the application.
- INSTALLATION: unpack Beam68.lha, open Install, describe Novice and the hotkey.
- SOURCE AND LICENCE: repository, tagged release and MIT licence information.
- VERSION CHANGELOG: the complete matching VERSION section from CHANGELOG.md,
  grouped under Features, Enhancements and Bugs with indented entries.

Do not truncate the current release's changes or include older release sections.
The uploader contact is public. Review it before submitting.

### Preparing and uploading

Use the normal release commands documented in this guide. Aminet preparation
is separate from the four GitHub/GitLab assets and records its own hashes.
The existing-release path uses the published archive and tagged documentation.

Use a fresh output directory after changing naming or readme rules. Do not edit
old manifests, receipts or submitted files to make them appear current. Existing
incoming filenames require inspection before another submission; never overwrite
an unknown pending upload. FTP acknowledgement means awaiting moderation.

Earlier Beam68 releases used versioned Aminet filenames. The first submission
under the stable name does not automatically remove those older entries. Review
any migration/removal with Aminet separately rather than claiming it happened.

## Release visibility

GitHub and GitLab publications are normal, latest releases. GitHub explicitly
sets prerelease=false and make_latest=true after verifying all uploaded assets;
its temporary draft is published only after verification. GitLab sets released_at
to the current UTC time so it is not an upcoming release. The tool checks each
service's latest-release endpoint before reporting publication as complete.
A matching existing GitHub prerelease is promoted; a matching GitLab upcoming
release is dated to the present. Assets, notes and tags must still pass the
normal conflict checks. Re-running publication can make the selected release
latest again, so do not use this workflow to publish historical releases.

## Configure locally

The default settings file is external to the checkout:

```text
~/Development/tools/git-release/projects/beam68.json
```

Set `BEAM68_RELEASE_CONFIG` or pass `--config` to select another file. Use
`examples/release-config.json` as the non-secret template. Keep real machine
paths, internal repository destinations and credential lookup selectors outside
the public source tree. The external JSON supports repository HTTPS/SSH URLs,
`author`, `uploader`, `aminet`, `source_branch`, `env` and `credentials`.

Allowed `env` entries are `BEAM68_CC`, `BEAM68_VBCC`, `BEAM68_PHP`, `HOST_CC`,
`CFLAGS`, `BEAM68_RELEASE_PYTHON` and `LHA`. The preferred Python executable also
sets the Python tools used by make. A native 68000 compiler and NDK must already
be provisioned. Set LHA to a LHa-for-UNIX compression-capable executable for a
smaller LH5 package; without it, the stored archive fallback needs no packer.
There is no Mac signing/notarization target in this Amiga-only build.

To enable Aminet preparation, set `aminet` to true and `uploader` to your real
email contact. That contact appears in the public Aminet readme and is used for
anonymous FTP login. Set `author` as appropriate. Leaving Aminet disabled is
valid for GitHub/GitLab-only releases.

## Committed source only

```sh
python3 scripts/release.py source --to gitlab,github --dry-run
python3 scripts/release.py source --to gitlab,github --confirm-push
```

This preflights every selected branch push, then pushes only committed source
without force, mirror or follow-tags. It neither builds nor creates a release.
`source_branch` selects the destination branch; empty uses the current branch.
For an extracted directory, `source --init --dry-run` may initialize and commit
locally only after every selected remote is verified empty and Git identity and
.gitignore are checked. It never replaces existing history or adds an initial
commit to an established repository. Dry-run with --init can change local Git
metadata but never uploads. Inspect the commit before --confirm-push.

## Build, check and prepare

Finish source edits and commit them before building a production release:

```sh
python3 scripts/release.py plan
python3 scripts/release.py build
python3 scripts/release.py check
python3 scripts/release.py prepare --notes docs/RELEASE-NOTES.md
```

To keep a new version separate from earlier artifacts and publication receipts,
choose an output directory under `outputs/`:

```sh
python3 scripts/release.py build --output outputs/release-0.4 --verbose
python3 scripts/release.py check --output outputs/release-0.4
python3 scripts/release.py prepare --output outputs/release-0.4 --tag 0.4
```

Pass the same `--output outputs/release-0.4` when publishing those artifacts.
The four release assets go directly into that directory. Logs, manifests and
receipts go into its `release/` subdirectory, including
`outputs/release-0.4/release/build.log`. Without `--output`, the existing layout
is unchanged. `outputs/release/` itself remains reserved for the original
release state and cannot be selected as an artifact destination.

`plan` only prints steps. `build` runs the catalogue check and portable tests,
then builds all 17 native executables and verifies native provenance. It rejects common token/private-key formats and private credential files before
building, source pushes and package checks. Matched values are never printed. It checks
all release input fingerprints before and after compilation and packaging.
Inputs include code, tests, scripts, docs and website files. Source links and
checksum links must be updated before building; even documentation changes make
the prepared release stale. Native runtime certification remains a separate task.

The only files packaged at the top level of outputs/ and uploaded to GitHub
and GitLab tagged releases are:

- Beam68-0.4.lha
- Beam68-0.4.lha.sha256
- Beam68-0.4-plugin-sdk.zip
- Beam68-0.4-plugin-sdk.zip.sha256

Names derive from VERSION. The LHA contains the runtime, bundled plugins,
themes and notices. The SDK ZIP contains the standalone SDK, C/ARexx examples,
manuals, packaging tool and MIT license under one Beam68-PluginSDK drawer.
Generated example executables and private settings are excluded. Checksums use
SHA-256, two spaces, the exact filename and a newline.

Build records, logs, the reviewed manifest and staged upload copies remain under
outputs/release/. They are not release assets. Application source downloads are
generated by GitHub/GitLab from the tag. No separate application-source ZIP,
website ZIP or aggregate SHA256SUMS is created by this workflow. The PHP website
is deployed from public-web/ in the repository.

The LHA uses classic level-0 headers and LH5 compression when LHA is configured,
with a stored-entry fallback otherwise. Archive contents, extracted bytes and
per-file checksums are checked against the current source and native records.
Preparation from an extracted directory is allowed, but publishing requires a
clean Git checkout, tracked inputs and the exact prepared commit. Finish and
commit edits before building; changing an input makes the build stale.

Old source/website ZIPs, aggregate sums and obsolete prepared assets from the
previous inventory are retained under outputs/release/superseded/ when rebuilding.
Unknown files cause refusal. Existing publication receipts are never discarded.
If Aminet is enabled, its LHA/readme preparation stays separately under
outputs/release/aminet/ and is submitted only to Aminet, never attached to a
GitHub/GitLab release.

## Credentials and publication

Credentials may come from the environment or 1Password. They are resolved only
for selected publication destinations; source pushes and builds never fetch API
tokens. Do not put tokens, passwords or private keys in JSON, notes or commands.

```json
"credentials": {
  "github": {"provider": "env", "variable": "GITHUB_TOKEN"},
  "gitlab": {
    "provider": "1password",
    "vault": "YourVault",
    "item": "gitlab.example.com",
    "field": "your-group/beam68-rw",
    "account": ""
  }
}
```

Alternatively use `provider: "1password"` with a copied
`reference: "op://YourVault/YourItem/YourField"`, plus optional account. Do not
combine a reference with vault/item/field. The CLI must be signed in or connected
to its desktop app. Missing, cancelled or ambiguous lookups stop publication
without displaying secret output. Tokens stay in memory; selectors are not copied
into manifests or source archives. This is not comprehensive secret scanning of
source/history. Use project-specific repository token fields, not another
project's scoped credentials.

Git source/tag pushes use your normal SSH key/credential helper separately.
GitHub needs Contents write permission; GitLab requires the api scope and Developer access (30) or higher for releases
and generic packages. Protected-tag rules may require additional authorization.

Review the manifest and notes, then choose destinations explicitly:

```sh
python3 scripts/release.py publish --to github --confirm-publish
python3 scripts/release.py publish --to gitlab,github --confirm-publish
python3 scripts/release.py publish --to gitlab,github,aminet --confirm-publish
```

You may combine preparation and publication with
`release --to gitlab,github --confirm-publish`; it still requires a current verified
build. The default selection is gitlab,github. Distinct names are required. Omitting
`--confirm-publish` refuses publication. Selected credentials, repositories,
remote tags, existing assets and Aminet receipts are preflighted before mutations.
All destinations must use the same annotated tag object pointing to the prepared
commit. Tags are never forced. The version and default unprefixed tag derive from VERSION.
Beam68 uses explicit unprefixed tags: pass `prepare --tag 0.4` and
`publish --tag 0.4` for the current release.
`--tag-message` sets its annotation and must match between preparation and publication.
Package filenames and embedded app versions do not change with a tag override.
Release titles use `Beam68 <version>`, while Git tags keep the exact version. Update version-specific website links before
building; the download page discovers published stable releases automatically.

The publication stage is hash-checked before every destination; reviewed copies
are used for every upload. The release inventory is exactly the two requested packages and their two
checksums.

GitHub creates a draft, uploads assets and requires the returned SHA-256 digest
and size to match. Existing assets are reused only if their known hashes match.
Unknown digests, different notes and unexpected assets stop the operation. A
complete verified draft is made public last; an incomplete public release is not
modified. The hosts provide their automatic tagged source archives separately from these
four uploaded assets.

GitLab uses versioned generic packages, downloads and hashes their bytes before
creating a release linking them. Existing package bytes, notes and release links
must match. HTTPS redirects for package downloads never forward API credentials
to other origins. API writes reject redirects entirely.

Follow the [Aminet submission rules](#aminet-submission-rules) for stable `Beam68.lha` and
`Beam68.readme` names and the plain-text readme layout. Repository release assets
keep their versioned names.

Aminet submits the LHA/readme via anonymous FTP to main.aminet.net, `/new`.
This is unencrypted FTP, separate from the HTTPS repository APIs. The incoming
listing must be readable and filenames must not already exist. Acknowledged
submission awaits moderator review; it does not mean public Aminet publication.

## Receipts and interrupted runs

`outputs/release/receipt.json` binds completed destinations and the currently
running destination to the manifest hash. Resume with the same manifest and
selected destinations after inspecting any failure. Matching repository assets
are reused; different/unknown assets are never replaced. Completed destinations
are retained. There is no automatic network retry or cross-service transaction.

`outputs/release/aminet-receipt.json` is separate. It is written before the first
FTP transfer and tracks acknowledgements. If not marked
`submitted_awaiting_moderation`, submission is considered potentially partial;
the tool refuses automatic retry. Inspect the incoming server and preserve the
receipt, then resolve recovery with Aminet administrators. Deleting receipts is
not a recovery procedure. A changed manifest cannot reuse old receipts, and
`build` refuses to overwrite a release directory that has publication receipts.
Use a separate checkout/output directory for subsequent releases.

## Testing and limits

`python3 tests/test_release_tool.py` uses temporary Git repositories, local
artifacts and mocked APIs/FTP. It never publishes. Hosted adapters must also be
exercised against dedicated test repositories before the first production tag.
No real service publication is performed by unit tests or build/check/prepare.
Failed build commands retain redacted diagnostics under
`outputs/release/failed-build.log`. Git/API responses are not logged.
Build fingerprints establish matching bytes, not reproducible toolchains or
universal native compatibility. Multiple services cannot publish atomically;
remote changes between preflight and upload can still stop a later destination.

Reference API contracts:
[GitHub releases](https://docs.github.com/en/rest/releases/releases),
[GitHub assets](https://docs.github.com/en/rest/releases/assets),
[GitLab releases](https://docs.gitlab.com/api/releases/),
[GitLab generic packages](https://docs.gitlab.com/user/packages/generic_packages/).

## Build output

`build` prints each stage, its elapsed time and the final package sizes. Detailed
compiler and test output is saved to ignored `outputs/release/build.log` on both
success and failure. Run `python3 scripts/release.py build --verbose` to stream
that output while it runs. Credentials are removed from child environments and
known secret values are redacted. Git/API credential output is not logged.
The build log is overwritten for each attempt and is excluded from packages.

## GitHub App installation credentials

A helper-created GitHub App installation token is supported through the same
1Password field. It is not a personal access token and does not appear in the
personal-token settings lists. Preflight verifies the exact repository ID and
URL through the installation repository endpoint instead of treating the user
role permissions.push flag as an App permission. Release creation and uploads
still require Contents:write, enforced by GitHub. Installation access tokens
expire after one hour; refresh through your token helper when needed. The helper
and release configuration must use the same 1Password vault/item/field.

## Submit an already published release to Aminet

When a GitHub or GitLab publication receipt exists, Aminet-only publication
uses the retained, checksum-verified publication stage and the documentation
from the tagged commit. Working-tree edits do not require rebuilding this
already published archive. Enable Aminet and set the uploader in the external
configuration, then prepare and review the two files:

```sh
python3 scripts/release.py prepare-aminet --tag 0.1
python3 scripts/release.py publish --tag 0.1 --to aminet --confirm-publish
```

Preparation writes `outputs/release/aminet-existing/` and a separate manifest.
It does not change the repository publication manifest, packages, tags or receipts.
The upload retains the existing partial-submission protection and awaits moderation.
Do not delete receipts or edit fingerprints to work around a stale build.

## Bundled plugin versions

Run `python3 scripts/plugin-versions.py --check` before release builds. Plugin
versions are independent from Beam68 VERSION and Protocol 1. Changed plugin
inputs require a higher PLUGIN_VERSION and a reviewed --record update; see
[Plugin SDK](PLUGIN_SDK.md#independent-bundled-plugin-versions).

## Stage a test installer in a running shared builds drawer

After a verified build, use scripts/stage-builds.py with the same output directory,
the shared builds root as --destination, and the compression-capable LHA tool as
--lha when needed. The tool checks the archive and checksum, preserves the Beam68
drawer and nested drawer identities, and atomically replaces individual files.
It preserves unrelated files and guest metadata and backs up replaced test files
under the selected output's release/staging-backups directory. It never installs
into the guest system, starts a session or changes networking.

Do not remove/recreate a shared drawer while Workbench has it open: guest locks
can continue referencing the old host directory, causing missing-icon/script
errors. After an earlier whole-directory replacement, close and reopen that
Workbench drawer once before starting Install. Future staged updates preserve it.
