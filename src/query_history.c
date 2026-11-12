#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "query_history.h"

static int valid_query(const char *s)
{
    size_t n;
    if (!s || !(n=strlen(s)) || n>BEAM_QUERY_LEN) return 0;
    return !strchr(s,'\n') && !strchr(s,'\r');
}
void beam_query_history_reset(struct BeamQueryHistory *h)
{ memset(h,0,sizeof(*h)); }
void beam_query_history_end(struct BeamQueryHistory *h)
{ if (h) h->cursor=h->count; }
void beam_query_history_record(struct BeamQueryHistory *h,const char *query)
{
    int i=-1;
    unsigned int j;
    char saved[BEAM_QUERY_LEN+1];
    if (!h || !valid_query(query)) return;
    for(j=0;j<h->count;j++) if (!strcmp(h->entries[j],query)) { i=(int)j;break; }
    if (i>=0) {
        strcpy(saved,h->entries[i]);
        for(j=(unsigned int)i;j+1<h->count;j++) strcpy(h->entries[j],h->entries[j+1]);
        strcpy(h->entries[h->count-1],saved);
    } else {
        if (h->count<BEAM_QUERY_HISTORY_CAP) i=(int)h->count++;
        else {
            for(j=0;j+1<h->count;j++) strcpy(h->entries[j],h->entries[j+1]);
            i=(int)h->count-1;
        }
        strcpy(h->entries[i],query);
    }
    if (h->pending<65535U) ++h->pending;
    h->cursor=h->count;
}
const char *beam_query_history_older(struct BeamQueryHistory *h)
{
    if (!h || !h->count) return NULL;
    if (h->cursor>h->count) h->cursor=h->count;
    if (h->cursor) --h->cursor;
    return h->entries[h->cursor];
}
const char *beam_query_history_newer(struct BeamQueryHistory *h)
{
    if (!h || !h->count || h->cursor>=h->count) return NULL;
    ++h->cursor;
    return h->cursor<h->count?h->entries[h->cursor]:"";
}
int beam_query_history_load(struct BeamQueryHistory *h,const char *path)
{
    BPTR f; char line[BEAM_QUERY_LEN+4],*end;
    beam_query_history_reset(h);
    f=Open((STRPTR)path,MODE_OLDFILE); if (!f) return 0;
    while(FGets(f,line,sizeof(line))) {
        end=line+strlen(line);
        while(end>line && (end[-1]=='\n' || end[-1]=='\r')) *--end=0;
        if (valid_query(line)) beam_query_history_record(h,line);
    }
    Close(f); h->pending=0; h->cursor=h->count; return 1;
}
int beam_query_history_save(struct BeamQueryHistory *h,const char *path)
{
    char tmp[256],bak[256]; BPTR f,dir,old; unsigned int i; int had_old,ok=1;
    if (!h || !path || strlen(path)>240 || !h->pending) return 1;
    if (!strcmp(path,BEAM_QUERY_HISTORY_PATH)) {
        dir=CreateDir((STRPTR)"ENVARC:Beam68"); if (dir) UnLock(dir);
    }
    sprintf(tmp,"%s.new",path); sprintf(bak,"%s.bak",path);
    f=Open((STRPTR)tmp,MODE_NEWFILE); if (!f) return 0;
    for(i=0;i<h->count;i++) if (FPuts(f,h->entries[i])<0 || FPuts(f,"\n")<0) {ok=0;break;}
    if (Close(f)==0) ok=0;
    if (!ok) {DeleteFile((STRPTR)tmp);return 0;}
    old=Lock((STRPTR)path,ACCESS_READ); had_old=old!=0; if (old) UnLock(old);
    if (had_old) {DeleteFile((STRPTR)bak);if (!Rename((STRPTR)path,(STRPTR)bak)){DeleteFile((STRPTR)tmp);return 0;}}
    if (!Rename((STRPTR)tmp,(STRPTR)path)) {if(had_old)Rename((STRPTR)bak,(STRPTR)path);DeleteFile((STRPTR)tmp);return 0;}
    h->pending=0; return 1;
}
