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
