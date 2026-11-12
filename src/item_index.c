/* Pooled, hashed resident index.  Portable C89; on AmigaOS it allocates with
 * AllocVec(), and on a host test build (BEAM_HOST) with calloc(). */
#include <string.h>
#include "item_index.h"

#ifdef BEAM_HOST
#include <stdlib.h>
#define INDEX_ALLOC(n) calloc(1,(size_t)(n))
#define INDEX_FREE(p) free(p)
#else
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#define INDEX_ALLOC(n) AllocVec((ULONG)(n),MEMF_ANY|MEMF_CLEAR)
#define INDEX_FREE(p) FreeVec(p)
#endif

static int fold(int c)
{ return (c>='A' && c<='Z')?c+32:c; }

static unsigned long hash_path(const char *s)
{
    unsigned long h=5381UL;
    while (*s) h=((h<<5)+h)+(unsigned long)fold((unsigned char)*s++);
    return h;
}

static int same_path(const char *a,const char *b)
{
    while (*a && *b)
        if (fold((unsigned char)*a++)!=fold((unsigned char)*b++)) return 0;
    return !*a && !*b;
}

const char *beam_index_name_for(const char *path)
{
    const char *name=path,*p;
    if (!path) return "";
    for (p=path;*p;p++) if (*p=='/' || *p==':') name=p+1;
    return *name?name:path; /* "RAM:" -> "RAM:" */
}

void beam_index_free(struct BeamIndex *x)
{
    if (!x) return;
    if (x->items) INDEX_FREE(x->items);
    if (x->pool) INDEX_FREE(x->pool);
    if (x->hash) INDEX_FREE(x->hash);
    memset(x,0,sizeof(*x));
}

int beam_index_init(struct BeamIndex *x,int capacity)
{
    unsigned int size=16;
    memset(x,0,sizeof(*x));
    if (capacity<1 || capacity>65000) return 0;
    while (size<(unsigned int)capacity*2U) size<<=1;
    x->items=(struct BeamItem *)INDEX_ALLOC(sizeof(struct BeamItem)*(unsigned long)capacity);
    x->pool_size=(unsigned long)capacity*BEAM_POOL_BYTES_PER_ITEM;
    x->pool=(char *)INDEX_ALLOC(x->pool_size);
    x->hash=(unsigned short *)INDEX_ALLOC(sizeof(unsigned short)*(unsigned long)size);
    if (!x->items || !x->pool || !x->hash) { beam_index_free(x); return 0; }
    x->capacity=capacity;
    x->hash_mask=size-1U;
    return 1;
}

void beam_index_clear(struct BeamIndex *x)
{
    x->count=0; x->full=0; x->pool_used=0;
    memset(x->hash,0,sizeof(unsigned short)*(x->hash_mask+1U));
}

static unsigned int slot_for(const struct BeamIndex *x,const char *path,int *found)
{
    unsigned int slot=(unsigned int)(hash_path(path)&x->hash_mask);
    *found=-1;
    while (x->hash[slot]) {
        int i=(int)x->hash[slot]-1;
        if (same_path(x->items[i].path,path)) { *found=i; break; }
        slot=(slot+1U)&x->hash_mask;
    }
    return slot;
}

int beam_index_find(const struct BeamIndex *x,const char *path)
{
    int found;
    if (!x || !x->hash || !path) return -1;
    slot_for(x,path,&found);
    return found;
}

