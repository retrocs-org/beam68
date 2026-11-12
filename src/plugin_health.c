#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_health.h"

static int equal_path(const char *a,const char *b)
{
    int ca,cb;
    while(*a&&*b){
        ca=(unsigned char)*a++;cb=(unsigned char)*b++;
        if(ca>='A'&&ca<='Z')ca+=32;
        if(cb>='A'&&cb<='Z')cb+=32;
        if(ca!=cb)return 0;
    }
    return !*a&&!*b;
}
static int valid_path(const char *p)
{ return p && *p && strlen(p)<BEAM_MAX_PATH && !strchr(p,'\t') && !strchr(p,'\n') && !strchr(p,'\r'); }
void beam_plugin_health_reset(struct BeamPluginHealth *h){memset(h,0,sizeof(*h));}
const struct BeamPluginHealthEntry *beam_plugin_health_find(const struct BeamPluginHealth *h,const char *path)
{ unsigned int i;if(!h||!path)return NULL;for(i=0;i<h->count;i++)if(equal_path(h->entries[i].path,path))return &h->entries[i];return NULL; }
void beam_plugin_health_record(struct BeamPluginHealth *h,const char *path,int outcome)
{
    unsigned int i; struct BeamPluginHealthEntry *e=NULL;
    if(!h||!valid_path(path)||outcome<1||outcome>3)return;
    for(i=0;i<h->count;i++)if(equal_path(h->entries[i].path,path)){e=&h->entries[i];break;}
    if(!e){if(h->count>=BEAM_MAX_PLUGINS)return;e=&h->entries[h->count++];memset(e,0,sizeof(*e));strcpy(e->path,path);}
    if(outcome==BEAM_PLUGIN_HEALTH_SUCCESS){if(e->successes<65535UL)++e->successes;e->consecutive_failures=0;e->quarantined=0;}
    else if(outcome==BEAM_PLUGIN_HEALTH_ERROR){if(e->errors<65535UL)++e->errors;if(e->consecutive_failures<255)++e->consecutive_failures;}
    else if(outcome==BEAM_PLUGIN_HEALTH_TIMEOUT){if(e->timeouts<65535UL)++e->timeouts;if(e->consecutive_failures<255)++e->consecutive_failures;}
    if(e->consecutive_failures>=BEAM_PLUGIN_QUARANTINE_THRESHOLD)e->quarantined=1;
    e->last=(unsigned char)outcome;if(h->pending<65535U)++h->pending;
}
int beam_plugin_health_is_quarantined(const struct BeamPluginHealth *h,const char *path)
{ const struct BeamPluginHealthEntry *e=beam_plugin_health_find(h,path);return e && e->quarantined; }
int beam_plugin_health_clear_quarantine(struct BeamPluginHealth *h,const char *path)
{
    unsigned int i;if(!h||!path)return 0;
    for(i=0;i<h->count;i++)if(equal_path(h->entries[i].path,path)){
        h->entries[i].quarantined=0;h->entries[i].consecutive_failures=0;
        if(h->pending<65535U)++h->pending;
        return 1;
    }
    return 0;
}
int beam_plugin_health_format(const struct BeamPluginHealthEntry *e,char *out,unsigned int cap)
{
    char line[160];const char *last;
    if(!out||!cap)return 0;
    out[0]=0;
    if(!e){if(cap<20)return 0;strcpy(out,"No recorded launches");return 1;}
    last=e->last==BEAM_PLUGIN_HEALTH_SUCCESS?"OK":e->last==BEAM_PLUGIN_HEALTH_TIMEOUT?"TIMEOUT":"ERROR";
    sprintf(line,"OK %lu | Errors %lu | Timeouts %lu | Last %s | Streak %u%s",e->successes,e->errors,e->timeouts,last,(unsigned)e->consecutive_failures,e->quarantined?" | QUARANTINED":"");
    if(strlen(line)+1>cap)return 0;
    strcpy(out,line);return 1;
}
static int number(const char *s,unsigned long limit,unsigned long *out)
{ char *end;unsigned long v=strtoul(s,&end,10);if(*end||v>limit)return 0;*out=v;return 1; }
/* Health written by 0.1 uses the flat PROGDIR:Plugins/<Name>.plugin
 * layout; rewrite it to the package drawer so counters and quarantine
 * survive the move (same rule as beam_plugin_path_migrate in config.c). */
