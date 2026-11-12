# Beam68 0.1 favorites

Favorites are shortcuts to applications already discovered in configured
index directories. They do not add new scan directories or bypass the index.

## Launcher

Highlight a result and press **F6** to pin or unpin it. A pinned result has
an ASCII `*` marker and uses the `General` group. Clear the query to see
pinned applications in pin order, or type `fav:<group>` to show one group.
Press Enter to launch, Tab to choose Open Drawer, or F6 to unpin.
F6 writes to both ENV: and ENVARC: immediately.

## Separate Preferences app

Open SYS:Prefs/Beam68Prefs and select Favorites. Use Browse to pick a file,
Enter a group such as Tools, Games, Internet or Development, then use Add to
pin and Remove to unpin. Use writes temporary ENV: settings and Save
writes both permanent ENVARC: and active ENV: settings. Cancel discards
uncommitted edits. Other Preferences pages do not overwrite favorites unless
you change this list.

## Editable files

`ENV:Beam68/Favorites.txt` is active; `ENVARC:Beam68/Favorites.txt` is the
permanent version. Normal boot copies ENVARC: to ENV:. Each noncomment line
is `Group|application path`. Legacy path-only lines load into `General`.
`#` or `;` starts a full-line comment:

```text
# Beam68 0.1 favorites; Group|full application path
Tools|SYS:Tools/Ed
Games|Work:Games/MyGame
```

Press F3 or run Beam68Ctl RELOAD after manual editing. At most 48 entries
are loaded. Duplicate paths are de-duplicated case-insensitively. Group names
are bounded text and matching is case-insensitive. Paths must
fit in 239 characters; tabs, newlines, `*`, and quotes are rejected.
Saves are staged as `.new`, with a `.bak` of the previous destination.
Full-line comments are preserved. ENV: and ENVARC: are separate saves, so a
write failure between them can leave differing copies.

The separate list allows F6 to persist favorites without accidentally saving
other settings that were temporarily applied with Preferences Use.
