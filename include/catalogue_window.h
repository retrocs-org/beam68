#ifndef BEAM68_CATALOGUE_WINDOW_H
#define BEAM68_CATALOGUE_WINDOW_H
#include <intuition/intuition.h>
#include "beam.h"
/* Beam68Prefs: browse a plugin/theme catalogue and install entries.
 * Downloads use the user's own tool (DOWNLOAD_COMMAND, wget by default);
 * plugins go through the normal package installer, themes through
 * `import_theme`.  Returns 1 when something was installed. */
int beam_catalogue_window(struct Screen *screen, APTR visual, struct BeamConfig *c,
                          int (*import_theme)(const char *path),
                          char *msg, unsigned int cap);
#endif
