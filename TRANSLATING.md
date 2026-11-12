# Translating Beam68

Beam68 uses standard AmigaOS `locale.library` catalogs. There are three:

| Catalog | Program | Source |
| --- | --- | --- |
| `Beam68.catalog` | the launcher and Beam68Ctl | `catalogs/Beam68.cd` |
| `Beam68Prefs.catalog` | Beam68Prefs and the plugin installer | `catalogs/Beam68Prefs.cd` |
| `Beam68Plugins.catalog` | the bundled plugins' replies | `catalogs/Beam68Plugins.cd` |

The GUI installer's texts live in the `Install` script itself (one block of
`(set #t_...)` lines per language).

## Adding a language

1. Copy an existing translation, e.g. `catalogs/Beam68.deutsch.ct`, to
   `catalogs/Beam68.<language>.ct`, using the Amiga language name
   (`francais`, `italiano`, `nederlands`, `dansk`, `norsk`, `suomi`,
   `polski`, `espanol`...). Do the same for `Beam68Prefs` and
   `Beam68Plugins`.
2. Change `## language` and translate the line after every `MSG_...`
   identifier. The `;` line below it is the English original.
3. Keep every `%s`, `%ld`, `%lu`, `%.40s`... in the same order: the build
   refuses translations that change them (a wrong placeholder would crash
   the launcher).
4. Keep `\n` line breaks, the `TEXT=` / `ERROR=` prefixes in plugin
   replies, and command words such as `find:`, `reboot confirm`, `hex:`
   untranslated: users type those.
5. Mind the space: labels on buttons and result badges (`APP  `, `FILE  `)
   should stay about as long as the English text. Test at 640x256 with
   topaz 8.
6. Run `python3 scripts/make-catalog.py` (or `--check`); it writes
   `catalogs/<language>/*.catalog` and reports missing strings, which fall
   back to English.
7. Add the language to the `(select #lang ...)` list in `Install` and to the
   installer's text blocks, and to `scripts/Install-Beam68`.

Files are UTF-8 in this repository; the catalogs are written in
ISO-8859-1, so only characters that exist in Latin-1 can be used. The
`.cd`/`.ct` files follow the CatComp/FlexCat format, so those tools work too.

Deliberately English: ARexx replies and `Beam68Ctl STATUS` output (scripts
read them).
