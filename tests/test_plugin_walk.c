/* A thirty-level virtual tree exercises depth, links and cleanup deterministically. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "plugin_walk.h"
struct Handle { int depth; };
static int allocations,fail_at,live,locks,fibs,link_followed;
static LONG error;
void *AllocVec(unsigned long size,unsigned long flags)
{void *p;(void)flags;if(++allocations==fail_at)return NULL;p=calloc(1,size);if(p)live++;return p;}
void FreeVec(void *p){if(p){live--;free(p);}}
BPTR Lock(STRPTR path,LONG mode)
{
    struct Handle *h;int depth=0;const char *p=path+2;(void)mode;
    if(strstr(path,"loop")){link_followed++;return NULL;}
    if(strncmp(path,"R:",2))return NULL;
    while(*p){if(*p=='d')depth++;else if(*p!='/')return NULL;p++;}
    h=malloc(sizeof(*h));assert(h);h->depth=depth;locks++;return (BPTR)h;
}
void UnLock(BPTR p){locks--;free(p);}
void *AllocDosObject(ULONG type,void *tags){(void)type;(void)tags;fibs++;return calloc(1,sizeof(struct FileInfoBlock));}
void FreeDosObject(ULONG type,void *p){(void)type;fibs--;free(p);}
LONG Examine(BPTR lock,struct FileInfoBlock *fib){(void)lock;fib->fib_DiskKey=0;return 1;}
LONG ExNext(BPTR lock,struct FileInfoBlock *fib)
{
    int n=(int)fib->fib_DiskKey++,depth=((struct Handle *)lock)->depth;
    if(n==0){strcpy(fib->fib_FileName,"tune.mod");fib->fib_DirEntryType=-3;return 1;}
    if(n==1 && depth<30){strcpy(fib->fib_FileName,"d");fib->fib_DirEntryType=ST_USERDIR;return 1;}
    if(n==2 && depth==5){strcpy(fib->fib_FileName,"loop");fib->fib_DirEntryType=ST_LINKDIR;return 1;}
    error=ERROR_NO_MORE_ENTRIES;return 0;
}
LONG AddPart(STRPTR path,STRPTR part,LONG size)
{
    size_t n=strlen(path),m=strlen(part),separator=n && path[n-1]!=':' && path[n-1]!='/';
    if(n+m+separator+1>(size_t)size)return 0;
    if(separator)strcat(path,"/");
    strcat(path,part);return 1;
}
LONG IoErr(void){return error;}
void beam_plugin_copy(char *out,const char *s,unsigned int cap)
{if(cap){strncpy(out,s,cap-1);out[cap-1]=0;}}
int beam_ascii_prefix(const char *text,const char *prefix)
{return !strncasecmp(text,prefix,strlen(prefix));}
static int walk_tree(struct BeamConfig *cfg,int early,int *unreadable)
{
    struct BeamWalk w;char path[BEAM_MAX_PATH];int kind,root,files=0;
    beam_walk_begin(&w,cfg,NULL);
    while(beam_walk_next(&w,path,sizeof(path),&kind,&root)) {
        assert(root==0);if(kind<0)files++;
        if(early && files==early)break;
    }
    *unreadable=w.unreadable;beam_walk_end(&w);
    assert(!live && !locks && !fibs && !link_followed);return files;
}
int main(void)
{
    struct BeamConfig cfg;int errors;
    memset(&cfg,0,sizeof(cfg));cfg.dir_count=1;strcpy(cfg.dirs[0],"R:");
    cfg.max_depth=4;assert(walk_tree(&cfg,0,&errors)==5 && !errors);
    cfg.max_depth=-1;assert(walk_tree(&cfg,0,&errors)==31 && !errors);
    strcpy(cfg.excluded_dirs[0],"R:d/d/d");cfg.excluded_dir_count=1;
    assert(walk_tree(&cfg,0,&errors)==3 && !errors);
    cfg.excluded_dir_count=0;
    strcpy(cfg.install_dir,"R:");strcpy(cfg.dirs[0],"PROGDIR:");
    assert(walk_tree(&cfg,0,&errors)==31 && !errors);
    strcpy(cfg.excluded_dirs[0],"PROGDIR:d/d/d");cfg.excluded_dir_count=1;
    assert(walk_tree(&cfg,0,&errors)==3 && !errors);cfg.excluded_dir_count=0;
    assert(walk_tree(&cfg,2,&errors)==2);
    allocations=0;fail_at=5;assert(walk_tree(&cfg,0,&errors)==4 && errors==1);
    puts("Plugin traversal: unlimited depth, finite depth, exclusions, no link loops, early exit and low-memory cleanup: PASS");return 0;
}
