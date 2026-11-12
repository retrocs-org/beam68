#ifndef BEAM68_PRESET_H
#define BEAM68_PRESET_H
/* Preferences presets: one text file holding a complete Beam68 setup
 * (Beam68.cfg, favorites, aliases, hidden results, plugin settings).
 *
 *   BEAM68PRESET 1
 *   =FILE Beam68.cfg
 *   |HOTKEY=rawkey control alt space
 *   ...
 *   =END
 *
 * Every content line starts with '|', so no content can be mistaken for a
 * header.  File names are relative to ENVARC:Beam68/ and limited to
 * letters, digits, '.', '_', '-' and one "Plugins/" level. */
#define BEAM_PRESET_MAX_FILES 32
int beam_preset_name_ok(const char *name);
/* names: relative names; dir: e.g. "ENVARC:Beam68".  Missing files are
 * skipped.  Returns the number of files written, or -1 on error. */
int beam_preset_export(const char *out_path, const char *dir,
                       const char *const *names, int count);
/* Writes every file of the preset into each of the `dirs` (e.g. ENVARC:
 * and ENV: copies).  Returns files imported, or -1 for an invalid preset
 * (nothing is written unless the whole preset parses). */
int beam_preset_import(const char *in_path, const char *const *dirs, int dir_count);
#endif
