<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
$current = screenshots();
page_start('Beam68 — Screenshots.', 'Explore application search, Aminet packages, Wikipedia summaries and bundled tools.', 'screenshots');
?>
<section class="page-intro wrap"><p class="eyebrow">THE APP, UP CLOSE</p><h1>A look inside.</h1><p>Find applications, explore packages and use everyday tools. Click an image for a closer look. These captures illustrate the features shown; they do not document every current Preferences control or newer integration.</p></section>
<section class="wrap gallery-section gallery-layout" aria-label="Beam68 features"><div class="gallery"><?php foreach ($current as $item) screenshot_card($item); ?></div></section>
<?php page_end(); ?>
