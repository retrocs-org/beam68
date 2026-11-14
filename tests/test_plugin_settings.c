/* End-to-end tests for editable, order-independent plugin enablement. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam.h"

static void put_file(const char *name,const char *content)
{
    FILE *f=fopen(name,"wb");
    assert(f);
    assert(fputs(content,f)>=0);
    assert(fclose(f)==0);
}
static int has_file_text(const char *name,const char *needle)
{
    FILE *f=fopen(name,"rb");
    char buffer[12000];
    size_t n;
    assert(f);
    n=fread(buffer,1,sizeof(buffer)-1,f);
    buffer[n]=0;
    fclose(f);
    return strstr(buffer,needle)!=NULL;
}
static void test_preview_controls(void)
{
    static struct BeamConfig c,d;
    const char *clock="PROGDIR:Plugins/Clock/Clock.plugin";
    const char *calc="PROGDIR:Plugins/Calc/Calc.plugin";
    const char *file="build/testsettings/PROGDIR/preview.cfg";
    int parsed=750;
    assert(beam_plugin_preview_parse_delay("100",&parsed) && parsed==100);
    assert(beam_plugin_preview_parse_delay("5000",&parsed) && parsed==5000);
    assert(beam_plugin_preview_parse_delay("  750\t",&parsed) && parsed==750);
    assert(beam_plugin_preview_parse_delay("0123",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("99",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("5001",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("-100",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("1.5",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("750ms",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("999999999999999999999",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("750\n",&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay(NULL,&parsed) && parsed==123);
    assert(!beam_plugin_preview_parse_delay("750",NULL));
    put_file(file,
        "# Keep preview notes\nPLUGIN_PREVIEW_DISABLED=PROGDIR:Plugins/Clock.plugin\n"
        "PLUGIN_PREVIEW_DELAY=750|progdir:plugins/CLOCK.plugin\n"
        "PLUGIN_PREVIEW_DELAY=1200|PROGDIR:Plugins/Calc/Calc.plugin\n"
        "PLUGIN_PREVIEW_DELAY=0|PROGDIR:Plugins/Calc/Calc.plugin\n"
        "PLUGIN_PREVIEW_DELAY=300xyz|PROGDIR:Plugins/Calc/Calc.plugin\n"
        "PLUGIN_PREVIEW_DELAY=900|Work:NotInstalled.plugin\n"
        "PLUGIN=PROGDIR:Plugins/Calc.plugin\nPLUGIN=PROGDIR:Plugins/Clock.plugin\n");
    beam_config_defaults(&c);assert(beam_config_read(&c,"PROGDIR:preview.cfg"));
    assert(c.plugin_file_count==2 && beam_config_valid(&c));
    assert(beam_plugin_preview_delay(&c,clock,1)==0);
    assert(beam_plugin_preview_delay(&c,calc,1)==1200);
    assert(beam_plugin_preview_delay(&c,calc,0)==0); /* manifest must opt in */
    assert(!beam_plugin_preview_set(&c,clock,1,99));
    assert(!beam_plugin_preview_set(&c,clock,1,5001));
    assert(!beam_plugin_preview_set(&c,"Work:Unknown.plugin",1,400));
    assert(beam_plugin_preview_set(&c,clock,1,750));
    assert(beam_plugin_preview_delay(&c,clock,1)==750);
    assert(beam_plugin_preview_set(&c,clock,0,750));
    assert(beam_config_write(&c,"PROGDIR:preview.cfg"));
    assert(has_file_text(file,"# Keep preview notes"));
    assert(has_file_text(file,"PLUGIN_PREVIEW_DELAY=750|PROGDIR:Plugins/Clock/Clock.plugin"));
    beam_config_defaults(&d);assert(beam_config_read(&d,"PROGDIR:preview.cfg"));
    assert(beam_plugin_preview_delay(&d,clock,1)==0 && d.plugin_preview_delay[1]==750);
    assert(beam_plugin_preview_set(&d,clock,1,400));
    assert(beam_plugin_preview_delay(&d,clock,1)==400);
    /* File order changes cannot transfer the override to another plugin. */
    put_file(file,"PLUGIN_PREVIEW_DELAY=1500|PROGDIR:Plugins/Calc.plugin\n"
                  "PLUGIN=PROGDIR:Plugins/Clock.plugin\nPLUGIN=PROGDIR:Plugins/Calc.plugin\n");
    beam_config_defaults(&d);assert(beam_config_read(&d,"PROGDIR:preview.cfg"));
    assert(beam_plugin_preview_delay(&d,clock,1)==400);
    assert(beam_plugin_preview_delay(&d,calc,1)==1500);
    d.plugin_file_count=1;
    assert(beam_config_write(&d,"PROGDIR:preview.cfg"));
    assert(!has_file_text(file,"PLUGIN_PREVIEW_DELAY="));
    d.plugin_preview_delay[0]=99;assert(!beam_config_valid(&d));
    puts("Per-plugin previews: opt-in, disable, delay, ordering, migration and persistence: PASS");
}

