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

static int starts(const char *s,const char *prefix)
{
    while(*prefix){unsigned char a=(unsigned char)*s++,b=(unsigned char)*prefix++;
        if(a>='A'&&a<='Z')a+=32;
        if(b>='A'&&b<='Z')b+=32;
        if(a!=b)return 0;
    }return 1;
}
static int verb(const char *s,const char *v)
{return starts(s,v) && (!s[strlen(v)] || s[strlen(v)]==' ' || s[strlen(v)]=='\t');}
int beam_plugin_query_readonly(const char *id,const char *s)
{
    if(!id||!s||!valid_query(s))return 0;
    while(*s==' '||*s=='\t')++s;
    if(!*s)return 0;
    if(!strcmp(id,"org.beam68.amihass"))return !verb(s,"do");
    if(!strcmp(id,"org.beam68.wikipedia"))return 1;
    if(!strcmp(id,"org.beam68.aminetbrowser"))return verb(s,"search")||verb(s,"page")||verb(s,"info")||verb(s,"read")||verb(s,"help");
    if(!strcmp(id,"org.beam68.musiclibrary")||!strcmp(id,"org.beam68.whdload"))return !verb(s,"play");
    if(!strcmp(id,"org.beam68.dateutilities")||!strcmp(id,"org.beam68.developertools"))return 1;
    if(!strcmp(id,"org.beam68.taskmanager"))return verb(s,"list")||verb(s,"tasks");
    if(!strcmp(id,"org.beam68.clipboardtools"))return verb(s,"show");
    if(!strcmp(id,"org.beam68.dostools"))return verb(s,"volumes")||verb(s,"assigns")||verb(s,"diskfree");
    if(!strcmp(id,"org.beam68.systemcommands"))return verb(s,"avail")||verb(s,"version");
    if(!strcmp(id,"org.beam68.windowswitcher"))return verb(s,"list")||verb(s,"windows");
    if(!strcmp(id,"org.beam68.arexxrunner"))return verb(s,"list");
    if(!strcmp(id,"org.beam68.musiccontrols"))return verb(s,"players");
    return !strcmp(id,"org.beam68.calc")||!strcmp(id,"org.beam68.clock")||!strcmp(id,"org.beam68.memory")||!strcmp(id,"org.beam68.filefinder");
}
int beam_plugin_query_scope(const char *query,char *id,unsigned int cap,const char **input)
{
    static const char *prefixes[]={"ha:","wiki:","aminet:","mod:","=","@","?","find:","date:","dev:","task:","clip:","dos:","system:","window:","rexx:","music:","whd:","dopus:","[","\\","]",";","/",":","$","^","&","#","!","~","%","|","{","}",NULL};
    static const char *ids[]={"amihass","wikipedia","aminetbrowser","musiclibrary","calc","clock","memory","filefinder","dateutilities","developertools","taskmanager","clipboardtools","dostools","systemcommands","windowswitcher","arexxrunner","musiccontrols","whdload","dopus","amihass","wikipedia","aminetbrowser","musiclibrary","filefinder","dateutilities","developertools","taskmanager","clipboardtools","dostools","systemcommands","windowswitcher","arexxrunner","musiccontrols","whdload","dopus"};
    unsigned int i;char owner[64];const char *s;
    if(!query || !id || !cap || !valid_query(query))return 0;
    for(i=0;prefixes[i];++i)if(starts(query,prefixes[i])){
        s=query+strlen(prefixes[i]);sprintf(owner,"org.beam68.%s",ids[i]);
        if(strlen(owner)+1>cap)return 0;
        strcpy(id,owner);if(input)*input=s;return 1;
    }
    return 0;
}
int beam_plugin_query_parse(const char *query,char *id,unsigned int cap,const char **input)
{
    char owner[64];const char *text;
    if(!id || !beam_plugin_query_scope(query,owner,sizeof(owner),&text) ||
       !beam_plugin_query_readonly(owner,text) || strlen(owner)+1>cap)return 0;
    strcpy(id,owner);if(input)*input=text;return 1;
}
const char *beam_query_history_recall(struct BeamQueryHistory *h,int newer,int skip_plugins)
{
    const char *s;char owner[64];unsigned int i;
    if(!h)return NULL;
    for(i=0;i<=h->count;++i){
        s=newer?beam_query_history_newer(h):beam_query_history_older(h);
        if(!s || !*s || !skip_plugins || !beam_plugin_query_scope(s,owner,sizeof(owner),NULL))return s;
        if(!newer && h->cursor==0)return NULL;
    }return NULL;
}
int beam_plugin_history_path(const char *id,char *out,unsigned int cap)
{
    const char *p;unsigned int n;
    if(!id||!*id||strlen(id)>63)return 0;
    for(p=id;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'||*p=='_'))return 0;
    if(!strcmp(id,".")||!strcmp(id,".."))return 0;
    n=(unsigned int)strlen(id)+strlen("ENVARC:Beam68/PluginQueries/.txt")+1;
    if(n>cap)return 0;
    sprintf(out,"ENVARC:Beam68/PluginQueries/%s.txt",id);return 1;
}
