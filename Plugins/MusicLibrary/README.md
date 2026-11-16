# Music Library 0.7

Music Library searches module files without adding them to Beam68's application index.
Use `mod:<name>` (or `;<name>`) and press Enter. Up to eight matches are returned;
select a match and press Enter to choose it, then Enter again to request playback.
Searches skip the extra SYSTEM confirmation; playback still asks for confirmation.

In Preferences → Plugins → Music Library → Settings, set the music drawer and
player command. Leave the drawer empty to search the main configured drawers.
The default player command is `Run >NIL: HippoPlayer "{file}"`.
On existing installations the old player command is used until overridden here.
Paths too long for a play query open with their Workbench default tool instead.

Search follows real drawers only, respects the configured scan depth, stops at
eight results and reports unreadable drawers. Searches run on Enter and are subject
to Beam68's plugin timeout. Music Controls remains a separate playback-control plugin.

Protocol 1. Requires Beam68 0.2 and AmigaOS 3.0+. MIT licence.
