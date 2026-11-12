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

The gallery contains nine version 0.1 views and eight labelled reference
views. Every image is cropped to the app window. images/screenshots.json records
captions, dimensions and capture notes. New captures retain build hashes.

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
