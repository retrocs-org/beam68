# Releasing Beam68

Use Python 3.9+ from the project root. The entry point is `scripts/release.py`.
It builds once, prepares an exact manifest, then publishes those same bytes.
Do not publish until release notes, destinations and every artifact are reviewed.
No command creates repositories, pushes branches, deploys the website, moves a
tag, overwrites a conflicting asset or rolls back another service's release.

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

`plan` only prints steps. `build` runs the catalogue check and portable tests,
then builds all 17 native executables and verifies native provenance. It rejects common token/private-key formats and private credential files before
building, source pushes and package checks. Matched values are never printed. It checks
all release input fingerprints before and after compilation and packaging.
Inputs include code, tests, scripts, docs and website files. Source links and
checksum links must be updated before building; even documentation changes make
the prepared release stale. Native runtime certification remains a separate task.

Outputs are:

- `outputs/Beam68-<version>.lha`: complete runtime, plugins, themes and notices.
- `outputs/Beam68-<version>-source.zip`: matching source/SDK/website and build records.
- `outputs/Beam68-website.zip`: PHP website for manual upload.
- `outputs/Beam68-<version>.readme`: Aminet metadata, only when enabled.
- `outputs/<filename>.sha256`: one checksum for each package/readme.
- `outputs/SHA256SUMS`: exact inventory of packages and per-file checksums.
- `outputs/release/build.json`: full input fingerprint and native record binding.
- `outputs/release/manifest.json`: exact commit, reviewed notes, destinations and hashes.
- `outputs/release/publish/outputs/`: copies of the exact reviewed upload bytes.

The LHA uses classic level-0 headers and LH5 compression when LHA is configured,
with a stored-entry fallback otherwise. Compressed archives are extraction-checked
against the runtime payload. ZIP and LHA members, CRCs and runtime/source contents are
checked against current inputs. Source selection rejects links and private files,
excludes local work/output directories and never packages API tokens. Source ZIPs
carry `BUILD-PROVENANCE.json` and `RELEASE-BUILD.json`; no generated executables are
included in the source ZIP. Keep any required dependency notices in runtime files.

Preparation also works from an extracted source directory and records a null
commit. Publication refuses that directory: it requires a clean checkout at the
project root, tracked release inputs and the exact prepared commit. Committing
unchanged source after building requires preparing again. Changing any release
input requires rebuilding first. Do not edit build records to bypass checks.

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
GitHub needs Contents write permission; GitLab requires API and Maintainer access.

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
commit. Tags are never forced. The version derives from VERSION; the default tag is v followed by that version.
Use `prepare --tag 0.1` and `publish --tag 0.1` for an exact alternative tag.
`--tag-message` sets its annotation and must match between preparation and publication.
Package filenames and embedded app versions do not change with a tag override.
Release titles use the exact tag. Update any website tag links before building
if you choose a nondefault tag.

The publication stage is hash-checked before every destination; reviewed copies
are used for every upload. Beam68 keeps its explicit source ZIP and website ZIP
as release assets, alongside the runtime and checksums.

GitHub creates a draft, uploads assets and requires the returned SHA-256 digest
and size to match. Existing assets are reused only if their known hashes match.
Unknown digests, different notes and unexpected assets stop the operation. A
complete verified draft is made public last; an incomplete public release is not
modified. GitHub's automatic tag source archives are separate from our explicit
source asset, which carries the release build records.

GitLab uses versioned generic packages, downloads and hashes their bytes before
creating a release linking them. Existing package bytes, notes and release links
must match. HTTPS redirects for package downloads never forward API credentials
to other origins. API writes reject redirects entirely.

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
