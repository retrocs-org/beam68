## Features

- Add local inline completion suggestions, accepted explicitly with Ctrl+Right without executing a command or contacting a network service.
- Add optional session clipboard history to Clipboard Tools, with bounded entries/text, expiry, search, explicit restore, clear and stop controls; recording is off by default.
- Add a visible result action menu opened with Tab, right-click or the result footer, with keyboard and mouse selection.
- Add All, Apps, Files, Drawers and Plugins category buttons using existing search scopes.
- Add optional two-line result rows showing names above locations or descriptions, with automatic compact-screen fallback.

## Enhancements

- Show plugin dependency readiness in Preferences and Details, with local checks and an explicit read-only AmiHASS connection test.
- Show Aminet download speed and estimated remaining time, and meaningful byte/activity feedback for unknown-size transfers.
- Show the selected result’s Enter action and an Actions hint in the footer.
- Keep action choices appropriate to item type and retain the selected item when cancelling the menu.
- Remove staged installer QA drawers and icon metadata after testing.

## Bugs

- Handle GitLab's same-origin latest-release redirect during publication verification without forwarding credentials to another origin or replaying writes.
- Use the current build date for files inside LHA packages and SDK ZIPs instead of 1 January 1980.
