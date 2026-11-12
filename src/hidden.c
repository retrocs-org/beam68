#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include "hidden.h"

#ifdef BEAM_HOST
#include <stdlib.h>
#define LIST_ALLOC(n) calloc(1,(size_t)(n))
#define LIST_FREE(p) free(p)
#else
#include <exec/memory.h>
#include <proto/exec.h>
#define LIST_ALLOC(n) AllocVec((ULONG)(n),MEMF_ANY|MEMF_CLEAR)
#define LIST_FREE(p) FreeVec(p)
#endif

/* (Re)size a path list, keeping the first entries.  0 when out of memory. */
static int size_list(BeamPathLine **paths,int *count,int *cap,int want)
{
    BeamPathLine *fresh;
    if (want<1) want=1;
    if (*paths && *cap==want) return 1;
    fresh=(BeamPathLine *)LIST_ALLOC(sizeof(BeamPathLine)*(unsigned long)want);
    if (!fresh) return 0;
    if (*paths) {
        if (*count>want) *count=want;
        memcpy(fresh,*paths,sizeof(BeamPathLine)*(unsigned long)*count);
        LIST_FREE(*paths);
    }
    *paths=fresh; *cap=want;
    return 1;
}

int beam_hidden_size(struct BeamHidden *h,int cap)
{
    if (cap>BEAM_HIDDEN_MAX) cap=BEAM_HIDDEN_MAX;
    return size_list(&h->paths,&h->count,&h->cap,cap);
}
void beam_hidden_free(struct BeamHidden *h)
{ if (h->paths) LIST_FREE(h->paths); h->paths=NULL; h->count=h->cap=0; }
int beam_recent_docs_size(struct BeamRecentDocs *r,int cap)
{
    if (cap>BEAM_RECENT_DOCS_MAX) cap=BEAM_RECENT_DOCS_MAX;
    return size_list(&r->paths,&r->count,&r->cap,cap);
}
void beam_recent_docs_free(struct BeamRecentDocs *r)
{ if (r->paths) LIST_FREE(r->paths); r->paths=NULL; r->count=r->cap=0; }

void beam_hidden_load(struct BeamHidden *h,const char *file)
{
    char line[BEAM_MAX_PATH+4];
    BPTR f;
    h->count=0;
    if (!h->paths && !beam_hidden_size(h,BEAM_HIDDEN_DEFAULT)) return;
    f=Open((STRPTR)file,MODE_OLDFILE);
    if (!f) return;
    while (h->count<h->cap && FGets(f,line,sizeof(line))) {
        char *t=beam_trim(line);
        if (!*t || *t=='#' || strlen(t)>=BEAM_MAX_PATH || beam_hidden_contains(h,t)) continue;
        beam_copy(h->paths[h->count++],t,BEAM_MAX_PATH);
    }
    Close(f);
}

int beam_hidden_save(const struct BeamHidden *h,const char *file)
{
    BPTR f,dir;
    int i,ok=1;
    dir=CreateDir((STRPTR)"ENVARC:Beam68");
    if (dir) UnLock(dir);
    f=Open((STRPTR)file,MODE_NEWFILE);
    if (!f) return 0;
    if (FPuts(f,(STRPTR)"# Beam68 hidden results (Shift+Del in the launcher)\n")<0) ok=0;
    for (i=0;ok && i<h->count;i++)
        if (FPuts(f,(STRPTR)h->paths[i])<0 || FPuts(f,(STRPTR)"\n")<0) ok=0;
    if (!Close(f)) ok=0;
    return ok;
}

int beam_hidden_contains(const struct BeamHidden *h,const char *path)
{
    int i;
    for (i=0;i<h->count;i++) if (beam_equal(h->paths[i],path)) return 1;
    return 0;
}

int beam_hidden_toggle(struct BeamHidden *h,const char *path)
{
    int i;
    if (!path || !*path || strlen(path)>=BEAM_MAX_PATH || strchr(path,'\n')) return -1;
    for (i=0;i<h->count;i++) if (beam_equal(h->paths[i],path)) {
        for (;i<h->count-1;i++) beam_copy(h->paths[i],h->paths[i+1],BEAM_MAX_PATH);
        --h->count;
        return 0;
    }
    if (!h->paths && !beam_hidden_size(h,BEAM_HIDDEN_DEFAULT)) return -1;
    if (h->count>=h->cap) return -1;
    beam_copy(h->paths[h->count++],path,BEAM_MAX_PATH);
    return 1;
}

/* -------------------------------------------------------- recent docs */

void beam_recent_docs_load(struct BeamRecentDocs *r,const char *file)
{
    char line[BEAM_MAX_PATH+4];
    BPTR f;
    int i,dup;
    r->count=0;
    if (!r->paths && !beam_recent_docs_size(r,BEAM_RECENT_DOCS_DEFAULT)) return;
    f=Open((STRPTR)file,MODE_OLDFILE);
    if (!f) return;
    while (r->count<r->cap && FGets(f,line,sizeof(line))) {
        char *t=beam_trim(line);
        if (!*t || *t=='#' || strlen(t)>=BEAM_MAX_PATH) continue;
        for (dup=0,i=0;i<r->count;i++) if (beam_equal(r->paths[i],t)) dup=1;
        if (!dup) beam_copy(r->paths[r->count++],t,BEAM_MAX_PATH);
    }
    Close(f);
}

int beam_recent_docs_save(const struct BeamRecentDocs *r,const char *file)
{
    BPTR f,dir;
    int i,ok=1;
    dir=CreateDir((STRPTR)"ENVARC:Beam68");
    if (dir) UnLock(dir);
    f=Open((STRPTR)file,MODE_NEWFILE);
    if (!f) return 0;
    for (i=0;ok && i<r->count;i++)
        if (FPuts(f,(STRPTR)r->paths[i])<0 || FPuts(f,(STRPTR)"\n")<0) ok=0;
    if (!Close(f)) ok=0;
    return ok;
}

void beam_recent_docs_record(struct BeamRecentDocs *r,const char *path)
{
    int i,at=-1;
    if (!path || !*path || strlen(path)>=BEAM_MAX_PATH || strchr(path,'\n')) return;
    if (!r->paths && !beam_recent_docs_size(r,BEAM_RECENT_DOCS_DEFAULT)) return;
    for (i=0;i<r->count;i++) if (beam_equal(r->paths[i],path)) { at=i; break; }
    if (at<0) at=r->count<r->cap?r->count++:r->cap-1;
    for (i=at;i>0;i--) beam_copy(r->paths[i],r->paths[i-1],BEAM_MAX_PATH);
    beam_copy(r->paths[0],path,BEAM_MAX_PATH);
}
