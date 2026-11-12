#include <stdlib.h>
#include "beam68plugin.h"
#include "plugin_locale.h"

static int opened;

const char *plugin_str(int id)
{
    const char *builtin=(id>=1 && id<=BEAM68PLUGINS_STRING_COUNT)?beam68plugins_strings[id-1]:"";
    if (!opened) {
        opened=1;
        beam68_catalog_open("Beam68Plugins.catalog");
        atexit(beam68_catalog_close);
    }
    return beam68_str(id,builtin);
}
