<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
$current = screenshots();
page_start('Beam68 — Screenshots.', 'Explore application search, Aminet package browsing, command help and launcher settings.', 'screenshots');
?>
<section class="page-intro wrap"><p class="eyebrow">THE APP, UP CLOSE</p><h1>A look inside.</h1><p>Find applications, explore packages and make the launcher your own. Click an image for a closer look.</p></section>
<section class="wrap gallery-section gallery-layout" aria-label="Beam68 features"><div class="gallery"><?php foreach ($current as $item) screenshot_card($item); ?></div></section>
<?php page_end(); ?>
