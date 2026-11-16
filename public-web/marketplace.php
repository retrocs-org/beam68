<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
$catalog = marketplace_catalog();
page_start('Beam68 — Marketplace.', 'Explore the plugins and themes included with Beam68.', 'marketplace');
?>
<section class="page-intro wrap"><h1>Plugins &amp; themes</h1><p>Included with the current Beam68 source. Third-party listings are planned.</p><div class="jump-links"><a href="#plugins">Plugins (<?= count($catalog['plugins']) ?>)</a><a href="#themes">Themes (<?= count($catalog['themes']) ?>)</a></div></section>
<section id="plugins" class="wrap section"><h2>Plugins</h2><div class="plugin-list"><?php foreach ($catalog['plugins'] as $plugin): ?><article class="plugin-list-item"<?= $plugin['NAME'] === 'Aminet Browser' ? ' id="aminet-browser"' : '' ?>><h3><?= escape($plugin['NAME']) ?></h3><p><?= escape($plugin['DESCRIPTION']) ?></p></article><?php endforeach; ?></div></section>
<section id="themes" class="wrap section"><h2>Themes</h2><p class="section-note">Every theme is editable, with Restore original palette in Preferences. These four portable files are included alongside the built-in palettes.</p><div class="theme-list"><?php foreach ($catalog['themes'] as $theme): ?><article class="theme-row"><div><h3><?= escape($theme['name']) ?></h3><p><?= escape($theme['description']) ?></p></div><a href="https://github.com/retrocs-org/beam68/blob/main/<?= escape($theme['path']) ?>">Theme file ↗</a></article><?php endforeach; ?></div><p class="section-note">Want to create one? Read the <a href="developers.php">plugin and theme development guides</a>.</p></section>
<?php page_end(); ?>
