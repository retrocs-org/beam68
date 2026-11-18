/* Isolated native tests for saved queries and selection; never executes queries. */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "item_index.h"
#include "index_cache.h"
#include "query_history.h"
#include "favorites.h"
#include "search_rank.h"
#include "plugin_job.h"
#include "stack_guard.h"
static struct BeamIndex qa_index,loaded;
static struct BeamFavorites favorites,restored;
static struct BeamQueryHistory wiki,ha,history;
static struct BeamConfig cfg;
static char root[120],path[160],owner[64];
static const char *file(const char *name){strcpy(path,root);AddPart(path,(STRPTR)name,sizeof(path));return path;}
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 20;}}while(0)
static int run_qa(int argc,char **argv)
{
 int i,result[3]={2,0,1},tracked[3]={0,1,2};BPTR dir=0;
 (void)argc;(void)argv;
 for(i=0;i<100;++i){sprintf(root,"T:Beam68QueryQA%02d",i);dir=CreateDir(root);if(dir)break;}
 CHECK(dir);UnLock(dir);
 CHECK(beam_index_init(&qa_index,64)&&beam_index_init(&loaded,64));
 CHECK(beam_index_add(&qa_index,"SYS:AppA",BEAM_ITEM_WORKBENCH,0,NULL)==0);
 CHECK(beam_index_add(&qa_index,"QUERY:ha:entity sensor.kitchen",BEAM_ITEM_QUERY,BEAM_ROOT_SOURCES,NULL)==1);
 CHECK(beam_index_set_title(&qa_index,1,"ha:entity sensor.kitchen"));
 CHECK(beam_index_add(&qa_index,"SYS:AppB",BEAM_ITEM_WORKBENCH,0,NULL)==2);
 CHECK(beam_search_selection(qa_index.items,qa_index.count,result,3,"SYS:AppB")==0);
 result[0]=1;result[1]=0;result[2]=2;
 CHECK(beam_search_selection(qa_index.items,qa_index.count,result,3,"SYS:AppB")==2);
 CHECK(beam_job_row_same(2,"ha:entity light.wall","Wall: off",2,"ha:entity light.wall","Wall: on"));
 CHECK(!beam_job_row_same(2,"ha:entity light.wall","Wall",2,"ha:entity light.other","Wall"));
 puts("PASS: stable selection after reordered results and changing plugin state labels.");
 beam_config_defaults(&cfg);cfg.max_items=64;
 CHECK(beam_cache_save(&cfg,&qa_index,file("qa_index.txt"))&&beam_cache_load(&cfg,&loaded,file("qa_index.txt")));
 CHECK(loaded.items[1].kind==BEAM_ITEM_QUERY&&loaded.items[1].name==loaded.items[1].title);
 qa_index.items[2].flags|=BEAM_ITEM_FLAG_STALE;
 CHECK(beam_index_remove(&qa_index,"QUERY:ha:entity sensor.kitchen",tracked,3)==1);
 CHECK(qa_index.count==2&&tracked[1]==-1&&tracked[2]==1&&(qa_index.items[1].flags&BEAM_ITEM_FLAG_STALE));
 result[0]=0;result[1]=1;CHECK(beam_search_selection(qa_index.items,qa_index.count,result,2,"SYS:AppB")==1);
 puts("PASS: saved-query cache round trip and unpin compaction preserve pending scan entries.");
 CHECK(beam_plugin_query_parse("ha:entity sensor.kitchen",owner,sizeof(owner),NULL));
 CHECK(!beam_plugin_query_parse("ha:do on light.wall",owner,sizeof(owner),NULL));
 CHECK(!beam_plugin_query_parse("aminet:install util/wb/file.lha",owner,sizeof(owner),NULL));
 CHECK(!beam_plugin_query_parse("mod:play Music:file.mod",owner,sizeof(owner),NULL));
 beam_favorites_clear(&favorites);CHECK(beam_favorites_add_group(&favorites,"QUERY:wiki:Amiga","Research"));
 CHECK(beam_favorites_save(&favorites,file("Favorites.txt"))&&beam_favorites_load(&restored,file("Favorites.txt")));
 CHECK(restored.count==1&&beam_favorites_in_group(&restored,"QUERY:wiki:Amiga","Research"));
 puts("PASS: read-only query favorites persist with groups; action queries rejected.");
 beam_query_history_reset(&wiki);beam_query_history_reset(&ha);
 beam_query_history_record(&wiki,"wiki:Amiga");beam_query_history_record(&ha,"ha:entity sensor.kitchen");
 CHECK(beam_query_history_save(&wiki,file("wiki.txt"))&&beam_query_history_save(&ha,file("ha.txt")));
 CHECK(beam_query_history_load(&history,file("wiki.txt"))&&!strcmp(beam_query_history_older(&history),"wiki:Amiga"));
 CHECK(beam_query_history_load(&history,file("ha.txt"))&&!strcmp(beam_query_history_older(&history),"ha:entity sensor.kitchen"));
 beam_query_history_record(&history,"Preferences");CHECK(!strcmp(beam_query_history_recall(&history,0,1),"Preferences"));
 CHECK(!beam_query_history_recall(&history,0,1));
 puts("PASS: separate plugin histories and legacy application-history filtering.");
 beam_index_free(&qa_index);beam_index_free(&loaded);return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,run_qa,argc,argv);}
