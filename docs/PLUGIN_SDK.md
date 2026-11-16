# Beam68 plugin SDK — Protocol 1

Protocol 1 is the first public plugin protocol. All bundled native plugins use
its asynchronous selectable-result transport, including single-text results.
The manifest and reply message both declare version 1. Pre-release experimental
plugins must be rebuilt; version 2 manifests are rejected.

The complete authoring reference is [sdk/README.md](../sdk/README.md), with
[the API](../sdk/include/beam68plugin.h), [implementation](../sdk/src/beam68plugin.c)
and [examples](../sdk/Example/). The standalone AmigaGuide is
[sdk/Beam68PluginSDK.guide](../sdk/Beam68PluginSDK.guide).

## Invocation and results

```text
<plugin> <HEX> <reserved-reply-file> BEAM68_REPLY <serial>
```

Validate with `beam68_plugin_input()` before any work. Use
`beam68_plugin_text(argc, argv, text)` or `beam68_plugin_error(argc, argv, text)`
for one result; use the row API for up to eight TEXT, OPEN or QUERY results.
The launcher owns delivered Exec messages. Missing ports or allocation failures
return 20, without a lossy file fallback. Input and display text are bounded to
120 bytes; native plugins must target 68000 / AmigaOS 3.0+ and a small Shell stack.

ARexx scripts use a separate text transport within Protocol 1: `RX script.rexx
<HEX> <reply-file>`. Write a single `TEXT=` or `ERROR=` line atomically through
`<reply-file>.part`; scripts do not send native Exec messages. The native
ARexx Runner and Music Controls plugins use the same row messages as all other
bundled executables.

## Execution and safety

Execution is asynchronous. A timeout or Escape stops the launcher's wait, not
the child process. Request serials reject late native replies. Live previews
must be quick and free of side effects. Three consecutive errors or timeouts
quarantine a plugin until reset in Preferences.

Capability metadata describes behavior and does not restrict access. SYSTEM
plugins require confirmation and never preview. Validate input, quote paths,
close resources and keep settings outside the package drawer so upgrades retain
them. The package validator checks manifest versions and drawer-relative paths.

## Independent bundled plugin versions

Beam68's application version is separate from each plugin's PLUGIN_VERSION.
All bundled plugins start at 0.1 under this policy. Manifest VERSION remains
Protocol 1; MIN_BEAM68 expresses compatibility and is not a plugin version.

When a plugin's code, configuration, manifest, documentation or compiled shared
dependency changes, increase its PLUGIN_VERSION (0.1 to 0.2, then 0.3, etc.).
Increase every affected plugin when shared code changes. Main application-only
changes do not increase plugin versions. Keep package IDs stable for upgrades.

After reviewing changes and updating manifest versions, run:

```sh
python3 scripts/plugin-versions.py --record
python3 scripts/plugin-versions.py --check
```

The checked-in Plugins/versions.json binds versions to their inputs. Recording
refuses changed inputs with an unchanged or lower version. It also regenerates
each plugin's version.c, which embeds its own AmigaDOS $VER tag. Both native
build paths compile these tags, and the build wrapper checks the version record.
Commit manifests, generated version sources and the record with the plugin change.
Do not reset the record to bypass a missing version bump. Preferences displays
manifest versions in aligned Plugin, Version, State and Key columns and Details.

## Preferences plugin controls

Priority 1 is matched first; Priority up/down reorders the list and its numbers.
Toggle enables or disables a plugin. The selected row keeps its highlight.
Import opens the package selector and imports an archive, drawer or valid manifest.
Remove is available only for non-bundled plugins. Bundled IDs and installation
paths stay protected; disable bundled plugins with Toggle instead. Use or Save
applies the changes. The old Reload plugins and Export report buttons are absent.
Details is one wrapped document, with scrolling if the screen is too small to
show it all. Typing preview controls automatic requests after a typing pause.

## Integration ownership

WHDLoad, DOpus and Music Library own their application-specific settings.
Normal results retain generic indexing and delegate integration actions to the
enabled bundled plugin. ARexx remote control of Beam68 remains part of the host.
ARexx script indexing and bookmark indexing remain compatibility sources;
they are candidates for future dedicated plugins. Plugin import is on Plugins.

## Activation and independent versions

Beam68 0.2 enables six essential local plugins on a fresh installation.
Network and optional integrations start disabled; updates preserve saved choices.
Plugin versions are independent of the launcher version. Start a new plugin at
0.1 and increment its version when changing its implementation. The bundled
version-check script verifies compiled inputs. Import third-party packages in
Preferences; bundled plugins can be toggled but cannot be removed.
