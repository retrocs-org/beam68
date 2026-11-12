/* Plain ASCII launch counts, separate from user-editable configuration.
 * The launcher writes every fourth successful launch and on clean exit.
 * No timer, background scan, large allocation, or per-query filesystem IO.
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "history.h"

static int same_path(const char *a,const char *b)
{
    int ca,cb;
    while (*a && *b) {
        ca=(unsigned char)*a++; cb=(unsigned char)*b++;
        if (ca>='A' && ca<='Z') ca+=32;
        if (cb>='A' && cb<='Z') cb+=32;
        if (ca!=cb) return 0;
    }
    return !*a && !*b;
}
static int find_entry(const struct BeamHistory *h,const char *path)
{
    unsigned int i;
    for(i=0;i<h->count;i++)
        if (same_path(h->entries[i].path,path)) return (int)i;
    return -1;
}
void beam_history_reset(struct BeamHistory *h)
{ memset(h,0,sizeof(*h)); }
unsigned int beam_history_bonus(const struct BeamHistory *h,const char *path)
{
    int i=find_entry(h,path);
    unsigned long n;
    if (i<0) return 0;
    n=h->entries[i].launches;
    /* Exact and prefix name matches outrank frequency: this is only a tie-break. */
    return (unsigned int)((n>24?24:n)*6);
}
int beam_history_recent_position(const struct BeamHistory *h,const char *path)
{
    int i=find_entry(h,path);
    return i<0?-1:(int)h->count-1-i;
}
static int valid_path(const char *path)
{
    size_t n;
    if (!path || !(n=strlen(path)) || n>=sizeof(((struct BeamHistoryEntry*)0)->path)) return 0;
    return !strchr(path,'\t') && !strchr(path,'\n') && !strchr(path,'\r');
}
void beam_history_record(struct BeamHistory *h,const char *path)
{
    int i;
    unsigned int j;
    struct BeamHistoryEntry recent;
    if (!valid_path(path)) return;
    i=find_entry(h,path);
    if (i<0) {
        if (h->count<BEAM_HISTORY_CAP) i=(int)h->count++;
        else {
            /* Replace the least used entry, without unbounded growth. */
            i=0;
            for(j=1;j<h->count;j++)
                if (h->entries[j].launches<h->entries[i].launches) i=(int)j;
        }
        strcpy(h->entries[i].path,path);
        h->entries[i].launches=0;
    }
    if (h->entries[i].launches<65535UL) ++h->entries[i].launches;
    /* File order is also the bounded recency order.  Keep the newest item at
       the end without adding timestamps or another on-disk format. */
    if (i<(int)h->count-1) {
        recent=h->entries[i];
        for(j=(unsigned int)i;j+1<h->count;j++) h->entries[j]=h->entries[j+1];
        h->entries[h->count-1]=recent;
    }
    if (h->pending<65535U) ++h->pending;
}
int beam_history_load(struct BeamHistory *h,const char *path)
{
    BPTR f;
    char line[512],*sep,*end;
    unsigned long count;
    unsigned int n;
    beam_history_reset(h);
    f=Open((STRPTR)path,MODE_OLDFILE);
    if (!f) return 0;
    while(FGets(f,line,sizeof(line))) {
        n=(unsigned int)strlen(line);
        /* Reject truncated/oversized lines. */
        if (n && line[n-1]!='\n' && n+1>=sizeof(line)) {
            char ch;
            while(FRead(f,&ch,1,1)==1 && ch!='\n') { }
            continue;
        }
        sep=strchr(line,'\t');
        if (!sep) continue;
        *sep++=0;
        end=sep+strlen(sep);
        while(end>sep && (end[-1]=='\n' || end[-1]=='\r')) *--end=0;
        if (!valid_path(sep)) continue;
        /* Count must be canonical decimal and bounded; no overflow. */
        if (!line[0]) continue;
        count=strtoul(line,&end,10);
        if (*end || count<1 || count>65535UL) continue;
        if (find_entry(h,sep)>=0) continue;
        if (h->count<BEAM_HISTORY_CAP) {
            struct BeamHistoryEntry *e=&h->entries[h->count++];
            strcpy(e->path,sep); e->launches=count;
        }
    }
    Close(f);
    return 1;
}
int beam_history_save(struct BeamHistory *h,const char *path)
{
    char newfile[256],bak[256],line[280];
    BPTR f,dir,old;
    unsigned int i;
    int ok=1,had_old;
    if (!path || strlen(path)>240 || !h->pending) return 1;
    if (!strcmp(path,BEAM_HISTORY_PATH)) {
        dir=CreateDir((STRPTR)"ENVARC:Beam68"); if (dir) UnLock(dir);
    }
    sprintf(newfile,"%s.new",path);
    sprintf(bak,"%s.bak",path);
    f=Open((STRPTR)newfile,MODE_NEWFILE);
    if (!f) return 0;
    for (i=0;i<h->count;i++) {
        sprintf(line,"%lu\t%s\n",h->entries[i].launches,h->entries[i].path);
        if (FPuts(f,line)<0) { ok=0; break; }
    }
    if (Close(f)==0) ok=0;
    if (!ok) { DeleteFile((STRPTR)newfile); return 0; }
    old=Lock((STRPTR)path,ACCESS_READ);
    had_old=old!=0;
    if (old) UnLock(old);
    if (had_old) {
        DeleteFile((STRPTR)bak);
        if (!Rename((STRPTR)path,(STRPTR)bak)) {
            DeleteFile((STRPTR)newfile); return 0;
        }
    }
    if (!Rename((STRPTR)newfile,(STRPTR)path)) {
        if (had_old) Rename((STRPTR)bak,(STRPTR)path);
        DeleteFile((STRPTR)newfile); return 0;
    }
    h->pending=0;
    return 1;
}
