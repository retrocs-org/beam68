#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_cache.h"
static struct BeamConfig cfg;
static struct BeamIndex items,loaded;
static const char *cache_file=BEAM_INDEX_CACHE_PATH;
static const char *host_file="build/testsettings/PROGDIR/Beam68.index";
static void file_overwrite(const char *path,const char *text)
{
    FILE *f=fopen(path,"wb");
    assert(f);
    assert(fputs(text,f)>=0);
    assert(fclose(f)==0);
}
static void fill(void)
{
    beam_index_clear(&items);
    assert(beam_index_add(&items,"SYS:Tools/Ed",BEAM_ITEM_WORKBENCH,0,NULL)==0);
    assert(beam_index_add(&items,"Work:Games/Amiga%App\tX",BEAM_ITEM_SHELL,0,NULL)==1);
    assert(beam_index_add(&items,"Work:Games/ReadMe",BEAM_ITEM_FILE,0,NULL)==2);
    assert(beam_index_add(&items,"Work:",BEAM_ITEM_VOLUME,BEAM_ROOT_NONE,NULL)==3);
    assert(beam_index_add(&items,"Work:Games/Lotus",BEAM_ITEM_GAME,0,NULL)==4);
    assert(beam_index_set_title(&items,4,"Lotus Turbo\tChallenge (1990 Gremlin)"));
    assert(beam_index_add(&items,"http://aminet.net/",BEAM_ITEM_BOOKMARK,BEAM_ROOT_SOURCES,NULL)==5);
    assert(beam_index_set_title(&items,5,"Aminet") && !strcmp(items.items[5].name,"Aminet"));
    assert(beam_index_add(&items,"SNIPPET:sig",BEAM_ITEM_SNIPPET,BEAM_ROOT_SOURCES,NULL)==6);
    assert(beam_index_set_title(&items,6,"Best regards"));
}
int main(void)
{
    FILE *f;
    char hdr[256];
    int rc=system("mkdir -p build/testsettings/PROGDIR");
    assert(rc==0);
    memset(&cfg,0,sizeof(cfg));
    cfg.index_cache=1; cfg.scan_icons=1; cfg.max_items=16; cfg.max_depth=4;
    cfg.dir_count=1; strcpy(cfg.dirs[0],"SYS:Tools");
    assert(beam_index_init(&items,16) && beam_index_init(&loaded,16));
    fill();
    assert(!strcmp(items.items[1].name,"Amiga%App\tX"));
    assert(!strcmp(items.items[3].name,"Work:"));
    assert(beam_cache_save(&cfg,&items,cache_file));
    assert(beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==7 && loaded.items[5].kind==BEAM_ITEM_BOOKMARK &&
           !strcmp(loaded.items[5].name,"Aminet") && loaded.items[5].root==BEAM_ROOT_SOURCES &&
           loaded.items[6].kind==BEAM_ITEM_SNIPPET && !strcmp(loaded.items[6].title,"Best regards"));
    assert(loaded.items[4].kind==BEAM_ITEM_GAME &&
           !strcmp(loaded.items[4].title,"Lotus Turbo\tChallenge (1990 Gremlin)") &&
           !loaded.items[0].title &&
           loaded.items[2].kind==BEAM_ITEM_FILE &&
           loaded.items[3].kind==BEAM_ITEM_VOLUME &&
           loaded.items[3].root==BEAM_ROOT_NONE);
    assert(!strcmp(items.items[1].path,loaded.items[1].path));
    assert(!strcmp(loaded.items[0].name,"Ed"));
    cfg.max_depth=5; /* stale config fingerprint */
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    cfg.max_depth=4;
    assert(beam_cache_save(&cfg,&items,cache_file));
    f=fopen(host_file,"rb");
    assert(f && fgets(hdr,sizeof(hdr),f));
    fclose(f);
    assert(!strncmp(hdr,"BEAM68CACHE5\t",13));
    hdr[11]='3';
    file_overwrite(host_file,hdr);
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    /* Truncated cache must not yield a partial startup index. */
    assert(beam_cache_save(&cfg,&items,cache_file));
    f=fopen(host_file,"rb");
    assert(f && fgets(hdr,sizeof(hdr),f));
    fclose(f);
    file_overwrite(host_file,hdr);
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    /* A changed data row fails checksum and is rejected. */
    assert(beam_cache_save(&cfg,&items,cache_file));
    f=fopen(host_file,"r+b");
    assert(f);
    assert(fgets(hdr,sizeof(hdr),f));
    assert(fgetc(f)=='1');
    fseek(f,-1L,SEEK_CUR);
    assert(fputc('2',f)=='2');
    fclose(f);
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    cfg.index_cache=0;
    assert(!beam_cache_save(&cfg,&items,cache_file));
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    beam_index_free(&items); beam_index_free(&loaded);
    puts("Text index cache, fingerprint, escaping, corruption recovery: PASS");
    return 0;
}
