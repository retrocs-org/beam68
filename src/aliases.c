/* Persistent per-application search aliases. The tab-separated text format
 * is deliberately editable with AmigaOS Ed and bounded for 68000 systems. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "aliases.h"

static int safe_path(const char *s)
{
    unsigned int n;
    if (!s || !(n=(unsigned int)strlen(s)) || n>=BEAM_MAX_PATH) return 0;
    return strchr(s,':') && !strpbrk(s,"\r\n\t\"*");
}

static int safe_text(const char *s,unsigned int cap,int allow_empty)
{
    unsigned int n;
    if (!s || (n=(unsigned int)strlen(s))>=cap || (!allow_empty && !n)) return 0;
    return !strpbrk(s,"\r\n\t");
}

#ifdef BEAM_HOST
#include <stdlib.h>
#define ALIAS_ALLOC(n) calloc(1,(size_t)(n))
#define ALIAS_FREE(p) free(p)
#else
#include <exec/memory.h>
#include <proto/exec.h>
#define ALIAS_ALLOC(n) AllocVec((ULONG)(n),MEMF_ANY|MEMF_CLEAR)
#define ALIAS_FREE(p) FreeVec(p)
#endif

/* Keeps the allocation: a reload usually refills the same number. */
void beam_aliases_clear(struct BeamAliases *a)
{
    if (a->entries) memset(a->entries,0,sizeof(struct BeamAlias)*a->capacity);
    a->count=0;
}

void beam_aliases_free(struct BeamAliases *a)
{
    if (a->entries) ALIAS_FREE(a->entries);
    a->entries=NULL; a->count=0; a->capacity=0;
}

static int grow(struct BeamAliases *a)
{
    struct BeamAlias *bigger;
    unsigned int capacity=a->capacity?a->capacity*2U:8U;
    if (a->capacity>=BEAM_ALIAS_CAP) return 0;
    if (capacity>BEAM_ALIAS_CAP) capacity=BEAM_ALIAS_CAP;
    bigger=(struct BeamAlias *)ALIAS_ALLOC(sizeof(struct BeamAlias)*capacity);
    if (!bigger) return 0;
    if (a->entries) {
        memcpy(bigger,a->entries,sizeof(struct BeamAlias)*a->count);
        ALIAS_FREE(a->entries);
    }
    a->entries=bigger;
    a->capacity=(unsigned short)capacity;
    return 1;
}

int beam_aliases_find(const struct BeamAliases *a,const char *path)
{
    unsigned int i;
    if (!a || !path) return -1;
    for (i=0;i<a->count;i++)
        if (beam_equal(a->entries[i].path,path)) return (int)i;
    return -1;
}

const char *beam_aliases_words(const struct BeamAliases *a,int index)
{
    if (!a || index<0 || index>=(int)a->count) return NULL;
    return a->entries[index].words;
}

int beam_aliases_set(struct BeamAliases *a,const char *path,const char *words)
{
    return beam_aliases_set_full(a,path,words,"","");
}

int beam_aliases_set_full(struct BeamAliases *a,const char *path,
                          const char *words,const char *action_label,
                          const char *action_command)
{
    int i;
    int has_label=action_label && *action_label;
    int has_command=action_command && *action_command;
    if (!a || !safe_path(path) ||
        !safe_text(words,BEAM_ALIAS_LEN,1) ||
        !safe_text(action_label,BEAM_ALIAS_ACTION_LABEL_LEN,1) ||
        !safe_text(action_command,BEAM_ALIAS_ACTION_COMMAND_LEN,1) ||
        has_label!=has_command || (!*words && !has_command)) return 0;
    i=beam_aliases_find(a,path);
    if (i<0) {
        if (a->count>=a->capacity && !grow(a)) return 0;
        i=(int)a->count++;
        beam_copy(a->entries[i].path,path,sizeof(a->entries[i].path));
    }
    beam_copy(a->entries[i].words,words,sizeof(a->entries[i].words));
    beam_copy(a->entries[i].action_label,action_label,
              sizeof(a->entries[i].action_label));
    beam_copy(a->entries[i].action_command,action_command,
              sizeof(a->entries[i].action_command));
    return 1;
}

int beam_aliases_remove(struct BeamAliases *a,int index)
{
    int i;
    if (!a || index<0 || index>=(int)a->count) return 0;
    for (i=index;i<(int)a->count-1;i++) a->entries[i]=a->entries[i+1];
    --a->count;
    memset(&a->entries[a->count],0,sizeof(a->entries[a->count]));
    return 1;
}

const char *beam_alias_action_label(const struct BeamAliases *a,int index)
{
    if (!a || index<0 || index>=(int)a->count ||
        !a->entries[index].action_label[0] ||
        !a->entries[index].action_command[0]) return NULL;
    return a->entries[index].action_label;
}

const char *beam_alias_action_command(const struct BeamAliases *a,int index)
{
    if (!beam_alias_action_label(a,index)) return NULL;
    return a->entries[index].action_command;
}

