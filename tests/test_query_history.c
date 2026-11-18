#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "query_history.h"
#define PATH "ENVARC:Beam68/Queries.txt"
int main(void)
{
    struct BeamQueryHistory h,r; const char *q; int i; char line[32];
    {char owner[64],file[160];const char *input;
     assert(beam_plugin_query_parse("HA:entity light.wall",owner,sizeof(owner),&input));
     assert(!strcmp(owner,"org.beam68.amihass")&&!strcmp(input,"entity light.wall"));
     assert(!beam_plugin_query_parse("ha:do on light.wall",owner,sizeof(owner),NULL));
     assert(!beam_plugin_query_parse("ha:DO toggle light.wall",owner,sizeof(owner),NULL));
     assert(beam_plugin_query_parse("aminet:search tracker",owner,sizeof(owner),NULL));
     assert(!beam_plugin_query_parse("aminet:install mus/player.lha",owner,sizeof(owner),NULL));
     assert(!beam_plugin_query_parse("mod:play Music:test.mod",owner,sizeof(owner),NULL));
     assert(beam_plugin_query_parse("wiki:Amiga",owner,sizeof(owner),NULL));
     assert(!beam_plugin_query_parse(">delete all",owner,sizeof(owner),NULL));
     assert(beam_plugin_history_path("org.beam68.wikipedia",file,sizeof(file)));
     assert(strstr(file,"PluginQueries/org.beam68.wikipedia.txt"));
     assert(!beam_plugin_history_path("../Bad/Path",file,sizeof(file)));
     assert(!beam_plugin_history_path("..",file,sizeof(file)));
    }
    beam_query_history_reset(&h);
    beam_query_history_record(&h,"ha:do on light.wall");
    beam_query_history_record(&h,"wiki:Amiga");
    beam_query_history_record(&h,"Preferences");
    assert(!strcmp(beam_query_history_recall(&h,0,1),"Preferences"));
    assert(!beam_query_history_recall(&h,0,1));
    beam_query_history_reset(&h);
    assert(!beam_query_history_older(&h));
    beam_query_history_record(&h,"screen mode");
    beam_query_history_record(&h,">version");
    beam_query_history_record(&h,"screen mode");
    assert(h.count==2 && !strcmp(h.entries[1],"screen mode"));
    q=beam_query_history_older(&h); assert(q && !strcmp(q,"screen mode"));
    q=beam_query_history_older(&h); assert(q && !strcmp(q,">version"));
    q=beam_query_history_newer(&h); assert(q && !strcmp(q,"screen mode"));
    q=beam_query_history_newer(&h); assert(q && !*q);
    for(i=0;i<20;i++){sprintf(line,"query %d",i);beam_query_history_record(&h,line);}
    assert(h.count==BEAM_QUERY_HISTORY_CAP);
    assert(beam_query_history_save(&h,PATH));
    assert(beam_query_history_load(&r,PATH));
    assert(r.count==BEAM_QUERY_HISTORY_CAP && !strcmp(r.entries[r.count-1],"query 19"));
    {char wiki[160],ha[160];
     assert(system("mkdir -p build/testsettings/ENVARC/Beam68/PluginQueries")==0);
     assert(beam_plugin_history_path("org.beam68.wikipedia",wiki,sizeof(wiki)));
     assert(beam_plugin_history_path("org.beam68.amihass",ha,sizeof(ha)));
     beam_query_history_reset(&h);beam_query_history_record(&h,"wiki:Amiga");assert(beam_query_history_save(&h,wiki));
     beam_query_history_reset(&h);beam_query_history_record(&h,"ha:entity sensor.kitchen");assert(beam_query_history_save(&h,ha));
     assert(beam_query_history_load(&r,wiki));assert(r.count==1&&!strcmp(beam_query_history_older(&r),"wiki:Amiga"));
     assert(beam_query_history_load(&r,ha));assert(r.count==1&&!strcmp(beam_query_history_older(&r),"ha:entity sensor.kitchen"));
    }
    puts("Recent query history, recall, dedup and persistence: PASS");
    return 0;
}
