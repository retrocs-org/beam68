<?php
declare(strict_types=1);
if (!defined('BEAM68_PAGE')) { http_response_code(404); exit; }

const BEAM68_GITHUB = 'https://github.com/retrocs-org/beam68';
const BEAM68_RELEASE_API = 'https://api.github.com/repos/retrocs-org/beam68/releases/latest';
const BEAM68_RELEASE_TTL = 600;

function release_normalize(array $raw): ?array {
    $tag = $raw['tag_name'] ?? null;
    if (!is_string($tag) || !preg_match('/^[A-Za-z0-9][A-Za-z0-9._-]{0,79}$/D', $tag) ||
        ($raw['draft'] ?? true) !== false || ($raw['prerelease'] ?? true) !== false ||
        ($raw['html_url'] ?? '') !== BEAM68_GITHUB . '/releases/tag/' . rawurlencode($tag)) return null;
    $date = $raw['published_at'] ?? '';
    if (!is_string($date) || !preg_match('/^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$/D', $date) || strtotime($date) === false) return null;
    if (!is_array($raw['assets'] ?? null)) return null;
    $assets = [];
    foreach ($raw['assets'] as $asset) {
        if (!is_array($asset)) return null;
        $name = $asset['name'] ?? '';
        if (!is_string($name) || !preg_match('/^Beam68-[A-Za-z0-9][A-Za-z0-9._-]{0,110}$/D', $name)) continue;
        if (isset($assets[$name])) return null;
        $expected = BEAM68_GITHUB . '/releases/download/' . rawurlencode($tag) . '/' . rawurlencode($name);
        if (($asset['browser_download_url'] ?? '') !== $expected || ($asset['state'] ?? '') !== 'uploaded' ||
            !is_int($asset['size'] ?? null) || $asset['size'] < 0) continue;
        $assets[$name] = ['name' => $name, 'url' => $expected, 'size' => $asset['size']];
    }
    $runtime = array_values(array_filter($assets, fn($a) => (bool)preg_match('/^Beam68-\d+(?:\.\d+)+(?:-[A-Za-z0-9.-]+)?\.lha$/D', $a['name'])));
    $sdk = array_values(array_filter($assets, fn($a) => (bool)preg_match('/^Beam68-\d+(?:\.\d+)+(?:-[A-Za-z0-9.-]+)?-plugin-sdk\.zip$/D', $a['name'])));
    $runtime = count($runtime) === 1 ? $runtime[0] : null;
    $sdk = count($sdk) === 1 ? $sdk[0] : null;
    $pair = function(?array $asset) use ($assets): ?array {
        if ($asset === null) return null;
        $asset['checksum'] = $assets[$asset['name'] . '.sha256']['url'] ?? null;
        return $asset;
    };
    return ['tag' => $tag, 'url' => $raw['html_url'], 'published_at' => $date,
        'source_url' => BEAM68_GITHUB . '/archive/refs/tags/' . rawurlencode($tag) . '.zip',
        'runtime' => $pair($runtime), 'sdk' => $pair($sdk)];
}

function release_fetch(string $etag = ''): array {
    $headers = ['Accept: application/vnd.github+json', 'User-Agent: Beam68-Website', 'X-GitHub-Api-Version: 2022-11-28'];
    if (preg_match('/^[\x20-\x7e]{1,200}$/D', $etag)) $headers[] = 'If-None-Match: ' . $etag;
    $body = ''; $status = 0; $receivedEtag = '';
    if (function_exists('curl_init')) {
        $handle = curl_init(BEAM68_RELEASE_API);
        curl_setopt_array($handle, [CURLOPT_HTTPHEADER => $headers, CURLOPT_FOLLOWLOCATION => false,
            CURLOPT_CONNECTTIMEOUT => 2, CURLOPT_TIMEOUT => 4, CURLOPT_SSL_VERIFYPEER => true, CURLOPT_SSL_VERIFYHOST => 2,
            CURLOPT_HEADERFUNCTION => function($ch, string $line) use (&$receivedEtag): int {
                if (stripos($line, 'ETag:') === 0) $receivedEtag = trim(substr($line, 5));
                return strlen($line);
            }, CURLOPT_WRITEFUNCTION => function($ch, string $chunk) use (&$body): int {
                if (strlen($body) + strlen($chunk) > 1048576) return 0;
                $body .= $chunk; return strlen($chunk);
            }]);
        $ok = curl_exec($handle);
        if ($ok !== false) $status = (int)curl_getinfo($handle, CURLINFO_RESPONSE_CODE);
        curl_close($handle);
    } elseif (in_array(strtolower((string)ini_get('allow_url_fopen')), ['1','on','true','yes'], true) && extension_loaded('openssl')) {
        $context = stream_context_create(['http' => ['method' => 'GET', 'header' => implode("\r\n", $headers),
            'timeout' => 4, 'ignore_errors' => true, 'follow_location' => 0],
            'ssl' => ['verify_peer' => true, 'verify_peer_name' => true, 'allow_self_signed' => false]]);
        $stream = @fopen(BEAM68_RELEASE_API, 'rb', false, $context);
        if ($stream !== false) {
            $body = (string)stream_get_contents($stream, 1048577);
            foreach (stream_get_meta_data($stream)['wrapper_data'] ?? [] as $line) {
                if (preg_match('/^HTTP\/\S+ (\d{3})/', $line, $match)) $status = (int)$match[1];
                if (stripos($line, 'ETag:') === 0) $receivedEtag = trim(substr($line, 5));
            }
            fclose($stream);
            if (strlen($body) > 1048576) $status = 0;
        }
    }
    if (!preg_match('/^[\x20-\x7e]{1,200}$/D', $receivedEtag)) $receivedEtag = '';
    return ['status' => $status, 'body' => $body, 'etag' => $receivedEtag];
}

