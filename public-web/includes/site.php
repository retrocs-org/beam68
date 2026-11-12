<?php
declare(strict_types=1);
if (!defined('BEAM68_PAGE')) {
    http_response_code(404);
    exit;
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
    $routes = ['home' => '', 'screenshots' => 'screenshots.php', 'download' => 'download.php', 'developers' => 'developers.php', 'source' => 'source.php', 'marketplace' => 'marketplace.php'];
    $canonical = 'https://beam68.retrocs.org/' . $routes[$page];
    ?>
<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="description" content="<?= escape($description) ?>">
<meta name="theme-color" content="#1e1917">
<title><?= escape($title) ?></title>
<link rel="canonical" href="<?= escape($canonical) ?>">
<link rel="stylesheet" href="site.css">
</head><body id="top">
<a class="skip-link" href="#main">Skip to content</a>
<header class="site-header wrap">
<a class="brand" href="index.php" aria-label="Beam68 home"><span class="brand-mark" aria-hidden="true"></span>beam<span>68</span></a>
<nav aria-label="Main navigation">
<a href="index.php"<?= $page === 'home' ? ' aria-current="page"' : '' ?>>Overview</a>
<a href="screenshots.php"<?= $page === 'screenshots' ? ' aria-current="page"' : '' ?>>Screenshots</a>
<a href="download.php"<?= $page === 'download' ? ' aria-current="page"' : '' ?>>Download</a>
<a href="developers.php"<?= $page === 'developers' ? ' aria-current="page"' : '' ?>>Developers</a>
<a href="source.php"<?= $page === 'source' ? ' aria-current="page"' : '' ?>>Source</a>
<a href="marketplace.php"<?= $page === 'marketplace' ? ' aria-current="page"' : '' ?>>Marketplace</a>
<a class="nav-source" href="https://github.com/retrocs-org/beam68">GitHub <span aria-hidden="true">↗</span></a>
</nav>
</header>
<main id="main">
<?php }
function page_end(): void { ?>
</main>
<footer class="site-footer wrap">
<div><a class="brand" href="index.php"><span class="brand-mark" aria-hidden="true"></span>beam<span>68</span></a><p>A little less searching.<br>A little more Amiga.</p></div>
<div class="footer-links"><a href="https://github.com/retrocs-org/beam68">Source code ↗</a><a href="https://github.com/retrocs-org/beam68/releases">GitHub Releases ↗</a><a href="download.php">Download</a><a href="developers.php">Developers</a><a href="source.php">Source</a><a href="marketplace.php">Marketplace</a></div>
<div class="footer-note"><span class="version-dot"></span> Version 0.1 · MIT License<p>No cookies. No trackers.<br>Made for classic AmigaOS.</p><a href="#top">Back to top ↑</a></div>
</footer>
</body></html>
<?php }
function screenshot_card(array $item): void { ?>
<figure class="shot-card">
<a class="shot-image" href="images/<?= escape($item['file']) ?>" aria-label="Open full image: <?= escape($item['title']) ?>"><?php screenshot_image($item['file']); ?></a>
<figcaption><span class="eyebrow"><?= isset($item['app']) ? escape($item['app']) : 'Reference view' ?></span><h3><?= escape($item['title']) ?></h3><p><?= escape($item['description']) ?></p></figcaption>
</figure>
<?php }
