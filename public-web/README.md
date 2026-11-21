# Beam68 website

Website: https://beam68.retrocs.org/
Repository: https://github.com/retrocs-org/beam68
Downloads: https://github.com/retrocs-org/beam68/releases

## Hosting

The website uses PHP 8.2 or later, semantic HTML and one local CSS file. It
needs no database, framework, package installation, JavaScript or writable
application directory. JSON support is part of PHP; no extra extensions are
required by the pages.

Copy the contents of public-web/ to the web server document root, including
index.php, screenshots.php, get.php, download.php, developers.php, source.php, marketplace.php,
validation.php and development.php (legacy redirects), includes/, images/, site.css,
robots.txt and sitemap.xml. Configure the server to execute PHP files and use
index.php as the directory index. Do not serve PHP files as plain text.

Apache can use the supplied .htaccess for the directory index and redirects
from the old .html addresses. On other servers, configure equivalent redirects
for /index.html, /screenshots.html and /validation.html. Remove old HTML pages
from an existing upload so they cannot serve stale content.

For a local preview:

```sh
php -S 127.0.0.1:8080 -t public-web
```

## Structure and checks

includes/site.php provides the shared header, footer, screenshot inventory and
escaped image/caption rendering. Only fixed page definitions and the supplied
JSON inventory are read; pages do not accept query-string content or set cookies.
Direct requests to the shared include return 404. The stylesheet uses system
fonts, responsive layouts and reduced-motion preferences. Navigation and gallery
links work without scripting; extra reference images use native details controls.

The gallery describes features rather than displaying version labels. It includes
application search, command help, Aminet browsing, and
Wikipedia search and native article-summary captures, File Finder results and
Date Utilities weekday lookup, Calculator results and available-memory checks. The About screenshot, outdated nine-section Preferences capture and
older-version reference gallery have been removed. Every screenshot is cropped
to the application window. images/screenshots.json retains internal capture
provenance and dimensions without adding version labels to public captions.
Wikipedia text attribution and its licence remain visible in the summary window.

Run python3 tests/test_public_web.py with PHP on PATH (or set BEAM68_PHP to its
executable) to render and check all pages, local assets, canonical URLs, image
metadata and escaping. PHP syntax checks run before rendering. This test needs
PHP and is included in make host-test. Install PHP CLI on the development host
before running the complete suite.

Release ZIPs stay outside this directory. The website ZIP contains the contents
at its root for manual upload. Build scripts do not deploy the site or publish
releases. Runtime compatibility limits remain in BUILD_STATUS.md.

## Marketplace

Marketplace metadata in includes/marketplace.json mirrors bundled manifests and
themes. Update it when the bundled inventory changes. Repository documentation
links assume the main branch; Developers and Source cover extension authoring
and application builds separately.

## Automatic Get Beam68 downloads

get.php retrieves the latest published stable release from the public GitHub
releases API for retrocs-org/beam68. download.php redirects permanently to it.
The page lists only actual Amiga LHA and plugin SDK assets, their sizes and
attached checksum files, with tagged source and release-note links. No token,
database or browser JavaScript is used. Drafts/prereleases are excluded.

PHP needs outbound HTTPS to api.github.com through cURL or allow_url_fopen with
OpenSSL. TLS verification stays enabled. The response is size-limited and has a
four-second request timeout. A private per-site folder in the system temporary
directory holds a ten-minute cache and a refresh lock. Make sure the PHP user
can write to its temporary directory. Conditional ETags avoid downloading an
unchanged response. Outages retry at most once per minute and may show validated
cached details for up to 24 hours, clearly labelled. Without a usable cache, the
page links directly to GitHub releases rather than inventing package links.

Run tests/test_web_releases.py with PHP CLI for offline cache, failure and URL
validation checks. BEAM68_WEB_RELEASE_FIXTURE is a CLI-only test fixture; it has
no effect on HTTP requests. Website updates do not replace published packages.

## Design and content

The site uses a midnight-navy background, bright blue actions and modern
sans-serif typography. A compact horizontal navigation replaces the earlier
sidebar. The overview has a centred introduction, a real application showcase,
a three-step workflow and a toolkit list. The gallery uses image-and-caption
rows; downloads use distinct application and SDK panels. Developer and source
pages use a dedicated documentation layout. There are no external fonts or
scripts. Main navigation has no external GitHub link. Release downloads
remain separate from build instructions and SDK documentation. The plugin list shows only names and short descriptions. Independent versions,
examples and default activation states remain in its internal metadata; keep
those fields in sync with manifests and the installer.

The overview includes six grouped feature summaries and twenty-four collapsible
FAQ answers. Keep examples and dependencies consistent with the
corresponding plugin manifests and documentation.

Application and SDK downloads display GitHub’s per-asset download counts when
available, refreshed with the existing ten-minute release cache. These counts
cover GitHub asset downloads only, not Aminet, GitLab or unique users. Unknown
counts are omitted; a reported zero is displayed.

## Current content

The overview, footer and plugin list describe 0.5. The download page queries
GitHub for the latest stable package; source-version copy never selects a
download asset or marks an unpublished package as available. Do not hard-code a release tag into download links.

Current feature copy covers scoped F7 history, F6 query favorites, stable result
selection, read-only retry feedback, Aminet completion choices, optional AmiHASS,
scan controls, import review and plugin Restore defaults. The plugin list remains
names and short descriptions only. Screenshots retain their original provenance;
no new capture or current-layout claim is implied. Native test scope and remaining
physical-machine limits are described on Source and in BUILD_STATUS.md.

Website edits are prepared locally for manual upload; do not deploy automatically.

0.5 copy also covers result actions, categories, two-line rows, explicit local
completion, readiness checks, Aminet speed/ETA and optional session clipboard
history. No deployment is performed by release preparation.
