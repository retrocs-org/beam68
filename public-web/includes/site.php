<?php
declare(strict_types=1);
if (!defined('BEAM68_PAGE')) {
    http_response_code(404);
    exit;
}
const BEAM68_SITE_VERSION = '0.5';
function marketplace_catalog(): array {
    static $catalog;
    if ($catalog === null) {
        $catalog = json_decode(file_get_contents(__DIR__ . '/marketplace.json'), true, 512, JSON_THROW_ON_ERROR);
    }
    return $catalog;
}
function escape(string $value): string {
    return htmlspecialchars($value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}
function screenshots(): array {
    static $shots;
    if ($shots === null) {
        $shots = json_decode(file_get_contents(__DIR__ . '/../images/screenshots.json'), true, 512, JSON_THROW_ON_ERROR);
    }
    return $shots;
}
function shot(string $file): array {
    foreach (screenshots() as $item) {
        if ($item['file'] === $file) return $item;
    }
    throw new RuntimeException('Screenshot not found.');
}
function screenshot_image(string $file, string $class = '', bool $eager = false): void {
    $item = shot($file);
    if (!preg_match('/^[a-zA-Z0-9._-]+\.png$/D', $item['file'])) {
        throw new RuntimeException('Invalid screenshot path.');
    }
    echo '<img class="' . escape($class) . '" src="images/' . escape($item['file']) . '" alt="' . escape($item['description']) . '" width="' . (int)$item['width'] . '" height="' . (int)$item['height'] . '" loading="' . ($eager ? 'eager' : 'lazy') . '" decoding="async">';
}
function page_start(string $title, string $description, string $page): void {
    header('Content-Type: text/html; charset=UTF-8');
    $routes = ['home' => '', 'screenshots' => 'screenshots.php', 'download' => 'get.php', 'developers' => 'developers.php', 'source' => 'source.php', 'marketplace' => 'marketplace.php'];
    $canonical = 'https://beam68.retrocs.org/' . $routes[$page];
    ?>
<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="description" content="<?= escape($description) ?>">
<meta name="theme-color" content="#080f1f">
<meta property="og:type" content="website">
<meta property="og:title" content="<?= escape($title) ?>">
<meta property="og:description" content="<?= escape($description) ?>">
<meta property="og:url" content="<?= escape($canonical) ?>">
<title><?= escape($title) ?></title>
<link rel="canonical" href="<?= escape($canonical) ?>">
<link rel="stylesheet" href="site.css">
</head><body id="top">
<a class="skip-link" href="#main">Skip to content</a>
<header class="site-header">
<a class="brand" href="index.php" aria-label="Beam68 home">beam<span>68</span><span class="brand-symbol" aria-hidden="true">↗</span></a>

<nav aria-label="Main navigation">
<a href="index.php"<?= $page === 'home' ? ' aria-current="page"' : '' ?>>Overview</a>
<a href="screenshots.php"<?= $page === 'screenshots' ? ' aria-current="page"' : '' ?>>Screenshots</a>
<a href="get.php"<?= $page === 'download' ? ' aria-current="page"' : '' ?>>Get Beam68</a>
<a href="developers.php"<?= $page === 'developers' ? ' aria-current="page"' : '' ?>>Developers</a>
<a href="source.php"<?= $page === 'source' ? ' aria-current="page"' : '' ?>>Source</a>
<a href="marketplace.php"<?= $page === 'marketplace' ? ' aria-current="page"' : '' ?>>Plugins &amp; themes</a>
</nav>

</header>
<main id="main" class="site-main">
<?php }
function page_end(): void { ?>
</main>
<footer class="site-footer wrap"><span>Beam68 <?= escape(BEAM68_SITE_VERSION) ?> · Classic AmigaOS</span><span>Open source · MIT License</span><a href="#top">Back to top ↑</a></footer>
</body></html>
<?php }
function screenshot_card(array $item): void { ?>
<figure class="shot-card">
<a class="shot-image" href="images/<?= escape($item['file']) ?>" aria-label="Open full image: <?= escape($item['title']) ?>"><?php screenshot_image($item['file']); ?></a>
<figcaption><h3><?= escape($item['title']) ?></h3><p><?= escape($item['description']) ?></p></figcaption>
</figure>
<?php }
