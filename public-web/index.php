<?php
declare(strict_types=1);
define('BEAM68_PAGE', true);
require __DIR__ . '/includes/site.php';
page_start('Beam68 — Find your flow on Workbench.', 'A lightweight, keyboard-first launcher for classic AmigaOS. Find applications, calculate and explore your Amiga without leaving Workbench.', 'home');
?>
<section class="hero wrap">
<div class="hero-copy"><p class="eyebrow"><span class="version-dot"></span> CLASSIC AMIGA. FRESH MOMENTUM.</p>
<h1>Less searching.<br><span>More doing.</span></h1>
<p class="hero-lead">Your Amiga, one shortcut away. Find apps, explore drawers and get little jobs done — without leaving Workbench.</p>
<div class="actions"><a class="button button-primary" href="download.php">Download <span aria-hidden="true">↗</span></a><a class="button button-quiet" href="screenshots.php">Take a look <span aria-hidden="true">→</span></a></div>
<p class="hero-meta">Version 0.1 <span>·</span> AmigaOS 3.x <span>·</span> Open source</p>
</div>
<div class="hero-art"><div class="art-caption"><span>01 / FIND YOUR NEXT APP</span><span>BEAM68</span></div>
<a href="screenshots.php" class="hero-screen" aria-label="Explore Beam68 screenshots"><?php screenshot_image('fresh-search.png', '', true); ?></a>
<div class="shortcut"><span class="shortcut-label">A small shortcut.<br>A quicker way around.</span><span class="keys"><kbd>Ctrl</kbd><span>+</span><kbd>Alt</kbd><span>+</span><kbd>Space</kbd></span></div>
</div>
</section>
<div class="spec-strip wrap"><span><strong>68000+</strong> CPU target</span><span><strong>13</strong> bundled plugins</span><span><strong>2</strong> native apps</span><span><strong>No FPU</strong> required</span></div>
<section class="section wrap" id="features">
<div class="section-heading"><div><p class="eyebrow">A FAMILIAR DESKTOP. A QUICKER ROUTE.</p><h2>Stay in your flow.</h2></div><p>Type what you need. Beam68 takes care of finding it.</p></div>
<div class="features">
<article><span class="feature-number">01</span><h3>Find it your way.</h3><p>Names, abbreviations, aliases or a small typo. Search selected drawers and narrow things down with scopes like <code>app:</code>, <code>prefs:</code> and <code>fav:games</code>.</p></article>
<article><span class="feature-number">02</span><h3>Do the little things.</h3><p>Calculate in place, check the clock or control your music. Thirteen bundled plugins turn a launcher into a handy everyday companion.</p></article>
<article><span class="feature-number">03</span><h3>Keep it yours.</h3><p>Workbench colors or a custom theme. Pick your fonts, spacing and indexed drawers in the separate, native Preferences app.</p></article>
</div>
</section>
<section class="showcase-band"><div class="section wrap showcase">
<div><p class="eyebrow">SMALL TOOL. USEFUL TRICKS.</p><h2>More than<br>an app list.</h2><p>Use the calculator without opening another window. Browse command help with F1. Press Right on a drawer to explore, then Left to go back.</p><div class="query-examples"><span><code>=12*(3+4)</code><small>A quick calculation</small></span><span><code>prefs:</code><small>Find a preference</small></span><span><code>fav:games</code><small>Jump to your favorites</small></span></div><a class="text-link" href="screenshots.php">Explore all 17 app views <span aria-hidden="true">→</span></a></div>
<figure class="calculator-figure"><div class="art-caption"><span>02 / A QUICK CALCULATION</span><span>DEC · HEX · BIN</span></div><?php screenshot_image('fresh-calculator.png'); ?><figcaption>84 in decimal. $54 in hexadecimal. Same answer, your format.</figcaption></figure>
</div></section>
<section class="section wrap preferences-section">
<div class="preferences-image"><a href="screenshots.php" aria-label="See Preferences screenshots"><?php screenshot_image('fresh-appearance.png'); ?></a><span class="image-note">Beam68Prefs / Appearance</span></div>
<div><p class="eyebrow">FEELS AT HOME ON WORKBENCH.</p><h2>Your setup.<br>Your shortcuts.</h2><p>A resident Commodity for quick access. A separate Preferences app for everything else. Nine sections keep your index, favorites, plugins and appearance in one place.</p><ul class="check-list"><li>Incremental indexing and a persistent cache</li><li>Remembered window placement per public screen</li><li>Adjustable fonts, contrast and row spacing</li><li>Plain-text settings, themes and a plugin SDK</li></ul><a class="text-link" href="https://github.com/retrocs-org/beam68">Browse the source <span aria-hidden="true">↗</span></a></div>
</section>
<section class="section wrap getting-started" id="download">
<div class="section-heading"><div><p class="eyebrow">READY WHEN YOU ARE.</p><h2>Make room for Beam68.</h2></div><a class="button button-primary" href="download.php">Download Beam68 <span aria-hidden="true">↗</span></a></div>
<div class="steps"><article><span>01 / INSTALL</span><h3>Unpack. Open Install.</h3><p>Choose a destination and your plugins. Existing settings stay in <code>ENVARC:Beam68</code>.</p></article><article><span>02 / MAKE IT YOURS</span><h3>Pick your drawers.</h3><p>Open <code>SYS:Prefs/Beam68Prefs</code>. Choose Use for this session, or Save to keep your settings.</p></article><article><span>03 / GET GOING</span><h3>Press Ctrl + Alt + Space.</h3><p>Type a name and press Enter. F1 opens help; F7 brings back your query history.</p></article></div>
<p class="release-note"><span class="version-dot"></span> Version 0.1 is preparing for publication. <a href="download.php">Browse release packages →</a></p>
</section>
<section class="requirements wrap"><div><p class="eyebrow">THE PRACTICAL DETAILS</p><h2>Built for classic AmigaOS.</h2><p>AmigaOS 3.0 or later and a 68000 or later CPU. AmigaOS 3.2 and a 68020 are recommended. The included binaries target 68000 and need no FPU.</p><p>Free memory needs depend on your index and plugins. Physical minimum-memory and 68000 certification remain open.</p></div><div><h3>A few things to know.</h3><p>Opening drawers through Workbench needs workbench.library V44+. Optional features use apps such as WHDLoad, OpenURL or your music player. English is built in; German and Swedish catalogues are included.</p><p>Plugins are native code. Install only code you trust; capability labels do not restrict access. Shell commands can be disabled in Preferences.</p><a class="text-link" href="source.php">Source &amp; compatibility →</a></div></section>
<section class="wrap faq-section" id="faq" aria-labelledby="faq-title">
<div class="section-heading"><div><p class="eyebrow">A FEW QUICK ANSWERS</p><h2 id="faq-title">Good to know.</h2></div><p>Getting started with Beam68.</p></div>
<div class="faq-list">
<details><summary>What do I need to run Beam68?</summary><div class="faq-answer"><p>AmigaOS 3.0 or later and a 68000 or later CPU. No FPU is required. AmigaOS 3.2 and a 68020 are recommended. Memory use depends on the size of your index and the plugins you enable; minimum-memory and physical 68000 testing remain open. See <a href="source.php#compatibility">compatibility notes</a>.</p></div></details>
<details><summary>How do I install and open it?</summary><div class="faq-answer"><p>Unpack the Amiga binary package and open Install. Choose your destination and plugins, then launch Beam68. Press Ctrl + Alt + Space to open the launcher. Use Beam68Prefs to choose which drawers to index and adjust the shortcut.</p></div></details>
<details><summary>Where can I download Beam68?</summary><div class="faq-answer"><p>The <a href="download.php">Download page</a> links to tagged GitHub releases, with separate LHA binary and source ZIP packages. Version 0.1 is awaiting publication; its links will work once the release files are uploaded.</p></div></details>
<details><summary>What can I do besides launch applications?</summary><div class="faq-answer"><p>Calculate, browse drawers, find files, check memory and dates, or use clipboard and system tools. Thirteen bundled plugins provide extra commands. Some integrations need their host applications. Press F1 to browse command help, or explore the <a href="marketplace.php#plugins">plugin list</a>.</p></div></details>
<details><summary>Can I change its appearance or add my own plugins?</summary><div class="faq-answer"><p>Yes. Preferences offers fonts, spacing and themes, with four theme files included. The <a href="developers.php">Developers page</a> documents the plugin SDK and portable theme format. All bundled native plugins use Protocol 1. Third-party marketplace listings are planned.</p></div></details>
<details><summary>Is Beam68 open source?</summary><div class="faq-answer"><p>Yes, Beam68 is licensed under MIT. The code and SDK are available through the <a href="https://github.com/retrocs-org/beam68">GitHub repository</a>. Visit <a href="source.php">Source</a> for toolchain requirements and instructions to build the app yourself.</p></div></details>
</div></section>
<?php page_end(); ?>
