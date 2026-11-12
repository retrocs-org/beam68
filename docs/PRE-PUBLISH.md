# Before the first public release

The reviewed source directory is an extracted project, not a Git checkout.
Its current files and distribution packages can be scanned, but there is no Git
history here to audit. Import into a reviewed checkout or use the explicit source
initialization workflow before publishing. Review any history imported from
elsewhere; checking current files does not remove older committed secrets.

## Automated gates

- Public source selection excludes local settings, editor state, build output,
  publication receipts and generated executables. Credential files are refused.
- scripts/check-sensitive.py checks common token/key formats, credential-bearing
  URLs and machine-specific paths without printing matched values. It also checks
  against credential values already present in the process environment. It does
  not retrieve credentials to perform a scan and cannot identify every secret.
- Source pushes, unified build/check and complete-project packaging run this gate.
- Website tests check repository link spelling and case, including on macOS.
- Native build records, archive contents, prepared copies and checksums must match.
- Publishing needs a clean committed checkout and an explicit confirmation flag.

## Operator checks still required

1. Run the native app and all bundled plugins on the intended Amiga setup,
   including Protocol 1 replies, previews, error handling and installation/update.
   Physical minimum-memory and 68000 certification remain open.
2. Verify project-specific credential fields and repository permissions. Do not
   reuse another project's scoped token. Confirm repository visibility and the
   exact source commit/tag before the first source upload or release.
3. Exercise publishing adapters against dedicated test repositories before using
   a production tag. Offline mocks do not certify live hosting configuration.
4. If submitting to Aminet, configure and review a real uploader contact and the
   readme. A successful submission awaits moderator approval.
5. Review outputs/release/manifest.json and all prepared files. The website's
   planned download links assume v0.1 and exact package filenames. Update its
   awaiting-publication text only when the actual release is available.

See RELEASING.md for commands and receipt recovery. No source pushes or public
uploads are performed by build/check/prepare or ordinary automated tests.
