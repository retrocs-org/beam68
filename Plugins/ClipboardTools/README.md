# Clipboard Tools

Clipboard Tools is optional and disabled by default. Ordinary commands include
`clip:show`, `clip:set:text`, `clip:clear`, `clip:upper`, `clip:lower` and
`clip:save:path`. Clipboard history additionally requires Beam68 0.5.

## Optional session history

Recording is off by default and never starts automatically after installation,
a restart or enabling the plugin. Type `clip:history on` and press Enter to
start recording subsequent clipboard changes. The current clipboard is used as
a baseline and is not captured immediately. Sampling is roughly once a second;
rapid intermediate copies may be missed. Text longer than 120 bytes is skipped
rather than stored as a truncated copy. Non-text clipboard content is ignored.

- `clip:history` lists captured entries, newest first.
- `clip:history words` searches captured text without writing search terms into
  permanent query history.
- Selecting an entry loads `clip:restore <id>`; Enter explicitly restores its
  original text, including line breaks. No program is executed.
- `clip:history clear` clears captured entries while recording continues.
- `clip:history off` stops the recorder and clears its temporary data.
- `clip:history status` reports whether recording is running.

Plugin Settings controls entry count (default 8, maximum 16), retained-text
budget (default 2048 bytes, 256–2048) and expiry (default 30 minutes, 1–240).
These limits apply when recording starts; stop and restart after changing them.
The text budget limits retained text, not the recorder's total process memory.
The recorder uses a bounded ring and a private 16 KiB stack when needed.

History is held in a temporary file on RAM: and is never saved to ENVARC:.
Disabling Clipboard Tools or stopping Beam68 stops the recorder. Uninstall stops
Beam68 before deleting files. Stop/clear/expiry prevent old result IDs from
restoring replacement entries. Existing clipboard contents and files written
with `clip:save:` are not deleted by stopping history.
