#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "wb_drop.h"

int main(void)
{
    char out[BEAM_MAX_PATH],longpath[BEAM_MAX_PATH+40];
    assert(beam_drop_candidate("Work:Games",1,BEAM_DROP_DIRECTORY,out,sizeof(out)));
    assert(!strcmp(out,"Work:Games"));
    assert(!beam_drop_candidate("Work:Games/Doom",0,BEAM_DROP_DIRECTORY,out,sizeof(out)));
    assert(beam_drop_candidate("Work:Beam68/Plugins/Clock.PLUGIN",0,
                               BEAM_DROP_PLUGIN,out,sizeof(out)));
    assert(!beam_drop_candidate("Work:Beam68/Plugins/Clock",0,
                                BEAM_DROP_PLUGIN,out,sizeof(out)));
    /* Package drawers and archives are accepted for installation. */
    assert(beam_drop_candidate("RAM:Weather",1,BEAM_DROP_PLUGIN,out,sizeof(out)));
    assert(beam_drop_candidate("RAM:Beam68-Plugin-Weather.LHA",0,
                               BEAM_DROP_PLUGIN,out,sizeof(out)));
    assert(beam_drop_candidate("RAM:Weather.zip",0,BEAM_DROP_PLUGIN,out,sizeof(out)));
    assert(!beam_drop_candidate("RAM:Weather.txt",0,BEAM_DROP_PLUGIN,out,sizeof(out)));
    assert(beam_drop_candidate("SYS:Tools/Ed",0,BEAM_DROP_FAVORITE,
                               out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Tools",1,BEAM_DROP_FAVORITE,
                                out,sizeof(out)));
    assert(beam_drop_candidate("Work:Themes/Mocha.theme",0,BEAM_DROP_THEME,
                               out,sizeof(out)));
    assert(!beam_drop_candidate("Work:Themes/Mocha.txt",0,BEAM_DROP_THEME,
                                out,sizeof(out)));
    assert(!beam_drop_candidate("relative/file",0,BEAM_DROP_FAVORITE,
                                out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Bad\nName",0,BEAM_DROP_FAVORITE,
                                out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Bad\"Name",0,BEAM_DROP_FAVORITE,
                                out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Bad*Name",0,BEAM_DROP_FAVORITE,
                                out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Tools/Ed",0,BEAM_DROP_FAVORITE,out,3));
    assert(!out[0]);
    memset(longpath,'x',sizeof(longpath));
    memcpy(longpath,"SYS:",4);
    longpath[sizeof(longpath)-1]=0;
    assert(!beam_drop_candidate(longpath,1,BEAM_DROP_DIRECTORY,out,sizeof(out)));
    assert(!beam_drop_candidate("SYS:Tools",1,100,out,sizeof(out)));
    puts("Workbench drop filters, bounds and paths: PASS");
    return 0;
}
