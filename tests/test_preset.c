#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preset.h"
static void put(const char *p,const char *t){FILE *f=fopen(p,"wb");assert(f);fputs(t,f);fclose(f);}
static int has(const char *p,const char *t){static char b[4096];FILE *f=fopen(p,"rb");size_t n;if(!f)return 0;n=fread(b,1,sizeof(b)-1,f);b[n]=0;fclose(f);return strstr(b,t)!=NULL;}
void beam_test_fail_writes(int after);
void beam_test_fail_rename(int after);
int main(void)
{
    static const char *names[]={"Beam68.cfg","Favorites.txt","Missing.txt","Plugins/Clock.cfg","../Evil"};
    static const char *dirs[]={"RAM:PresetOut"};
    if (system("rm -rf build/testsettings/RAM/PresetIn build/testsettings/RAM/PresetOut && "
               "mkdir -p build/testsettings/RAM/PresetIn/Plugins build/testsettings/RAM/PresetOut")!=0) return 1;
    put("build/testsettings/RAM/PresetIn/Beam68.cfg","HOTKEY=rawkey f10\n=FILE trick\n|x\n");
    put("build/testsettings/RAM/PresetIn/Favorites.txt","games\tWork:Doom\n");
    put("build/testsettings/RAM/PresetIn/Plugins/Clock.cfg","SHOW_SECONDS=YES");
    assert(beam_preset_export("RAM:my.preset","RAM:PresetIn",names,5)==3);
    assert(beam_preset_import("RAM:my.preset",dirs,1)==3);
    assert(has("build/testsettings/RAM/PresetOut/Beam68.cfg","=FILE trick\n|x\n"));
    assert(has("build/testsettings/RAM/PresetOut/Favorites.txt","Work:Doom"));
    assert(has("build/testsettings/RAM/PresetOut/Plugins/Clock.cfg","SHOW_SECONDS=YES\n"));
    assert(beam_preset_name_ok("Plugins/A.cfg") && !beam_preset_name_ok("Plugins/../x") &&
           !beam_preset_name_ok("/x") && !beam_preset_name_ok("a/b") && !beam_preset_name_ok("S:x"));
    put("build/testsettings/RAM/bad.preset","BEAM68PRESET 1\n=FILE ../../S/Startup-Sequence\n|x\n=END\n");
    assert(beam_preset_import("RAM:bad.preset",dirs,1)==-1);
    put("build/testsettings/RAM/bad2.preset","BEAM68PRESET 1\n=FILE ok.txt\n|x\n");
    assert(beam_preset_import("RAM:bad2.preset",dirs,1)==-1);
    assert(!has("build/testsettings/RAM/PresetOut/ok.txt","x")); /* nothing written */
    put("build/testsettings/RAM/header.preset","BEAM68PRESET 10\n=FILE ok.txt\n|x\n=END\n");
    assert(beam_preset_import("RAM:header.preset",dirs,1)==-1);
    put("build/testsettings/RAM/duplicate.preset","BEAM68PRESET 1\n=FILE Beam68.cfg\n|first\n=END\n=FILE beam68.cfg\n|second\n=END\n");
    assert(beam_preset_import("RAM:duplicate.preset",dirs,1)==-1);
    assert(beam_preset_import("RAM:my.preset",dirs,0)==-1);
    put("build/testsettings/RAM/PresetOut/Beam68.cfg","ORIGINAL\n");
    beam_test_fail_writes(1);
    assert(beam_preset_import("RAM:my.preset",dirs,1)==-1);
    assert(has("build/testsettings/RAM/PresetOut/Beam68.cfg","ORIGINAL\n"));
    beam_test_fail_rename(1);
    assert(beam_preset_import("RAM:my.preset",dirs,1)==-1);
    assert(has("build/testsettings/RAM/PresetOut/Beam68.cfg","ORIGINAL\n"));
    puts("Preferences presets: export, import, escaping, unsafe names, all-or-nothing: PASS");
    return 0;
}
