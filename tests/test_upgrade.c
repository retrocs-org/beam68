/* 0.1-format user data loaded, saved and reopened by the 0.2 code. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam.h"
#include "favorites.h"
#include "history.h"
#include "query_history.h"
#include "plugin_health.h"
#include "aliases.h"
#include "theme_file.h"
static void put(const char *name,const char *text)
{
    char path[320];FILE *f;
    sprintf(path,"build/testsettings/%s",name);
    f=fopen(path,"wb");assert(f);assert(fputs(text,f)>=0);assert(!fclose(f));
}
int main(void)
{
    static struct BeamConfig c,d;
    static struct BeamFavorites favorites;
    static struct BeamHistory history;
    static struct BeamQueryHistory queries;
    static struct BeamPluginHealth health;
    struct BeamAliases aliases={0};
    unsigned long colors[BEAM_THEME_COLOR_COUNT];
    char file[8192];FILE *f;size_t n;
    assert(system("rm -rf build/testsettings; mkdir -p build/testsettings/ENV/Beam68 build/testsettings/ENVARC/Beam68 build/testsettings/PROGDIR build/testsettings/T/Beam68")==0);
    put("ENVARC/Beam68/Beam68.cfg","# My 0.1 settings\nMAX_ITEMS=256\nHOTKEY=ctrl alt space\nTHEME=CUSTOM\nCUSTOM_COLORS=112233,445566,778899,AABBCC,DDEEFF\nPLUGIN_DISABLED=PROGDIR:Plugins/Clock.plugin\nPLUGIN=PROGDIR:Plugins/Clock.plugin\nPLUGIN=Work:ThirdParty/Weather.plugin\nPRIVATE_EXTENSION_KEY=keep-me\n");
    put("ENVARC/Beam68/Favorites.txt","# Beam68 0.1 favorites; one full application path per line\nSYS:Tools/Ed\nGraphics|Work:DeluxePaint\n");
    put("ENVARC/Beam68/History.txt","9\tSYS:Tools/Ed\n2\tWork:DeluxePaint\n");
    put("ENVARC/Beam68/Queries.txt","app:Ed\nclock:utc\n");
    put("ENVARC/Beam68/PluginHealth.txt","4\t1\t0\t1\tPROGDIR:Plugins/Clock.plugin\n");
    put("ENVARC/Beam68/Aliases.txt","SYS:Tools/Ed\tedit notes\n");
    put("ENVARC/Beam68/My.theme","# Beam68 0.1 portable theme\nBEAM68THEME=1\nCOLORS=112233,445566,778899,AABBCC,DDEEFF\n");
    beam_config_defaults(&c);assert(beam_config_read_active(&c) && beam_config_valid(&c));
    assert(c.max_items==256 && c.theme==BEAM_THEME_CUSTOM && c.custom_colors[0]==0x112233UL);
    assert(c.plugin_file_count==2 && !beam_plugin_enabled(&c,"PROGDIR:Plugins/Clock/Clock.plugin"));
    assert(beam_plugin_enabled(&c,"Work:ThirdParty/Weather.plugin"));
    assert(beam_config_write(&c,BEAM_CONFIG_ENV));
    beam_config_defaults(&d);assert(beam_config_read_active(&d));
    assert(d.max_items==c.max_items && !strcmp(d.hotkey,c.hotkey) && d.custom_colors[4]==c.custom_colors[4]);
    f=fopen("build/testsettings/ENV/Beam68/Beam68.cfg","rb");assert(f);n=fread(file,1,sizeof(file)-1,f);file[n]=0;fclose(f);
    assert(strstr(file,"# My 0.1 settings") && strstr(file,"PRIVATE_EXTENSION_KEY=keep-me"));
    assert(beam_favorites_load_active(&favorites) && favorites.count==2);
    assert(beam_favorites_in_group(&favorites,"Work:DeluxePaint","Graphics"));
    assert(beam_favorites_save(&favorites,BEAM_FAVS_ENV));
    assert(beam_favorites_load_active(&favorites) && favorites.count==2);
    assert(beam_history_load(&history,BEAM_HISTORY_PATH) && history.count==2);
    assert(beam_history_bonus(&history,"SYS:Tools/Ed")==54);
    assert(beam_history_save(&history,BEAM_HISTORY_PATH));
    assert(beam_history_load(&history,BEAM_HISTORY_PATH) && history.entries[0].launches==9);
    assert(beam_query_history_load(&queries,BEAM_QUERY_HISTORY_PATH) && queries.count==2);
    assert(!strcmp(beam_query_history_older(&queries),"clock:utc"));
    assert(beam_query_history_save(&queries,BEAM_QUERY_HISTORY_PATH));
    assert(beam_query_history_load(&queries,BEAM_QUERY_HISTORY_PATH) && queries.count==2);
    assert(beam_plugin_health_load(&health,BEAM_PLUGIN_HEALTH_PATH));
    assert(beam_plugin_health_find(&health,"PROGDIR:Plugins/Clock/Clock.plugin")->successes==4);
    beam_aliases_clear(&aliases);assert(beam_aliases_load_active(&aliases));
    assert(beam_aliases_find(&aliases,"SYS:Tools/Ed")>=0);beam_aliases_free(&aliases);
    assert(beam_theme_file_load("ENVARC:Beam68/My.theme",colors));
    assert(colors[0]==0x112233UL && colors[4]==0xddeeffUL);
    puts("0.1 to 0.2 upgrade: settings, plugins, favorites, history, aliases and theme: PASS");
    return 0;
}
