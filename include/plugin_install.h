#ifndef BEAM68_PLUGIN_INSTALL_H
#define BEAM68_PLUGIN_INSTALL_H
#include <intuition/intuition.h>
#include "beam.h"
/* Native plugin package installation for Beam68Prefs (AmigaOS only).
 *
 * Archives are unpacked with the user's own tools: C:LhA for .lha/.lzh and
 * C:UnZip for .zip (both on Aminet).  Without the tool, unpack the archive
 * yourself and install the resulting drawer.
 *
 * Install flow: list and screen the archive, extract to T:, locate the single
 * plugin drawer, validate manifest and executable, ask for consent (showing
 * declared capabilities and upgrade/downgrade), copy to <INSTALL_DIR>/Plugins/
 * <Name>.new, then swap it into place.  The configuration entry is added or
 * updated in `c`; the caller applies it with Use/Save. */

/* 1 when path is an archive or a drawer (i.e. should be installed rather
 * than added as a bare manifest). */
int beam_plugin_install_candidate(const char *path);
/* Returns 1 on success (msg says what happened), 0 on failure or cancel. */
int beam_plugin_install(struct Window *win, struct BeamConfig *c,
                        const char *source, char *msg, unsigned int cap);
/* For a PROGDIR:Plugins/<Drawer>/<file> spec, the absolute package drawer.
 * Returns 0 for plugins that are not installed packages. */
int beam_plugin_package_dir(const struct BeamConfig *c, const char *spec,
                            char *out, unsigned int cap);
/* Delete an installed package drawer (and its .info).  Returns 1 if gone. */
int beam_plugin_delete_package(const char *package_dir);
/* Find PROGDIR:Plugins/<Drawer>/<Name>.plugin manifests on disk. */
int beam_plugin_scan_installed(const struct BeamConfig *c,
                               char (*specs)[BEAM_MAX_PATH], int max);
#endif
