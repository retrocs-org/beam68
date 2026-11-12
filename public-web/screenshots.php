<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
page_start('Beam68 — A closer look.', 'See Beam68 application search, calculator, command help and native Preferences. Seventeen app-window views.', 'screenshots');
?>
<section class="page-intro wrap"><p class="eyebrow">THE APP, UP CLOSE.</p><h1>A little tour<br>of <span>Beam68.</span></h1><p>Application search, quick calculations and a setup that feels like yours. These are actual app windows, cropped to keep the focus where it belongs.</p><div class="page-meta"><span>9 version 0.1 views</span><span>8 reference views</span><span>Click any image to enlarge</span></div></section>
<section class="wrap gallery-section" aria-label="Version 0.1 screenshots"><div class="gallery-heading"><h2>Inside version 0.1</h2><p>Captured 11 November 2026</p></div><div class="gallery"><?php foreach (screenshots() as $item) { if (isset($item['app'])) screenshot_card($item); } ?></div></section>
<section class="wrap reference-section"><details><summary>More views &amp; alternative themes <span>8 reference images <span aria-hidden="true">+</span></span></summary><p class="reference-note">These images came with the project. Their capture dates were not recorded; they show additional layouts and themes rather than verification of the current build.</p><div class="gallery"><?php foreach (screenshots() as $item) { if (!isset($item['app'])) screenshot_card($item); } ?></div></details></section>
<section class="small-cta wrap"><div><p class="eyebrow">LESS CLICKING. MORE DOING.</p><h2>Find your next shortcut.</h2></div><a class="button button-primary" href="https://github.com/retrocs-org/beam68/releases">View releases ↗</a></section>
<?php page_end(); ?>
