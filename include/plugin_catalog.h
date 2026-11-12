#ifndef BEAM68_PLUGIN_CATALOG_H
#define BEAM68_PLUGIN_CATALOG_H
#include "beam.h"
#include "plugin_diagnostics.h"
/* Resolve a manifest or command name from a *separate* Preferences process.
 * For PROGDIR: references use INSTALL_DIR rather than Prefs' own PROGDIR:.
 * Caller provides a buffer at least BEAM_MAX_PATH bytes long. */
int beam_catalog_resolve(const struct BeamConfig *c, const char *path,
                         char *out, unsigned int cap);
/* Call in manifest order with a zero-initialized 128-byte claimed[] array.
 * Existing valid, enabled triggers win. Includes disabled plugin metadata
 * for display but never claims its trigger. Does not execute plugins. */
int beam_catalog_inspect(const struct BeamConfig *c, int index,
                         unsigned char claimed[128], struct BeamPlugin *plugin);
/* Merge shipped manifests that exist on disk but are absent from an older
 * user configuration. Newly discovered plugins start disabled, so opening
 * Preferences never silently grants new executable capabilities. */
int beam_catalog_discover_bundled(struct BeamConfig *c);
/* Merge manifests found on disk (PROGDIR:Plugins/<Drawer>/<Name>.plugin
 * specs from a native drawer scan) that the configuration does not list
 * yet.  They are added disabled.  Returns the number added. */
int beam_catalog_merge_discovered(struct BeamConfig *c,
                                  const char (*specs)[BEAM_MAX_PATH], int count);
#endif
