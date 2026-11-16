<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
require __DIR__ . '/includes/github-releases.php';
$data = release_page_data();
$release = $data['release'];
page_start('Get Beam68 — Downloads for AmigaOS.', 'Download the latest published Beam68 release for AmigaOS, the plugin SDK and SHA-256 checksums.', 'download');
?>
<section class="page-intro wrap"><p class="eyebrow">DOWNLOAD</p><h1>Get Beam68.</h1><p>Download the app or the plugin SDK. Latest stable release details are loaded automatically from GitHub.</p>
<?php if ($release): ?><div class="page-meta"><span>Latest release · <?= escape($release['tag']) ?></span><span>Published <?= escape(gmdate('j F Y', (int)strtotime($release['published_at']))) ?></span><span>Free · MIT License</span></div><?php endif; ?></section>
<section class="wrap get-downloads" aria-labelledby="downloads-title"><h2 id="downloads-title" class="visually-hidden">Downloads</h2>
<?php if (!$release): ?><div class="download-notice" role="status"><h3><?= $data['state'] === 'missing' ? 'No release details are available.' : 'Check GitHub for the latest download.' ?></h3><p><?= $data['state'] === 'missing' ? 'The release service did not return a published release. Check the GitHub releases page for available packages.' : 'Release details are temporarily unavailable. You can still view and download published packages directly on GitHub.' ?></p><a class="button button-primary" href="https://github.com/retrocs-org/beam68/releases">View GitHub releases ↗</a></div>
<?php else: ?>
<?php if ($data['state'] === 'stale'): ?><p class="release-note" role="status">Showing cached release details while GitHub is temporarily unavailable.</p><?php endif; ?>
<div class="download-options"><?php foreach([['key' => 'runtime', 'title' => 'Beam68 for AmigaOS', 'label' => 'THE APPLICATION', 'description' => 'The launcher, Preferences and bundled plugins and themes. Unpack the LHA and open Install. Novice handles the standard setup automatically. Consult the attached release notes for the contents of this published package.', 'requirements' => 'AmigaOS 3.0+ · 68000+ · No FPU required', 'button' => 'Download LHA'], ['key' => 'sdk', 'title' => 'Plugin SDK', 'label' => 'FOR DEVELOPERS', 'description' => 'The C API, C and ARexx examples, AmigaGuide manual, package validator and license. Everything you need to write a plugin.', 'requirements' => 'Source kit · Amiga toolchain needed to build native plugins', 'button' => 'Download SDK ZIP']] as $item): $asset = $release[$item['key']]; ?>
<article class="get-download-row download-<?= escape($item['key']) ?>"><div><p class="eyebrow"><?= escape($item['label']) ?></p><h3><?= escape($item['title']) ?></h3><p><?= escape($item['description']) ?></p><p class="download-requirements"><?= escape($item['requirements']) ?></p></div><div class="download-actions">
<?php if ($asset): ?><span class="download-version">Release <?= escape($release['tag']) ?> · <?= escape(release_size($asset['size'])) ?></span><a class="button <?= $item['key'] === 'runtime' ? 'button-primary' : 'button-quiet' ?>" href="<?= escape($asset['url']) ?>"><?= escape($item['button']) ?> <span aria-hidden="true">↓</span></a><span class="download-filename"><?= escape($asset['name']) ?></span><?php if ($asset['checksum']): ?><a class="text-link" href="<?= escape($asset['checksum']) ?>">SHA-256 checksum</a><?php else: ?><span class="download-version">Checksum not attached to this release.</span><?php endif; ?>
<?php else: ?><p>This package is not attached to the latest release.</p><a class="text-link" href="<?= escape($release['url']) ?>">View release assets ↗</a><?php endif; ?></div></article>
<?php endforeach; ?></div>
<div class="get-release-links"><a class="text-link" href="<?= escape($release['url']) ?>">Release notes · <?= escape($release['tag']) ?> ↗</a><a class="text-link" href="<?= escape($release['source_url']) ?>">Tagged source ZIP ↗</a><a class="text-link" href="https://github.com/retrocs-org/beam68/releases">All releases ↗</a></div>
<?php endif; ?></section>
<section class="wrap section get-practical"><div><h2>Installation</h2><p>Unpack the LHA and open Install. Novice uses automatic defaults. Other modes expose destination and autostart options. Updates retain your settings.</p></div><div><h2>Requirements</h2><p>AmigaOS 3.0+, a 68000+ CPU and no FPU. AmigaOS 3.2 and a 68020 are recommended. Memory requirements depend on your index and plugins.</p><p><a href="source.php#compatibility">Compatibility notes</a> · <a href="source.php">Build from source</a></p></div></section>
<?php page_end(); ?>