function latest_release(?callable $fetch = null, ?string $directory = null, ?int $now = null): array {
    $fetch ??= 'release_fetch'; $now ??= time();
    $directory ??= sys_get_temp_dir() . '/beam68-web-' . substr(hash('sha256', __DIR__), 0, 16);
    $cache = null; $storedRelease = null;
    $canCache = !is_link($directory) && (is_dir($directory) || @mkdir($directory, 0700, true));
    $file = $directory . '/latest.json';
    if ($canCache && !is_link($file) && is_file($file) && filesize($file) <= 1100000) {
        $decoded = json_decode((string)@file_get_contents($file), true);
        if (is_array($decoded) && is_int($decoded['checked_at'] ?? null) && $decoded['checked_at'] <= $now + 60) {
            $candidate = is_array($decoded['payload'] ?? null) ? release_normalize($decoded['payload']) : null;
            if ($candidate !== null || ((($decoded['missing'] ?? false) === true || ($decoded['last_failed'] ?? false) === true) && !isset($decoded['payload']))) {
                $cache = $decoded; $storedRelease = $candidate;
            }
        }
    }
    if ($cache !== null && $now - $cache['checked_at'] < BEAM68_RELEASE_TTL) {
        if (($cache['last_failed'] ?? false) === true) {
            $valid = $storedRelease !== null && $now - (int)($cache['fetched_at'] ?? 0) < 86400;
            return ['release' => $valid ? $storedRelease : null, 'state' => $valid ? 'stale' : 'unavailable'];
        }
        return ['release' => $storedRelease, 'state' => $storedRelease ? 'fresh' : 'missing'];
    }
    $fallback = ['release' => null, 'state' => 'unavailable'];
    if ($storedRelease !== null && $now - (int)($cache['fetched_at'] ?? 0) < 86400) $fallback = ['release' => $storedRelease, 'state' => 'stale'];
    $lock = null;
    if ($canCache) {
        if (is_link($directory . '/refresh.lock')) return $fallback;
        $lock = @fopen($directory . '/refresh.lock', 'c');
        if ($lock === false || !flock($lock, LOCK_EX | LOCK_NB)) {
            if (is_resource($lock)) fclose($lock);
            return $fallback;
        }
    }
    try {
        $response = $fetch((string)($cache['etag'] ?? ''));
        $raw = $response['status'] === 200 ? json_decode($response['body'], true) : null;
        $release = is_array($raw) ? release_normalize($raw) : null;
        $save = null;
        if ($release !== null) {
            $save = ['checked_at' => $now, 'fetched_at' => $now, 'payload' => $raw, 'etag' => $response['etag']];
            $result = ['release' => $release, 'state' => 'fresh'];
        } elseif ($response['status'] === 304 && $storedRelease !== null) {
            $save = $cache; $save['checked_at'] = $now; $save['fetched_at'] = $now;
            unset($save['last_failed']);
            $result = ['release' => $storedRelease, 'state' => 'fresh'];
        } elseif ($response['status'] === 404) {
            $save = ['checked_at' => $now, 'missing' => true];
            $result = ['release' => null, 'state' => 'missing'];
        } else {
            // Retry at most once per minute during an outage, retaining a bounded stale release.
            $save = $cache ?? [];
            $save['last_failed'] = true;
            $save['checked_at'] = $now - BEAM68_RELEASE_TTL + 60;
            $result = $fallback;
        }
        if ($save !== null && $canCache && !is_link($file)) {
            $temp = @tempnam($directory, '.latest-');
            if ($temp !== false) {
                @chmod($temp, 0600);
                if (@file_put_contents($temp, json_encode($save, JSON_UNESCAPED_SLASHES)) !== false) @rename($temp, $file);
                if (is_file($temp)) @unlink($temp);
            }
        }
        return $result;
    } finally {
        if (is_resource($lock)) { flock($lock, LOCK_UN); fclose($lock); }
    }
}

function release_page_data(): array {
    // Fixtures are a CLI-only test input, never a web request parameter or URL.
    $fixture = PHP_SAPI === 'cli' ? getenv('BEAM68_WEB_RELEASE_FIXTURE') : false;
    if ($fixture !== false && $fixture !== '') {
        $raw = json_decode((string)file_get_contents($fixture), true);
        $release = is_array($raw) ? release_normalize($raw) : null;
        return ['release' => $release, 'state' => $release ? 'fresh' : 'unavailable'];
    }
    return latest_release();
}

function release_size(int $size): string {
    return $size >= 1048576 ? number_format($size / 1048576, 1) . ' MiB' : number_format($size / 1024, 1) . ' KiB';
}