static void migrate_key(char *key)
{
    char name[64],out[BEAM_MAX_PATH];
    const char *rest;
    unsigned int n,i;
    static const char prefix[]="PROGDIR:Plugins/";
    for (i=0;prefix[i];i++) {
        int a=(unsigned char)key[i],b=(unsigned char)prefix[i];
        if (a>='a'&&a<='z') a-=32;
        if (b>='a'&&b<='z') b-=32;
        if (a!=b) return;
    }
    rest=key+16;
    if (strchr(rest,'/') || strchr(rest,':')) return;
    n=(unsigned int)strlen(rest);
    if (n<=7 || n-7>=sizeof(name)) return;
    for (i=0;i<7;i++) {
        int c=(unsigned char)rest[n-7+i];
        if (c>='A'&&c<='Z') c+=32;
        if (c!=".plugin"[i]) return;
    }
    memcpy(name,rest,n-7); name[n-7]=0;
    if (16+2*n+2>=sizeof(out)) return;
    sprintf(out,"PROGDIR:Plugins/%s/%s",name,rest);
    strcpy(key,out);
}

int beam_plugin_health_load(struct BeamPluginHealth *h,const char *path)
{
    BPTR f;char line[512],*parts[7],*p,*end;unsigned long ok,err,to,last,streak=0,quarantine=0;int count,i;
    beam_plugin_health_reset(h);f=Open((STRPTR)path,MODE_OLDFILE);if(!f)return 0;
    while(FGets(f,line,sizeof(line))&&h->count<BEAM_MAX_PLUGINS){
        end=line+strlen(line);while(end>line&&(end[-1]=='\n'||end[-1]=='\r'))*--end=0;
        p=line;count=0;while(count<7){parts[count++]=p;p=strchr(p,'\t');if(!p)break;*p++=0;}
        if(count!=5&&count!=7)continue;
        if(!number(parts[0],65535UL,&ok)||!number(parts[1],65535UL,&err)||
           !number(parts[2],65535UL,&to)||!number(parts[3],3UL,&last)||last<1)continue;
        if(count==7){if(!number(parts[4],255UL,&streak)||!number(parts[5],1UL,&quarantine))continue;i=6;}
        else{i=4;streak=0;quarantine=0;}
        if(!valid_path(parts[i]))continue;
        strcpy(h->entries[h->count].path,parts[i]);
        migrate_key(h->entries[h->count].path);
        h->entries[h->count].successes=ok;
        h->entries[h->count].errors=err;h->entries[h->count].timeouts=to;
        h->entries[h->count].last=(unsigned char)last;
        h->entries[h->count].consecutive_failures=(unsigned char)streak;
        h->entries[h->count].quarantined=(unsigned char)quarantine;++h->count;
    }
    Close(f);return 1;
}
int beam_plugin_health_save(struct BeamPluginHealth *h,const char *path)
{
    char tmp[256],bak[256],line[512];BPTR f,dir,old;unsigned int i;int had_old,ok=1;
    if(!h||!path||strlen(path)>240||!h->pending)return 1;
    if(!strcmp(path,BEAM_PLUGIN_HEALTH_PATH)){dir=CreateDir((STRPTR)"ENVARC:Beam68");if(dir)UnLock(dir);}
    sprintf(tmp,"%s.new",path);sprintf(bak,"%s.bak",path);f=Open((STRPTR)tmp,MODE_NEWFILE);if(!f)return 0;
    for(i=0;i<h->count;i++){struct BeamPluginHealthEntry *e=&h->entries[i];sprintf(line,"%lu\t%lu\t%lu\t%u\t%u\t%u\t%s\n",e->successes,e->errors,e->timeouts,(unsigned)e->last,(unsigned)e->consecutive_failures,(unsigned)e->quarantined,e->path);if(FPuts(f,line)<0){ok=0;break;}}
    if(Close(f)==0)ok=0;
    if(!ok){DeleteFile((STRPTR)tmp);return 0;}
    old=Lock((STRPTR)path,ACCESS_READ);had_old=old!=0;
    if(old)UnLock(old);
    if(had_old){DeleteFile((STRPTR)bak);if(!Rename((STRPTR)path,(STRPTR)bak)){DeleteFile((STRPTR)tmp);return 0;}}
    if(!Rename((STRPTR)tmp,(STRPTR)path)){
        if(had_old)Rename((STRPTR)bak,(STRPTR)path);
        DeleteFile((STRPTR)tmp);return 0;
    }
    h->pending=0;return 1;
}
