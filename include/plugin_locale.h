#ifndef BEAM68_PLUGIN_LOCALE_H
#define BEAM68_PLUGIN_LOCALE_H
#include "beam68plugins_strings.h"
/* Replies of the bundled plugins from Beam68Plugins.catalog (installed in
 * LOCALE:Catalogs), built-in English otherwise.  Uses the SDK's catalog
 * functions, exactly as a third-party plugin would. */
const char *plugin_str(int id);
#endif
