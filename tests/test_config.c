#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "beam.h"

static void write_text(const char *file,const char *s)
{
    FILE *f=fopen(file,"wb");
    assert(f!=NULL); assert(fputs(s,f)>=0); assert(fclose(f)==0);
}
static int file_has(const char *file,const char *needle)
{
    char buffer[8192]; size_t n;
    FILE *f=fopen(file,"rb");
    if (!f) return 0;
    n=fread(buffer,1,sizeof(buffer)-1,f); buffer[n]=0;
    fclose(f);
    return strstr(buffer,needle)!=NULL;
}
int main(void)
{
    static struct BeamConfig c,d;
    if (system("rm -rf build/testsettings; mkdir -p build/testsettings/T/Beam68 build/testsettings/ENV/Beam68 build/testsettings/ENVARC/Beam68 build/testsettings/PROGDIR")!=0) { fputs("test setup command failed\n",stderr); return 1; }
    write_text("build/testsettings/PROGDIR/Beam68.cfg",
        "# sample starter\nHOTKEY=rawkey control alt space\nDIR=SYS:Tools\nPLUGIN=PROGDIR:Plugins/Calc.plugin\n");
    beam_config_defaults(&c);
    assert(beam_config_read_active(&c));
    assert(c.dir_count==1);
    assert(beam_equal(c.dirs[0],"SYS:Tools"));
    assert(beam_config_valid(&c));
    assert(c.history==1 && c.index_cache==1 && c.plugin_timeout==30 && c.icon_preview==0 && c.window_width==550 && c.window_height==0 && c.theme==BEAM_THEME_WORKBENCH && c.shell_commands==1);
    write_text("build/testsettings/ENVARC/Beam68/Beam68.cfg",
        "# USER COMMENT\nTHIRD_PARTY=hello world\nMAX_ITEMS=128\nDIR=Games:\n");
    beam_config_defaults(&c);
    assert(beam_config_read_active(&c));
    assert(c.max_items==128);
    assert(c.dir_count==1 && beam_equal(c.dirs[0],"Games:"));
    beam_copy(c.hotkey,"rawkey control alt f10",sizeof(c.hotkey));
    c.max_items=512;
    c.history=0;
    c.index_cache=0;
    c.plugin_timeout=45;
    c.icon_preview=1;
    c.theme=BEAM_THEME_CUSTOM;
    c.custom_colors[0]=0x112233UL;
    c.custom_colors[1]=0x445566UL;
    c.custom_colors[2]=0x778899UL;
    c.custom_colors[3]=0xaabbccUL;
    c.custom_colors[4]=0xddeeffUL;
    c.shell_commands=0;
    c.index_games=0; strcpy(c.whdload_command,"C:WHDLoad");
    c.window_width=640; c.window_height=180;
    assert(beam_config_write(&c,BEAM_CONFIG_ENV));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","# USER COMMENT"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","THIRD_PARTY=hello world"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","MAX_ITEMS=512"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","HISTORY=NO"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","INDEX_CACHE=NO"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","PLUGIN_TIMEOUT=45"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","ICON_PREVIEW=YES"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","THEME=CUSTOM"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","CUSTOM_COLORS=112233,445566,778899,AABBCC,DDEEFF"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","SHELL_COMMANDS=NO"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","WINDOW_WIDTH=640"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","WINDOW_HEIGHT=180"));
    beam_config_defaults(&d);
    assert(beam_config_read_active(&d));
    assert(d.max_items==512 && beam_equal(d.hotkey,c.hotkey) && d.history==0 && d.index_cache==0 && d.plugin_timeout==45 && d.icon_preview==1 && d.window_width==640 && d.window_height==180 && d.theme==BEAM_THEME_CUSTOM && d.shell_commands==0);
    assert(d.index_games==0 && !strcmp(d.whdload_command,"C:WHDLoad"));
    assert(d.custom_colors[0]==0x112233UL && d.custom_colors[4]==0xddeeffUL);
    assert(d.dir_count==1 && beam_equal(d.dirs[0],"Games:"));
    assert(beam_config_write(&d,BEAM_CONFIG_ENV));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","THIRD_PARTY=hello world"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","# USER COMMENT"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg.bak","MAX_ITEMS=512"));
    assert(beam_config_write(&d,BEAM_CONFIG_ENVARC));
    assert(file_has("build/testsettings/ENVARC/Beam68/Beam68.cfg","HOTKEY=rawkey control alt f10"));
    d.max_items=514;
    assert(beam_config_write(&d,BEAM_CONFIG_SESSION));
    beam_config_defaults(&c);
    assert(beam_config_read_active(&c));
    assert(c.max_items==514);
    remove("build/testsettings/T/Beam68/Beam68.cfg");
    d.max_items=512;
    /* A stubborn host-filesystem .bak must not make later saves fail. */
    assert(!remove("build/testsettings/ENV/Beam68/Beam68.cfg.bak"));
    assert(!mkdir("build/testsettings/ENV/Beam68/Beam68.cfg.bak",0777));
    write_text("build/testsettings/ENV/Beam68/Beam68.cfg.bak/keep","unrelated data\n");
    d.max_items=513;
    assert(beam_config_write(&d,BEAM_CONFIG_ENV));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg","MAX_ITEMS=513"));
    assert(file_has("build/testsettings/ENV/Beam68/Beam68.cfg.b01","MAX_ITEMS=512"));
    d.max_items=512;
    d.icon_preview=2;
    assert(!beam_config_valid(&d));
    d.icon_preview=1;
    d.window_height=75;
    assert(!beam_config_valid(&d));
    d.window_height=0;
    assert(beam_config_valid(&d));
    d.plugin_timeout=121;
    assert(!beam_config_valid(&d));
    d.plugin_timeout=5;
    assert(beam_config_valid(&d));
    d.theme=BEAM_THEME_COUNT;
    assert(!beam_config_valid(&d));
    d.theme=BEAM_THEME_LATTE;
    d.shell_commands=2;
    assert(!beam_config_valid(&d));
    d.shell_commands=1;
    assert(beam_config_valid(&d));
    d.custom_colors[0]=0x1000000UL;
    assert(!beam_config_valid(&d));
    d.custom_colors[0]=0x112233UL;
    write_text("build/testsettings/ENVARC/Beam68/Beam68.cfg",
        "PLUGIN_TIMEOUT=999\nDIR=SYS:Tools\n");
    { static struct BeamConfig e;
      beam_config_defaults(&e);
      assert(beam_config_read(&e,"ENVARC:Beam68/Beam68.cfg"));
      assert(e.plugin_timeout==30);
    }
    /* If both active files are corrupt, a validated atomic-save backup is
       recovered before the shipped template. */
    write_text("build/testsettings/ENV/Beam68/Beam68.cfg","HOTKEY=\n");
    write_text("build/testsettings/ENVARC/Beam68/Beam68.cfg","HOTKEY=\n");
    write_text("build/testsettings/ENVARC/Beam68/Beam68.cfg.bak",
               "HOTKEY=rawkey control alt f9\nMAX_ITEMS=321\nDIR=SYS:Tools\n");
    { static struct BeamConfig recovered;
      beam_config_defaults(&recovered);
      assert(beam_config_read_active(&recovered));
      assert(recovered.max_items==321);
      assert(beam_equal(recovered.hotkey,"rawkey control alt f9"));
    }
    d.max_items=0;
    assert(!beam_config_valid(&d));
    assert(!beam_config_write(&d,BEAM_CONFIG_ENV));
    puts("Config read/write, ENV/ENVARC fallback, backup, unknown keys: PASS");
    return 0;
}
