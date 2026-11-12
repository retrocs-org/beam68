/* HelloWorld - minimal Beam68 plugin.
 *
 * Query "hello:Amiga" (or "+Amiga" with the trigger) and press Enter:
 * the launcher shows "Hello, Amiga!".  An empty query shows usage. */
#include <stdio.h>
#include "beam68plugin.h"

int main(int argc, char **argv)
{
    /* static: plugins may run on a 4 KB Shell stack */
    static char input[BEAM68_PLUGIN_MAX_INPUT+1];
    static char reply[BEAM68_PLUGIN_MAX_RESULT+1];
    char *name;
    if (!beam68_plugin_input(argc,argv,input,sizeof(input))) return 20;
    name=beam68_trim(input);
    if (!*name) return beam68_plugin_error(argc,argv,"Type a name, e.g. hello:Amiga");
    sprintf(reply,"Hello, %.100s!",name);
    return beam68_plugin_text(argc,argv,reply);
}
