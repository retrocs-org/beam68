/* Heap-backed depth-first traversal: no recursive Shell-stack growth. */
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "plugin_walk.h"
#include "plugin_common.h"
struct BeamWalkFrame {
    struct BeamWalkFrame *parent;
    BPTR lock;struct FileInfoBlock *fib;
    int depth,root;char path[BEAM_MAX_PATH];
};
static int excluded(struct BeamWalk *w,const char *path)
{
    int i;unsigned int n;
    for(i=0;i<w->config->excluded_dir_count;i++) {
        char resolved[BEAM_MAX_PATH];const char *p=w->config->excluded_dirs[i];
        if(beam_ascii_prefix(p,"PROGDIR:") && w->config->install_dir[0] &&
            !beam_ascii_prefix(w->config->install_dir,"PROGDIR:")) {
            beam_plugin_copy(resolved,w->config->install_dir,sizeof(resolved));
            if(!AddPart(resolved,(STRPTR)(p+8),sizeof(resolved)))continue;
            p=resolved;
        }
        n=strlen(p);
        if(n && beam_ascii_prefix(path,p) &&
            (p[n-1]==':' || p[n-1]=='/' || !path[n] || path[n]=='/'))return 1;
    }
    return 0;
}
static int push(struct BeamWalk *w,const char *path,int depth,int root)
{
    struct BeamWalkFrame *f;char resolved[BEAM_MAX_PATH];
    if(beam_ascii_prefix(path,"PROGDIR:") && w->config->install_dir[0] &&
        !beam_ascii_prefix(w->config->install_dir,"PROGDIR:")) {
        beam_plugin_copy(resolved,w->config->install_dir,sizeof(resolved));
        if(!AddPart(resolved,(STRPTR)(path+8),sizeof(resolved)))return 0;
        path=resolved;
    }
    if(excluded(w,path))return 1;
    f=AllocVec(sizeof(*f),MEMF_ANY|MEMF_CLEAR);if(!f)return 0;
    f->lock=Lock((STRPTR)path,ACCESS_READ);
    if(f->lock)f->fib=AllocDosObject(DOS_FIB,NULL);
    if(!f->fib || !Examine(f->lock,f->fib)) {
        if(f->fib)FreeDosObject(DOS_FIB,f->fib);
        if(f->lock)UnLock(f->lock);
        FreeVec(f);return 0;
    }
    beam_plugin_copy(f->path,path,sizeof(f->path));f->depth=depth;f->root=root;
    f->parent=w->top;w->top=f;return 1;
}
static void pop(struct BeamWalk *w)
{
    struct BeamWalkFrame *f=w->top;w->top=f->parent;
    FreeDosObject(DOS_FIB,f->fib);UnLock(f->lock);FreeVec(f);
}
void beam_walk_begin(struct BeamWalk *w,const struct BeamConfig *c,const char *override)
{memset(w,0,sizeof(*w));w->config=c;w->override=override;}
void beam_walk_end(struct BeamWalk *w){while(w->top)pop(w);}
int beam_walk_next(struct BeamWalk *w,char *path,unsigned int cap,int *kind,int *root)
{
    while(1) {
        struct BeamWalkFrame *f=w->top;
        if(!f) {
            const char *dir;
            if(w->override && *w->override) {
                if(w->root)return 0;
                dir=w->override;
            } else {if(w->root>=w->config->dir_count)return 0;dir=w->config->dirs[w->root];}
            if(!push(w,dir,0,w->root))w->unreadable++;
            w->root++;continue;
        }
        if(!ExNext(f->lock,f->fib)){if(IoErr()!=ERROR_NO_MORE_ENTRIES)w->unreadable++;pop(w);continue;}
        if(!strcmp(f->fib->fib_FileName,".") || !strcmp(f->fib->fib_FileName,".."))continue;
        beam_plugin_copy(path,f->path,cap);
        if(!AddPart(path,f->fib->fib_FileName,cap)){w->unreadable++;continue;}
        if(excluded(w,path))continue;
        *kind=f->fib->fib_DirEntryType;*root=f->root;
        if(*kind==ST_USERDIR && (w->config->max_depth<0 || f->depth<w->config->max_depth))
            if(!push(w,path,f->depth+1,f->root))w->unreadable++;
        return 1;
    }
}