int beam_aliases_load(struct BeamAliases *a,const char *filename)
{
    BPTR in;
    char line[BEAM_MAX_PATH+BEAM_ALIAS_LEN+8];
    beam_aliases_clear(a);
    in=Open((STRPTR)filename,MODE_OLDFILE);
    if (!in) return 0;
    while (FGets(in,line,sizeof(line))) {
        char empty_label[1]={0},empty_command[1]={0};
        char *tab,*tab2,*tab3,*path,*words,*label=empty_label,*command=empty_command;
        unsigned int n=(unsigned int)strlen(line);
        if (!n) continue;
        if (line[n-1]!='\n' && n+1>=sizeof(line)) {
            char c;
            while (FRead(in,&c,1,1)==1 && c!='\n') { }
            continue;
        }
        while (n && (line[n-1]=='\n' || line[n-1]=='\r')) line[--n]=0;
        path=beam_trim(line);
        if (!*path || *path=='#' || *path==';') continue;
        tab=strchr(path,'\t');
        if (!tab) continue;
        *tab++=0;
        tab2=strchr(tab,'\t');
        if (tab2) {
            *tab2++=0; label=tab2;
            tab3=strchr(label,'\t');
            if (tab3) { *tab3++=0; command=tab3; }
        }
        path=beam_trim(path); words=beam_trim(tab);
        label=beam_trim(label); command=beam_trim(command);
        beam_aliases_set_full(a,path,words,label,command); /* last duplicate wins */
    }
    Close(in);
    return 1;
}

int beam_aliases_load_active(struct BeamAliases *a)
{
    if (beam_aliases_load(a,BEAM_ALIASES_SESSION)) return 1;
    if (beam_aliases_load(a,BEAM_ALIASES_ENV)) return 1;
    if (beam_aliases_load(a,BEAM_ALIASES_ENVARC)) return 1;
    if (beam_aliases_load(a,BEAM_ALIASES_DEFAULT)) return 1;
    beam_aliases_clear(a);
    return 0;
}

int beam_aliases_save(const struct BeamAliases *a,const char *filename)
{
    BPTR out,lock,dir;
    unsigned int i;
    int ok=1,had_old;
    char temp[BEAM_MAX_PATH+8],bak[BEAM_MAX_PATH+8];
    if (!a || !filename || strlen(filename)>=BEAM_MAX_PATH ||
        a->count>BEAM_ALIAS_CAP) return 0;
    for (i=0;i<a->count;i++)
        if (!safe_path(a->entries[i].path) ||
            !safe_text(a->entries[i].words,BEAM_ALIAS_LEN,1) ||
            !safe_text(a->entries[i].action_label,BEAM_ALIAS_ACTION_LABEL_LEN,1) ||
            !safe_text(a->entries[i].action_command,BEAM_ALIAS_ACTION_COMMAND_LEN,1) ||
            (!!a->entries[i].action_label[0])!=(!!a->entries[i].action_command[0]) ||
            (!a->entries[i].words[0] && !a->entries[i].action_command[0]))
            return 0;
    if (!strcmp(filename,BEAM_ALIASES_SESSION) ||
        !strcmp(filename,BEAM_ALIASES_ENVARC) ||
        !strcmp(filename,BEAM_ALIASES_ENV)) {
        const char *drawer=!strcmp(filename,BEAM_ALIASES_SESSION)?"T:Beam68":
            (!strcmp(filename,BEAM_ALIASES_ENV)?"ENV:Beam68":"ENVARC:Beam68");
        dir=CreateDir((STRPTR)drawer);
        if (dir) UnLock(dir);
    }
    sprintf(temp,"%s.new",filename); sprintf(bak,"%s.bak",filename);
    out=Open((STRPTR)temp,MODE_NEWFILE);
    if (!out) return 0;
    if (FPuts(out,(STRPTR)"# Beam68 0.1 applications: path<TAB>aliases/keywords<TAB>action label<TAB>action command\n")<0)
        ok=0;
    for (i=0;ok && i<a->count;i++) {
        if (FPuts(out,(STRPTR)a->entries[i].path)<0 ||
            FPuts(out,(STRPTR)"\t")<0 ||
            FPuts(out,(STRPTR)a->entries[i].words)<0) ok=0;
        if (ok && a->entries[i].action_label[0] &&
            (FPuts(out,(STRPTR)"\t")<0 ||
             FPuts(out,(STRPTR)a->entries[i].action_label)<0 ||
             FPuts(out,(STRPTR)"\t")<0 ||
             FPuts(out,(STRPTR)a->entries[i].action_command)<0)) ok=0;
        if (ok && FPuts(out,(STRPTR)"\n")<0) ok=0;
    }
    if (Close(out)==0) ok=0;
    if (!ok) { DeleteFile((STRPTR)temp); return 0; }
    lock=Lock((STRPTR)filename,ACCESS_READ); had_old=lock!=0;
    if (lock) UnLock(lock);
    if (had_old) {
        DeleteFile((STRPTR)bak);
        if (!Rename((STRPTR)filename,(STRPTR)bak)) {
            DeleteFile((STRPTR)temp); return 0;
        }
    }
    if (!Rename((STRPTR)temp,(STRPTR)filename)) {
        if (had_old) Rename((STRPTR)bak,(STRPTR)filename);
        DeleteFile((STRPTR)temp); return 0;
    }
    return 1;
}