int main(void)
{
    static struct BeamConfig c,d;
    const char *calc="PROGDIR:Plugins/Calc/Calc.plugin";
    const char *clock="PROGDIR:Plugins/Clock/Clock.plugin";
    const char *memory="PROGDIR:Plugins/Memory/Memory.plugin";
    int i;
    assert(system("rm -rf build/testsettings; mkdir -p build/testsettings/ENV/Beam68 build/testsettings/ENVARC/Beam68 build/testsettings/PROGDIR") == 0);
    /* Manually written 0.1-format file (flat PROGDIR:Plugins/<Name>.plugin):
       entries are migrated to the package-drawer layout on read, and
       disabled entries can precede the manifest list. */
    put_file("build/testsettings/ENVARC/Beam68/Beam68.cfg",
        "# MY PLUGIN NOTES\nCUSTOM_PLUGIN_SETTING=ok\n"
        "PLUGIN_DISABLED=PROGDIR:Plugins/Clock.plugin\n"
        "PLUGIN_DISABLED=progdir:plugins/CLOCK.plugin\n"
        "PLUGIN=PROGDIR:Plugins/Calc.plugin\n"
        "PLUGIN=PROGDIR:Plugins/Clock.plugin\n"
        "PLUGIN=PROGDIR:Plugins/Memory.plugin\n");
    beam_config_defaults(&c);
    assert(beam_config_read_active(&c));
    assert(c.plugin_file_count==3);
    assert(c.disabled_plugin_count==1); /* no duplicate entries */
    assert(beam_plugin_enabled(&c,calc));
    assert(!beam_plugin_enabled(&c,clock));
    assert(beam_plugin_enabled(&c,memory));
    assert(!beam_plugin_set_enabled(&c,"Work:Unknown.plugin",0));
    assert(beam_plugin_set_enabled(&c,memory,0));
    assert(c.disabled_plugin_count==2);
    assert(beam_plugin_set_enabled(&c,memory,0));
    assert(c.disabled_plugin_count==2);
    assert(beam_config_valid(&c));
    assert(beam_config_write(&c,BEAM_CONFIG_ENV));
    assert(has_file_text("build/testsettings/ENV/Beam68/Beam68.cfg","PLUGIN_DISABLED=PROGDIR:Plugins/Clock/Clock.plugin"));
    assert(has_file_text("build/testsettings/ENV/Beam68/Beam68.cfg","PLUGIN_DISABLED=PROGDIR:Plugins/Memory/Memory.plugin"));
    assert(has_file_text("build/testsettings/ENV/Beam68/Beam68.cfg","CUSTOM_PLUGIN_SETTING=ok"));
    assert(has_file_text("build/testsettings/ENV/Beam68/Beam68.cfg","# MY PLUGIN NOTES"));
    beam_config_defaults(&d);
    assert(beam_config_read_active(&d));
    assert(!beam_plugin_enabled(&d,clock) && !beam_plugin_enabled(&d,memory));
    assert(beam_plugin_set_enabled(&d,clock,1));
    assert(beam_plugin_enabled(&d,clock));
    assert(d.disabled_plugin_count==1);
    assert(beam_config_write(&d,BEAM_CONFIG_ENVARC));
    assert(!has_file_text("build/testsettings/ENVARC/Beam68/Beam68.cfg","PLUGIN_DISABLED=PROGDIR:Plugins/Clock/Clock.plugin"));
    assert(has_file_text("build/testsettings/ENVARC/Beam68/Beam68.cfg","PLUGIN_DISABLED=PROGDIR:Plugins/Memory/Memory.plugin"));
    beam_plugin_forget_disabled(&d,memory);
    assert(d.disabled_plugin_count==0);
    assert(beam_plugin_enabled(&d,memory));
    assert(beam_plugin_set_enabled(&d,memory,0));
    assert(d.disabled_plugin_count==1);
    /* Drop manifest while a stale disabled entry remains; saving removes it. */
    for (i=2;i<d.plugin_file_count-1;i++)
        beam_copy(d.plugin_files[i],d.plugin_files[i+1],BEAM_MAX_PATH);
    d.plugin_file_count=2;
    assert(beam_config_write(&d,BEAM_CONFIG_ENV));
    assert(!has_file_text("build/testsettings/ENV/Beam68/Beam68.cfg","PLUGIN_DISABLED=PROGDIR:Plugins/Memory/Memory.plugin"));
    test_preview_controls();
    puts("Plugin enable/disable, manual order, dedup, persistence, stale cleanup: PASS");
    return 0;
}
