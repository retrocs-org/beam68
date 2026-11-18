#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "item_index.h"
static void test_pool_growth(void)
{
    struct BeamIndex x;
    char path[200],title[80];
    int i,j,created;
    unsigned long initial;
    assert(beam_index_init(&x,2048));
    initial=x.pool_size;
    assert(initial==4096 && initial<2048UL*BEAM_POOL_BYTES_PER_ITEM);
    for (i=0;i<200;i++) {
        sprintf(path,"SYS:Applications/Long drawer name/Collection number %03d/Application",i);
        sprintf(title,"Title %03d",i);
        assert(beam_index_add(&x,path,BEAM_ITEM_BOOKMARK,0,&created)==i && created);
        assert(beam_index_set_title(&x,i,title));
        /* Every growth must preserve all prior paths, titles and hashes. */
        for (j=0;j<=i;j++) {
            sprintf(path,"SYS:Applications/Long drawer name/Collection number %03d/Application",j);
            sprintf(title,"Title %03d",j);
            assert(beam_index_find(&x,path)==j);
            assert(!strcmp(x.items[j].path,path));
            assert(!strcmp(x.items[j].title,title));
            assert(x.items[j].name==x.items[j].title);
        }
    }
    assert(x.pool_size>initial && x.pool_size<=2048UL*BEAM_POOL_BYTES_PER_ITEM);
    beam_index_mark_stale(&x,1,0);
    sprintf(path,"SYS:Applications/Long drawer name/Collection number %03d/Application",199);
    assert(beam_index_add(&x,path,BEAM_ITEM_BOOKMARK,0,&created)==199 && !created);
    assert(beam_index_sweep(&x)==199);
    assert(!strcmp(x.items[0].title,"Title 199"));
    assert(beam_index_find(&x,path)==0);
    beam_index_clear(&x);
    assert(beam_index_add(&x,"RAM:New",BEAM_ITEM_FILE,0,&created)==0);
    beam_index_free(&x);
}

static void test_low_memory_and_reconcile(void)
{
    struct BeamIndex x;
    int i,created,count,tracked[4];
    char path[200];
    char *pool;
    unsigned long used;
    for (i=0;i<3;i++) {
        beam_index_test_fail_after(i);
        assert(!beam_index_init(&x,512));
        assert(!x.items && !x.pool && !x.hash && !x.count);
    }
    beam_index_test_fail_after(-1);
    assert(beam_index_init(&x,512));
    for (i=0;;i++) {
        sprintf(path,"SYS:Applications/LongDrawerName/Collection-%03d/Tool",i);
        if (x.pool_used+strlen(path)+1>x.pool_size) break;
        assert(beam_index_add(&x,path,BEAM_ITEM_FILE,0,NULL)==i);
    }
    pool=x.pool;used=x.pool_used;count=x.count;
    beam_index_test_fail_after(0);
    assert(beam_index_add(&x,path,BEAM_ITEM_FILE,0,&created)==-1 && !created);
    assert(x.full && x.pool==pool && x.pool_used==used && x.count==count);
    assert(beam_index_find(&x,x.items[0].path)==0);
    beam_index_test_fail_after(-1);
    assert(beam_index_add(&x,path,BEAM_ITEM_FILE,0,NULL)==count);
    assert(beam_index_set_title(&x,0,"Original title"));
    pool=x.pool;used=x.pool_used;
    beam_index_test_fail_after(0);
    assert(!beam_index_set_title(&x,0,"A longer changed title"));
    assert(x.pool==pool && x.pool_used==used && !strcmp(x.items[0].title,"Original title"));
    assert(beam_index_find(&x,x.items[count].path)==count);
    beam_index_test_fail_after(-1);
    assert(beam_index_set_title(&x,0,"A longer changed title"));
    assert(!strcmp(x.items[0].title,"A longer changed title"));
    beam_index_clear(&x);
    assert(beam_index_add(&x,"SYS:A",BEAM_ITEM_FILE,0,NULL)==0);
    assert(beam_index_add(&x,"SYS:B",BEAM_ITEM_FILE,0,NULL)==1);
    assert(beam_index_set_title(&x,1,"B title"));
    assert(beam_index_add(&x,"SYS:C",BEAM_ITEM_FILE,0,NULL)==2);
    assert(beam_index_add(&x,"SYS:D",BEAM_ITEM_FILE,1,NULL)==3);
    beam_index_mark_stale(&x,1,0);
    assert(beam_index_add(&x,"SYS:C",BEAM_ITEM_FILE,0,NULL)==2);
    tracked[0]=0;tracked[1]=1;tracked[2]=2;tracked[3]=3;
    /* Failed traversal does not prove A and B vanished. */
    assert(beam_index_reconcile(&x,0,tracked,4)==0 && x.count==4);
    assert(tracked[1]==1 && !strcmp(x.items[1].title,"B title"));
    beam_index_mark_stale(&x,1,0);
    assert(beam_index_add(&x,"SYS:C",BEAM_ITEM_FILE,0,NULL)==2);
    assert(beam_index_reconcile(&x,1,tracked,4)==2 && x.count==2);
    assert(tracked[0]==-1 && tracked[1]==-1 && tracked[2]==0 && tracked[3]==1);
    assert(beam_index_find(&x,"SYS:C")==0 && beam_index_find(&x,"SYS:D")==1);
    assert(beam_index_find(&x,"SYS:A")==-1);
    beam_index_free(&x);
}

static void test_saved_query_title(void)
{
    struct BeamIndex x;int i;char name[120];
    assert(beam_index_init(&x,64));
    assert(beam_index_add(&x,"QUERY:ha:entity sensor.kitchen",BEAM_ITEM_QUERY,BEAM_ROOT_SOURCES,NULL)==0);
    assert(beam_index_set_title(&x,0,"ha:entity sensor.kitchen"));
    for(i=0;i<50;++i){sprintf(name,"SYS:Long drawer name/Folder%03d/Some application",i);assert(beam_index_add(&x,name,BEAM_ITEM_FILE,0,NULL)>=0);}
    assert(x.items[0].name==x.items[0].title&&!strcmp(x.items[0].name,"ha:entity sensor.kitchen"));
    beam_index_mark_stale(&x,1UL,0);assert(beam_index_sweep(&x)==50);
    assert(x.items[0].kind==BEAM_ITEM_QUERY&&x.items[0].name==x.items[0].title);
    {int tracked[2]={0,-1};
     assert(beam_index_add(&x,"SYS:Pending scan",BEAM_ITEM_FILE,0,NULL)==1);
     x.items[1].flags|=BEAM_ITEM_FLAG_STALE;tracked[1]=1;
     assert(beam_index_remove(&x,"QUERY:ha:entity sensor.kitchen",tracked,2)==1);
     assert(x.count==1&&tracked[0]==-1&&tracked[1]==0&&(x.items[0].flags&BEAM_ITEM_FLAG_STALE));
    }
    beam_index_free(&x);
}
int main(void)
{
    struct BeamIndex x;
    int created,i;
    char path[64];
    test_saved_query_title();
    test_pool_growth();
    test_low_memory_and_reconcile();
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
    assert(beam_index_set_title(&x,0,"Updated existing title"));
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
