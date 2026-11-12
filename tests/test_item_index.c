#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "item_index.h"
int main(void)
{
    struct BeamIndex x;
    int created,i;
    char path[64];
    assert(beam_index_init(&x,64));
    assert(beam_index_add(&x,"SYS:Tools/Ed",BEAM_ITEM_FILE,0,&created)==0 && created);
    /* Case-insensitive duplicate upgrades the kind instead of appending. */
    assert(beam_index_add(&x,"sys:tools/ed",BEAM_ITEM_WORKBENCH,0,&created)==0 && !created);
    assert(x.items[0].kind==BEAM_ITEM_WORKBENCH);
    assert(beam_index_add(&x,"SYS:Tools/Ed",BEAM_ITEM_FILE,0,&created)==0);
    assert(x.items[0].kind==BEAM_ITEM_WORKBENCH);
    assert(!strcmp(x.items[0].name,"Ed"));
    /* A WHDLoad game icon is never downgraded to a plain project. */
    assert(beam_index_add(&x,"Games:Turrican",BEAM_ITEM_GAME,0,&created)==1 && created);
    assert(beam_index_add(&x,"games:turrican",BEAM_ITEM_WORKBENCH,0,&created)==1);
    assert(x.items[1].kind==BEAM_ITEM_GAME);
    /* Titles live right after their path and survive compaction. */
    assert(beam_index_set_title(&x,1,"Turrican II (1991 Factor 5)"));
    assert(!beam_index_set_title(&x,0,"too late: not adjacent"));
    beam_index_mark_stale(&x,1UL,0);
    beam_index_add(&x,"Games:Turrican",BEAM_ITEM_GAME,0,&created);
    assert(beam_index_sweep(&x)==1 && x.count==1);
    assert(!strcmp(x.items[0].path,"Games:Turrican") &&
           !strcmp(x.items[0].title,"Turrican II (1991 Factor 5)"));
    beam_index_clear(&x);
    assert(beam_index_add(&x,"SYS:Tools/Ed",BEAM_ITEM_WORKBENCH,0,&created)==0);
    assert(!strcmp(beam_index_name_for("RAM:"),"RAM:"));
    assert(!strcmp(beam_index_name_for("Work:A/B"),"B"));
    for (i=0;i<20;i++) {
        sprintf(path,"Work:Dir%d/File%d",i%3,i);
        assert(beam_index_add(&x,path,BEAM_ITEM_FILE,i%3,&created)>0 && created);
    }
    assert(beam_index_add(&x,"RAM:",BEAM_ITEM_VOLUME,BEAM_ROOT_NONE,&created)==21);
    /* Rescan of root 1: everything under it goes stale; re-found ones live. */
    beam_index_mark_stale(&x,1UL<<1,0);
    assert(beam_index_add(&x,"Work:Dir1/File1",BEAM_ITEM_FILE,1,&created)>0 && !created);
    assert(beam_index_sweep(&x)==6);
    assert(x.count==16);
    assert(beam_index_find(&x,"Work:Dir1/File1")>=0);
    assert(beam_index_find(&x,"Work:Dir1/File4")<0);
    assert(beam_index_find(&x,"Work:Dir2/File5")>=0);
    assert(beam_index_find(&x,"ram:")>=0);
    for (i=0;i<x.count;i++) {
        assert(x.items[i].name>=x.items[i].path);
        assert(beam_index_find(&x,x.items[i].path)==i);
    }
    /* Capacity is enforced. */
    beam_index_clear(&x);
    for (i=0;i<64;i++) {
        sprintf(path,"Work:F%d",i);
        assert(beam_index_add(&x,path,BEAM_ITEM_FILE,0,NULL)==i);
    }
    assert(beam_index_add(&x,"Work:Overflow",BEAM_ITEM_FILE,0,NULL)==-1 && x.full);
    beam_index_free(&x);
    puts("Pooled index: hashing, kind upgrade, mark-and-sweep rescans, capacity: PASS");
    return 0;
}
