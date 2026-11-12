#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "theme_file.h"
int main(void)
{
    unsigned long a[BEAM_THEME_COLOR_COUNT]={0x112233UL,0x445566UL,0x778899UL,0xaabbccUL,0xddeeffUL};
    unsigned long b[BEAM_THEME_COLOR_COUNT]={0,0,0,0,0};
    FILE *f;
    if (system("mkdir -p build/testsettings/ENVARC/Beam68")!=0) { fputs("test setup command failed\n",stderr); return 1; }
    assert(beam_theme_file_save("ENVARC:Beam68/Test.theme",a));
    assert(beam_theme_file_load("ENVARC:Beam68/Test.theme",b));
    assert(b[0]==a[0] && b[4]==a[4]);
    f=fopen("build/testsettings/ENVARC/Beam68/Bad.theme","wb");assert(f);
    assert(fputs("BEAM68THEME=1\nCOLORS=not-a-palette\n",f)>=0);fclose(f);
    b[0]=0xabcdefUL;
    assert(!beam_theme_file_load("ENVARC:Beam68/Bad.theme",b));
    assert(b[0]==0xabcdefUL);
    puts("Portable theme import/export and invalid-file rollback: PASS");
    return 0;
}
