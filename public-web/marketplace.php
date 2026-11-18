<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
$catalog = marketplace_catalog();
page_start('Beam68 — Marketplace.', 'Explore the plugins and themes included with Beam68.', 'marketplace');
?>
<section class="page-intro wrap"><p class="eyebrow">EXTENSIONS</p><h1>Your toolkit.</h1><p>The bundled plugins and portable themes in the <?= escape(BEAM68_SITE_VERSION) ?> source. Check the download page and release notes for the version available to install.</p><div class="jump-links"><a href="#plugins">Plugins (<?= count($catalog['plugins']) ?>)</a><a href="#themes">Themes (<?= count($catalog['themes']) ?>)</a></div></section>
<section id="plugins" class="wrap section catalog-layout"><div class="section-intro"><h2>Plugins</h2><p>Included tools for your Workbench.</p></div><div class="plugin-list"><?php foreach ($catalog['plugins'] as $plugin): ?><article class="plugin-list-item"<?= $plugin['NAME'] === 'Aminet Browser' ? ' id="aminet-browser"' : '' ?>><h3><?= escape($plugin['NAME']) ?></h3><p><?= escape($plugin['DESCRIPTION']) ?></p></article><?php endforeach; ?></div></section>
<section id="themes" class="wrap section catalog-layout"><div class="section-intro"><h2>Themes</h2><p class="section-note">Every theme is editable, with Restore original palette in Preferences. These four portable files are included alongside the built-in palettes.</p></div><div class="theme-list"><?php foreach ($catalog['themes'] as $theme): ?><article class="theme-row"><div><h3><?= escape($theme['name']) ?></h3><p><?= escape($theme['description']) ?></p></div><a href="https://github.com/retrocs-org/beam68/blob/main/<?= escape($theme['path']) ?>">Theme file ↗</a></article><?php endforeach; ?></div><p class="section-note">Want to create one? Read the <a href="developers.php">plugin and theme development guides</a>.</p></section>
<?php page_end(); ?>
