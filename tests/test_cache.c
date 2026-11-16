#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_cache.h"
void beam_test_fail_writes(int after);
void beam_test_fail_rename(int after);
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
static void remove_backups(void)
{
    remove("build/testsettings/PROGDIR/Beam68.index.bak");
    remove("build/testsettings/PROGDIR/Beam68.index.old");
}
static void test_interrupted_recovery(void)
{
    const char *bak="build/testsettings/PROGDIR/Beam68.index.bak";
    const char *old="build/testsettings/PROGDIR/Beam68.index.old";
    const char *pending="build/testsettings/PROGDIR/Beam68.index.new";
    int i;
    FILE *f;
    remove_backups();remove(pending);
    fill();assert(beam_cache_save(&cfg,&items,cache_file));
    assert(beam_index_add(&items,"SYS:NewEntry",BEAM_ITEM_FILE,0,NULL)==7);
    assert(beam_cache_save(&cfg,&items,cache_file)); /* primary 8, backup 7 */
    file_overwrite(pending,"unfinished new cache\n");
    beam_index_clear(&loaded);
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==8);
    /* Power loss after moving primary aside with an existing backup. */
    assert(!rename(host_file,old));
    file_overwrite(pending,"uncommitted cache\n");
    beam_index_clear(&loaded);
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==8);
    f=fopen(host_file,"rb");assert(f);fclose(f);
    assert(!beam_index_set_title(&loaded,99,"invalid"));
    /* A damaged primary falls back to a fully validated committed backup. */
    file_overwrite(host_file,"corrupt primary\n");
    file_overwrite(old,"corrupt transitional backup\n");
    beam_index_clear(&loaded);
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    assert(!strcmp(loaded.items[5].name,"Aminet"));
    /* Both repair rename failures leave the valid backup available. */
    for (i=0;i<2;i++) {
        fill();assert(beam_cache_save(&cfg,&items,cache_file));
        assert(beam_cache_save(&cfg,&items,cache_file));
        file_overwrite(host_file,"bad primary\n");
        beam_test_fail_rename(i);
        beam_index_clear(&loaded);
        assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
        f=fopen(bak,"rb");assert(f);fclose(f);
        beam_index_clear(&loaded);
        assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    }
    /* Missing primary after the original-to-backup rename. */
    assert(!rename(host_file,bak));
    beam_index_clear(&loaded);
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    /* A lone complete .new is still uncommitted and cannot replace a scan. */
    remove_backups();assert(!rename(host_file,pending));
    beam_index_clear(&loaded);
    assert(!beam_cache_load(&cfg,&loaded,cache_file) && !loaded.count);
    remove(pending);
    /* Reject malformed and mismatched backups without touching live results. */
    fill();assert(beam_cache_save(&cfg,&items,cache_file));
    assert(!rename(host_file,bak));file_overwrite(bak,"bad backup\n");
    assert(beam_index_add(&loaded,"RAM:Keep",BEAM_ITEM_FILE,0,NULL)==0);
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==1 && beam_index_find(&loaded,"RAM:Keep")==0);
    remove_backups();fill();assert(beam_cache_save(&cfg,&items,cache_file));
    assert(!rename(host_file,bak));++cfg.max_depth;
    assert(!beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==1);
    --cfg.max_depth;
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    puts("Interrupted cache save: primary precedence, committed backup recovery and repair rollback: PASS");
}

static void test_save_rollback(void)
{
    int i;
    fill();
    assert(beam_cache_save(&cfg,&items,cache_file));
    assert(beam_index_add(&items,"SYS:NewEntry",BEAM_ITEM_FILE,0,NULL)==7);
    beam_test_fail_writes(0);
    assert(!beam_cache_save(&cfg,&items,cache_file));
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    for (i=0;i<2;i++) {
        beam_test_fail_rename(i);
        assert(!beam_cache_save(&cfg,&items,cache_file));
        assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
        assert(beam_index_find(&loaded,"SYS:NewEntry")==-1);
    }
    assert(beam_cache_save(&cfg,&items,cache_file));
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==8);
    fill();
    assert(beam_cache_save(&cfg,&items,cache_file));
    assert(beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
}

static void test_transactional_reload(void)
{
    struct BeamIndex source,target;
    struct BeamConfig config=cfg;
    char path[160],title[64];
    char *pool;
    int i,budget;
    config.max_items=512;
    assert(beam_index_init(&source,512) && beam_index_init(&target,512));
    assert(beam_index_add(&target,"SYS:KnownGood",BEAM_ITEM_WORKBENCH,0,NULL)==0);
    assert(beam_cache_save(&config,&target,cache_file));
    for (i=0;i<200;i++) {
        sprintf(path,"SYS:Applications/Long drawer name/Collection %03d/Application",i);
        sprintf(title,"Bookmark title %03d",i);
        assert(beam_index_add(&source,path,BEAM_ITEM_BOOKMARK,0,NULL)==i);
        assert(beam_index_set_title(&source,i,title));
    }
    assert(beam_cache_save(&config,&source,cache_file));
    pool=target.pool;
    for (budget=0;budget<5;budget++) {
        beam_index_test_fail_after(budget);
        assert(!beam_cache_load(&config,&target,cache_file));
        assert(target.pool==pool && target.count==1);
        assert(beam_index_find(&target,"SYS:KnownGood")==0);
    }
    beam_index_test_fail_after(-1);
    assert(beam_cache_load(&config,&target,cache_file));
    assert(target.count==200 && !strcmp(target.items[199].name,"Bookmark title 199"));
    remove_backups();
    file_overwrite(host_file,"corrupt cache\n");
    assert(!beam_cache_load(&config,&target,cache_file));
    assert(target.count==200 && beam_index_find(&target,target.items[199].path)==199);
    beam_index_free(&source);beam_index_free(&target);
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
    test_interrupted_recovery();
    test_save_rollback();
    test_transactional_reload();
    assert(beam_cache_save(&cfg,&items,cache_file));
    cfg.disabled_plugin_count=1;strcpy(cfg.disabled_plugins[0],"PROGDIR:Plugins/WHDLoad/WHDLoad.plugin");
    assert(!beam_cache_load(&cfg,&loaded,cache_file) && loaded.count==7);
    cfg.disabled_plugin_count=0;
    cfg.max_depth=-1;assert(!beam_cache_load(&cfg,&loaded,cache_file));
    cfg.max_depth=5; /* stale config fingerprint */
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==7); /* rejected reload preserves the known-good index */
    beam_index_clear(&loaded);
    cfg.max_depth=4;
    assert(beam_cache_save(&cfg,&items,cache_file));
    f=fopen(host_file,"rb");
    assert(f && fgets(hdr,sizeof(hdr),f));
    fclose(f);
    assert(!strncmp(hdr,"BEAM68CACHE5\t",13));
    remove_backups();
    hdr[11]='3';
    remove_backups();
    file_overwrite(host_file,hdr);
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    /* Truncated cache must not yield a partial startup index. */
    assert(beam_cache_save(&cfg,&items,cache_file));
    f=fopen(host_file,"rb");
    assert(f && fgets(hdr,sizeof(hdr),f));
    fclose(f);
    remove_backups();
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
    remove_backups();
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    assert(loaded.count==0);
    cfg.index_cache=0;
    assert(!beam_cache_save(&cfg,&items,cache_file));
    assert(!beam_cache_load(&cfg,&loaded,cache_file));
    beam_index_free(&items); beam_index_free(&loaded);
    puts("Text index cache, fingerprint, escaping, corruption recovery: PASS");
    return 0;
}
