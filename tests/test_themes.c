/* The theme gallery files load with the launcher's own theme reader. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "theme_file.h"
int main(void)
{
    static const char *names[]={"Amber","GreenScreen","Paper","MagicGrey",NULL};
    unsigned long colors[BEAM_THEME_COLOR_COUNT];
    char path[96];
    int i;
    if (system("mkdir -p build/testsettings/PROGDIR/themes && cp themes/*.theme build/testsettings/PROGDIR/themes/")!=0) return 1;
    for (i=0;names[i];i++) {
        sprintf(path,"PROGDIR:themes/%s.theme",names[i]);
        assert(beam_theme_file_load(path,colors));
    }
    puts("Theme gallery: all bundled themes load: PASS");
    return 0;
}