int beam_index_add(struct BeamIndex *x,const char *path,int kind,int root,
                   int *created)
{
    unsigned int slot;
    unsigned long n;
    int found;
    struct BeamItem *it;
    if (created) *created=0;
    if (!x || !path || !*path) return -2;
    n=(unsigned long)strlen(path)+1UL;
    if (n>BEAM_MAX_PATH || strlen(beam_index_name_for(path))>=BEAM_MAX_NAME) return -2;
    slot=slot_for(x,path,&found);
    if (found>=0) {
        it=&x->items[found];
        /* A Workbench icon outranks a plain file/executable for the same
           path; an executable outranks a plain file. */
        if (kind==BEAM_ITEM_GAME ||
            (kind==BEAM_ITEM_WORKBENCH && it->kind!=BEAM_ITEM_GAME) ||
            (kind==BEAM_ITEM_SHELL && it->kind==BEAM_ITEM_FILE))
            it->kind=(unsigned char)kind;
        it->flags&=(unsigned char)~BEAM_ITEM_FLAG_STALE;
        return found;
    }
    if (x->count>=x->capacity || x->pool_used+n>x->pool_size) { x->full=1; return -1; }
    it=&x->items[x->count];
    memset(it,0,sizeof(*it));
    it->path=x->pool+x->pool_used;
    memcpy(it->path,path,(size_t)n);
    x->pool_used+=n;
    it->name=beam_index_name_for(it->path);
    it->kind=(unsigned char)kind;
    it->root=(unsigned char)root;
    it->favorite=BEAM_ITEM_META_NONE;
    it->recent=BEAM_ITEM_META_NONE;
    it->alias_index=-1;
    x->hash[slot]=(unsigned short)(x->count+1);
    ++x->count;
    if (created) *created=1;
    return x->count-1;
}

int beam_index_set_title(struct BeamIndex *x,int index,const char *title)
{
    struct BeamItem *it;
    unsigned long n;
    if (!x || index<0 || index>=x->count || !title || !*title) return 0;
    it=&x->items[index];
    /* Sweep moves path+title as one block, so they must be adjacent. */
    if (it->title || x->pool+x->pool_used!=it->path+strlen(it->path)+1) return 0;
    n=(unsigned long)strlen(title)+1UL;
    if (n>BEAM_MAX_PATH || x->pool_used+n>x->pool_size) return 0;
    memcpy(x->pool+x->pool_used,title,(size_t)n);
    it->title=x->pool+x->pool_used;
    x->pool_used+=n;
    /* A bookmark is found and shown by its title, not its URL. */
    if (it->kind==BEAM_ITEM_BOOKMARK) it->name=it->title;
    return 1;
}

void beam_index_mark_stale(struct BeamIndex *x,unsigned long root_mask,
                           int include_volumes)
{
    int i;
    for (i=0;i<x->count;i++) {
        struct BeamItem *it=&x->items[i];
        if (it->root==BEAM_ROOT_NONE) {
            if (include_volumes) it->flags|=BEAM_ITEM_FLAG_STALE;
        } else if (it->root<32 && (root_mask&(1UL<<it->root)))
            it->flags|=BEAM_ITEM_FLAG_STALE;
    }
}

int beam_index_sweep(struct BeamIndex *x)
{
    int i,out=0,removed=0;
    unsigned long used=0;
    /* Items and their strings were appended in the same order, so live
       strings only ever move towards the start of the pool: an in-place,
       order-preserving memmove compaction is safe. */
    for (i=0;i<x->count;i++) {
        struct BeamItem it=x->items[i];
        unsigned long n,name_offset;
        if (it.flags&BEAM_ITEM_FLAG_STALE) { ++removed; continue; }
        n=(unsigned long)strlen(it.path)+1UL;
        if (it.title) n+=(unsigned long)strlen(it.title)+1UL; /* adjacent */
        name_offset=(unsigned long)(it.name-it.path);
        if (it.title) {
            unsigned long title_offset=(unsigned long)(it.title-it.path);
            if (x->pool+used!=it.path) memmove(x->pool+used,it.path,(size_t)n);
            it.title=x->pool+used+title_offset;
        } else if (x->pool+used!=it.path) memmove(x->pool+used,it.path,(size_t)n);
        it.path=x->pool+used;
        it.name=it.path+name_offset;
        used+=n;
        x->items[out++]=it;
    }
    x->count=out;
    x->pool_used=used;
    x->full=0;
    memset(x->hash,0,sizeof(unsigned short)*(x->hash_mask+1U));
    for (i=0;i<x->count;i++) {
        int found;
        unsigned int slot=slot_for(x,x->items[i].path,&found);
        x->hash[slot]=(unsigned short)(i+1);
    }
    return removed;
}
