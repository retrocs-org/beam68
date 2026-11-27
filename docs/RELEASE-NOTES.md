## Features

- Add Open with and confirmed drawer exclusions to file actions; preserve Copy path and Information.
- Add C/F/K temperature conversion, reject values below absolute zero and retain meaningful small conversion results with up to twelve decimal places.
- Add offline Calculator conversions for bytes, time, length and metric mass, using integer ratios without an FPU.
- Add a confirmed `history:reset` command to clear learned ranking and recent launches while retaining favorites.

## Enhancements

- Add synthetic 10,000- and 50,000-item search/render benchmarks with latency percentiles, memory usage and cache-readiness measurements; keep the released index limit unchanged.
- Check exact path capitalization in runtime inventories, plugin/config/Installer references and source includes, and reject case-colliding source or archive paths.

- Apply appearance-only Preferences changes without reloading plugin manifests or restarting directory notifications and active indexing; retain the current query.
- Preserve active indexing and directory watches when other settings do not change the indexing configuration.

- Coalesce notifications for roots and up to 64 visited subdrawers, updating changed subtrees independently without stale-marking siblings or rebuilding unrelated sources.
- Use adaptive fast-path and binary placement for bounded search results, skipping known shared name prefixes without changing result order.
- Keep selected file paths and plugin row identities through asynchronous updates; require a fresh selection when the old target disappears.
- Add pending executable/drawer Browse controls to known plugin settings and show configured dependency names.
- Show the current indexed drawer, entry/item counters and completed-root progress; click the indexing banner or press Ctrl+F2 to cancel without losing results.
- Explain queries for disabled or unavailable plugins instead of showing an ordinary empty application search.
- Keep exact-name and prefix matches ahead of lower match classes, prioritize favorites and bounded usage within each class, and sort equal scores by name and path.
- Fit plugin settings to small screens with staged page navigation, Save/Cancel and defaults across all pages; disable unavailable plugin actions and priority boundaries.

## Bugs

- Invalidate pending plugin work on each query edit so queued away-and-back typing cannot revive an obsolete reply.

- Isolate plugin RAM copies by outstanding request slot and launcher instance, preventing collisions between identically named package drawers and changes beneath running children.
- Compare every cached package file, including supporting data and removed entries; fall back to the original package for unsupported or oversized trees.
- Remove completed detached file replies and leftover partial files before releasing their request slots; retain bounds on unfinished children and ignore obsolete replies.
- Suppress missing-volume requesters during indexing and report unavailable launch targets with reconnect/rescan guidance.

- Keep partial, cancelled or failed scans from certifying an incomplete cache.
- Match excluded volume roots and trailing-slash drawers correctly without matching similarly named siblings.
- Avoid rendering replacement plugin rows before selection identity is reconciled.
- Prevent launcher-control/footer and plugin-timeout overlaps on narrow 640x256 Preferences screens.
- Write portable directory separators in LHA packages so OLHa extracts nested drawers correctly.
