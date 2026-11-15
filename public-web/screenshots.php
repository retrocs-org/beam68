<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
$current = array_values(array_filter(screenshots(), fn($item) => ($item['version'] ?? '') === BEAM68_SITE_VERSION));
$earlier = array_values(array_filter(screenshots(), fn($item) => ($item['version'] ?? '') !== BEAM68_SITE_VERSION));
page_start('Beam68 — A closer look.', 'See the released Beam68 0.2 launcher and native Preferences, including its new running-version display.', 'screenshots');
?>
<section class="page-intro wrap"><p class="eyebrow">THE APP, UP CLOSE</p><h1>Your Workbench.<br><span>A quicker route.</span></h1><p>Actual Beam68 app windows, cropped to keep the focus on the application. Explore the latest 0.2 views, or open the earlier gallery for more layouts and themes.</p><div class="page-meta"><span><?= count($current) ?> new 0.2 views</span><span>Captured 15 November 2026</span><span>Click an image to enlarge</span></div></section>
<section class="wrap gallery-section" aria-label="Version 0.2 screenshots"><div class="gallery-heading"><h2>Inside Beam68 0.2</h2><p>Released apps · <a href="https://github.com/retrocs-org/beam68/releases/tag/0.2">Version 0.2 ↗</a></p></div><div class="gallery"><?php foreach ($current as $item) screenshot_card($item); ?></div></section>
<section class="wrap reference-section"><details><summary>Earlier versions &amp; additional themes <span><?= count($earlier) ?> earlier views <span aria-hidden="true">+</span></span></summary><p class="reference-note">The labelled 0.1 images were captured on 11 November 2026. Other reference images came with the project without recorded capture dates. These show additional layouts and themes, rather than the current 0.2 build.</p><div class="gallery"><?php foreach ($earlier as $item) screenshot_card($item); ?></div></details></section>
<section class="small-cta wrap"><div><p class="eyebrow">YOUR NEXT SHORTCUT</p><h2>Try it on your Workbench.</h2></div><a class="button button-primary" href="get.php">Get Beam68 →</a></section>
<?php page_end(); ?>
