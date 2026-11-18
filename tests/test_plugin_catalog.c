/* Host integration tests for native Preferences plugin diagnostics. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam.h"
#include "plugin_catalog.h"
static void write_test(const char *name,const char *text)
{
    FILE *f=fopen(name,"wb");
    assert(f);
    assert(fputs(text,f)>=0);
    assert(fclose(f)==0);
}
static void add(struct BeamConfig *cfg,const char *path)
{
    assert(cfg->plugin_file_count<BEAM_MAX_PLUGINS);
    beam_copy(cfg->plugin_files[cfg->plugin_file_count++],path,BEAM_MAX_PATH);
}
int main(void)
{
    static struct BeamConfig cfg;
    struct BeamPlugin plugin;
    unsigned char claimed[128];
    char path[BEAM_MAX_PATH];
    int state;
    {
        static struct BeamConfig bundled;
        struct BeamPlugin item;
        beam_config_defaults(&bundled);memset(&item,0,sizeof(item));
        assert(beam_catalog_is_bundled(&bundled,0,&item)); /* unreadable bundled path */
        strcpy(bundled.plugin_files[0],"Work:Extra/Other.plugin");
        assert(!beam_catalog_is_bundled(&bundled,0,&item));
        strcpy(item.id,"org.beam68.calc");
        assert(beam_catalog_is_bundled(&bundled,0,&item)); /* moved bundled identity */
        strcpy(item.id,"org.thirdparty.calc");strcpy(item.name,"Calculator");
        assert(!beam_catalog_is_bundled(&bundled,0,&item)); /* names are not identities */
        assert(!beam_catalog_is_bundled(&bundled,-1,&item));
    }
    {
        char first[96],second[96],small[4];
        beam_catalog_format_row(first,sizeof(first),40,"1","Calculator","0.1","ON","=");
        beam_catalog_format_row(second,sizeof(second),40,"12","Assigns and Volumes","0.2","OFF","#");
        assert(!strncmp(first,"1  ",3) && !strncmp(second,"12 ",3));
        assert(strstr(first,"0.1")-first==strstr(second,"0.2")-second);
        assert(strstr(first,"ON")-first==strstr(second,"OFF")-second);
        assert(strlen(first)==strlen(second) && strlen(first)<=40);
        beam_catalog_format_row(small,sizeof(small),40,"1","Calculator","0.1","ON","=");
        assert(strlen(small)==3);
    }
    {
        static const char *names[]={"Calc","Clock","Memory","FileFinder","SystemCommands","DosTools","TaskManager","ClipboardTools","DateUtilities","WindowSwitcher","DeveloperTools","ARexxRunner","MusicControls","AminetBrowser","MusicLibrary","WHDLoad","DOpus","Wikipedia","AmiHASS"};
        unsigned int n;
        for (n=0;n<sizeof(names)/sizeof(names[0]);n++) {
            char cmd[512],spec[160];
            sprintf(cmd,"mkdir -p build/testsettings/MANIFESTS/%s; cp Plugins/%s/%s.plugin build/testsettings/MANIFESTS/%s/",names[n],names[n],names[n],names[n]);
            assert(system(cmd)==0);
            sprintf(spec,"MANIFESTS:%s/%s.plugin",names[n],names[n]);
            if (!beam_plugin_read(&plugin,spec)) fprintf(stderr,"Invalid bundled manifest: %s\n",spec);
            assert(beam_plugin_read(&plugin,spec));
        }
    }
    assert(system("mkdir -p build/testsettings/PROGDIR/Plugins") == 0);
    write_test("build/testsettings/PROGDIR/Plugins/A.plugin",
        "VERSION=1\nNAME=Name A\nTRIGGER=!\nCOMMAND=PROGDIR:Plugins/A\n"
        "DESCRIPTION=Test first plugin\nSETTINGS=ENVARC:Beam68/Plugins/A.cfg\n"
        "CAPABILITIES=FILES,AREXX\n"
        "SETTING=ENABLED|Enabled|BOOL|YES\n"
        "SETTING=MODE|Display mode|CHOICE|Compact|Compact,Full\n");
    write_test("build/testsettings/PROGDIR/Plugins/B.plugin",
        "VERSION=1\nNAME=Name B\nTRIGGER=!\nCOMMAND=PROGDIR:Plugins/B\n");
    assert(system("mkdir -p build/testsettings/PROGDIR/Plugins/Calc") == 0);
    write_test("build/testsettings/PROGDIR/Plugins/Calc/Calc.plugin",
        "VERSION=1\nID=org.beam68.calc\nPLUGIN_VERSION=0.1\nMIN_BEAM68=0.1\n"
        "NAME=Calculator\nTRIGGER==\nCOMMAND=PLUGINDIR:Calc\n");
    write_test("build/testsettings/PROGDIR/Plugins/C.plugin",
        "VERSION=1\nNAME=Name C\nTRIGGER=$\nCOMMAND=PROGDIR:Plugins/Missing\n");
    write_test("build/testsettings/PROGDIR/Plugins/A", "binary mock\n");
    write_test("build/testsettings/PROGDIR/Plugins/B", "binary mock\n");
    write_test("build/testsettings/PROGDIR/Plugins/Calc/Calc", "binary mock\n");
    beam_config_defaults(&cfg);
    assert(beam_plugin_enabled(&cfg,"PROGDIR:Plugins/SystemCommands.plugin"));
    assert(!beam_plugin_enabled(&cfg,"PROGDIR:Plugins/AminetBrowser/AminetBrowser.plugin"));
    cfg.plugin_file_count=0;
    beam_copy(cfg.install_dir,"PROGDIR:",sizeof(cfg.install_dir));
    assert(beam_catalog_resolve(&cfg,"PROGDIR:Plugins/A.plugin",path,sizeof(path)));
    assert(strcmp(path,"PROGDIR:Plugins/A.plugin")==0);
    beam_copy(cfg.install_dir,"Work:Apps/Beam68",sizeof(cfg.install_dir));
    assert(beam_catalog_resolve(&cfg,"PROGDIR:Plugins/A.plugin",path,sizeof(path)));
    assert(strcmp(path,"Work:Apps/Beam68/Plugins/A.plugin")==0);
    assert(!beam_catalog_resolve(&cfg,"PROGDIR:Plugins/*",path,sizeof(path)));
    assert(!beam_catalog_resolve(&cfg,"PROGDIR:Plugins/\\\"oops",path,sizeof(path)));
    beam_copy(cfg.install_dir,"PROGDIR:",sizeof(cfg.install_dir));
    add(&cfg,"PROGDIR:Plugins/A.plugin");
    add(&cfg,"PROGDIR:Plugins/B.plugin");
    add(&cfg,"PROGDIR:Plugins/C.plugin");
    add(&cfg,"PROGDIR:Plugins/NoSuch.plugin");
    assert(beam_catalog_discover_bundled(&cfg)==1);
    assert(cfg.plugin_file_count==5);
    assert(!strcmp(cfg.plugin_files[4],"PROGDIR:Plugins/Calc/Calc.plugin"));
    assert(!beam_plugin_enabled(&cfg,cfg.plugin_files[4]));
    memset(claimed,0,sizeof(claimed));
    state=beam_catalog_inspect(&cfg,0,claimed,&plugin);
    assert(state==BEAM_PLUGIN_OK && plugin.trigger=='!');
    assert(!strcmp(plugin.name,"Name A"));
    assert(!strcmp(plugin.settings,"ENVARC:Beam68/Plugins/A.cfg"));
    assert(beam_plugin_has_capability(&plugin,"files"));
    assert(beam_plugin_has_capability(&plugin,"AREXX"));
    assert(!beam_plugin_has_capability(&plugin,"SYSTEM"));
    assert(plugin.setting_count==2);
    assert(!strcmp(plugin.setting_defs[0].key,"ENABLED"));
    assert(plugin.setting_defs[0].type==BEAM_PLUGIN_SETTING_BOOL);
    assert(plugin.setting_defs[1].type==BEAM_PLUGIN_SETTING_CHOICE);
    assert(!strcmp(plugin.setting_defs[1].choices,"Compact,Full"));
    assert(claimed['!']);
    assert(beam_catalog_inspect(&cfg,1,claimed,&plugin)==BEAM_PLUGIN_CONFLICT);
    assert(beam_catalog_inspect(&cfg,2,claimed,&plugin)==BEAM_PLUGIN_EXECUTABLE_MISSING);
    assert(beam_catalog_inspect(&cfg,3,claimed,&plugin)==BEAM_PLUGIN_MANIFEST_ERROR);
    /* Package-style manifest: PLUGINDIR: resolves inside its own drawer. */
    assert(beam_catalog_inspect(&cfg,4,claimed,&plugin)==BEAM_PLUGIN_DISABLED);
    assert(beam_plugin_set_enabled(&cfg,cfg.plugin_files[4],1));
    assert(beam_catalog_inspect(&cfg,4,claimed,&plugin)==BEAM_PLUGIN_OK);
    assert(!strcmp(plugin.id,"org.beam68.calc") && !strcmp(plugin.plugin_version,"0.1"));
    assert(beam_plugin_set_enabled(&cfg,cfg.plugin_files[4],0));
    {
        char found[2][BEAM_MAX_PATH];
        beam_copy(found[0],"PROGDIR:Plugins/Calc/Calc.plugin",BEAM_MAX_PATH);
        beam_copy(found[1],"PROGDIR:Plugins/Weather/Weather.plugin",BEAM_MAX_PATH);
        assert(beam_catalog_merge_discovered(&cfg,(const char (*)[BEAM_MAX_PATH])found,2)==1);
        assert(cfg.plugin_file_count==6 &&
               !strcmp(cfg.plugin_files[5],"PROGDIR:Plugins/Weather/Weather.plugin") &&
               !beam_plugin_enabled(&cfg,cfg.plugin_files[5]));
        cfg.plugin_file_count=5;
        beam_plugin_forget_disabled(&cfg,"PROGDIR:Plugins/Weather/Weather.plugin");
    }
    /* A mounted development build has Plugins/ beside Beam68Prefs.  If the
       installed INSTALL_DIR is absent, diagnostics must use that local copy. */
    beam_copy(cfg.install_dir,"SYS:Utilities/Beam68",sizeof(cfg.install_dir));
    memset(claimed,0,sizeof(claimed));
    assert(beam_catalog_inspect(&cfg,0,claimed,&plugin)==BEAM_PLUGIN_OK);
    assert(!strcmp(plugin.name,"Name A"));
    beam_copy(cfg.install_dir,"PROGDIR:",sizeof(cfg.install_dir));
    assert(beam_plugin_set_enabled(&cfg,cfg.plugin_files[0],0));
    memset(claimed,0,sizeof(claimed));
    assert(beam_catalog_inspect(&cfg,0,claimed,&plugin)==BEAM_PLUGIN_DISABLED);
    assert(!claimed['!']);
    assert(beam_catalog_inspect(&cfg,1,claimed,&plugin)==BEAM_PLUGIN_OK);
    write_test("build/testsettings/PROGDIR/Plugins/Bad.plugin",
        "VERSION=1\nNAME=Bad\nTRIGGER=+\nCOMMAND=PROGDIR:Plugins/A\n"
        "SETTINGS=ENVARC:Beam68/Plugins/*bad.cfg\n");
    add(&cfg,"PROGDIR:Plugins/Bad.plugin");
    assert(beam_catalog_inspect(&cfg,5,claimed,&plugin)==BEAM_PLUGIN_MANIFEST_ERROR);
    {
        char big[300];
        char manifest[650];
        memset(big,'A',sizeof(big)-1); big[sizeof(big)-1]=0;
        sprintf(manifest,"VERSION=1\nNAME=TooLong\nTRIGGER=+\nCOMMAND=PROGDIR:Plugins/A\nSETTINGS=ENVARC:%s\n",big);
        write_test("build/testsettings/PROGDIR/Plugins/Bad.plugin",manifest);
        assert(beam_catalog_inspect(&cfg,5,claimed,&plugin)==BEAM_PLUGIN_MANIFEST_ERROR);
    }
    write_test("build/testsettings/PROGDIR/Plugins/Bad.plugin",
        "VERSION=1\nNAME=Bad\nTRIGGER=+\nCOMMAND=PROGDIR:Plugins/A\n"
        "SETTING=BAD|Missing settings path|BOOL|YES\n");
    assert(beam_catalog_inspect(&cfg,5,claimed,&plugin)==BEAM_PLUGIN_MANIFEST_ERROR);
    assert(beam_config_valid(&cfg));
    assert(beam_config_write(&cfg,BEAM_CONFIG_ENV));
    beam_config_defaults(&cfg);
    assert(beam_config_read_active(&cfg));
    assert(!strcmp(cfg.install_dir,"PROGDIR:"));
    memset(&plugin,0,sizeof(plugin));
    assert(!beam_catalog_can_preview(&plugin));
    assert(!beam_catalog_has_settings(&plugin));
    plugin.preview=1;assert(beam_catalog_can_preview(&plugin));
    strcpy(plugin.capabilities,"FILES,SYSTEM");assert(!beam_catalog_can_preview(&plugin));
    strcpy(plugin.settings,"ENVARC:Beam68/Plugins/Test.cfg");assert(beam_catalog_has_settings(&plugin));
    assert(!beam_catalog_can_preview(NULL) && !beam_catalog_has_settings(NULL));
    {
        struct BeamPluginSetting setting;char value[64];
        memset(&setting,0,sizeof(setting));
        setting.type=BEAM_PLUGIN_SETTING_BOOL;
        assert(beam_catalog_setting_value(&setting," true ",value,sizeof(value)) && !strcmp(value,"YES"));
        assert(!beam_catalog_setting_value(&setting,"maybe",value,sizeof(value)));
        setting.type=BEAM_PLUGIN_SETTING_NUMBER;
        assert(beam_catalog_setting_value(&setting,"-12",value,sizeof(value)) && !strcmp(value,"-12"));
        assert(!beam_catalog_setting_value(&setting,"+",value,sizeof(value)));
        assert(!beam_catalog_setting_value(&setting,"12x",value,sizeof(value)));
        setting.type=BEAM_PLUGIN_SETTING_CHOICE;strcpy(setting.choices,"Auto,HippoPlayer");
        assert(beam_catalog_setting_value(&setting,"hippoplayer",value,sizeof(value)) && !strcmp(value,"HippoPlayer"));
        assert(!beam_catalog_setting_value(&setting,"Unknown",value,sizeof(value)));
        setting.type=BEAM_PLUGIN_SETTING_TEXT;
        assert(beam_catalog_setting_value(&setting,"",value,sizeof(value)) && !value[0]);
        assert(!beam_catalog_setting_value(&setting,"x\nBAD=YES",value,sizeof(value)));
        strcpy(value,"unchanged");
        assert(!beam_catalog_setting_value(&setting,"long",value,2) && !strcmp(value,"unchanged"));
    }
    {
        struct BeamPlugin defaults;char values[BEAM_MAX_PLUGIN_SETTINGS][64];
        memset(&defaults,0,sizeof(defaults));defaults.setting_count=2;
        defaults.setting_defs[0].type=BEAM_PLUGIN_SETTING_BOOL;
        strcpy(defaults.setting_defs[0].default_value,"true");
        defaults.setting_defs[1].type=BEAM_PLUGIN_SETTING_CHOICE;
        strcpy(defaults.setting_defs[1].choices,"Auto,Manual");
        strcpy(defaults.setting_defs[1].default_value,"manual");
        assert(beam_catalog_setting_defaults(&defaults,values));
        assert(!strcmp(values[0],"YES") && !strcmp(values[1],"Manual"));
        strcpy(defaults.setting_defs[1].default_value,"Invalid");
        strcpy(values[0],"unsaved edit");
        assert(!beam_catalog_setting_defaults(&defaults,values));
        assert(!strcmp(values[0],"unsaved edit"));
    }
    puts("Plugin details, triggers, disabled, missing files, bundled discovery, safe settings path, install dir: PASS");
    return 0;
}
