#include <string.h>
#include "actions.h"
#include "locale_support.h"
static int lower_ascii(int c)
{ return (c>='A' && c<='Z')?c+('a'-'A'):c; }

static int equal_word(const char *start,const char *end,const char *word)
{
    while (start<end && *word) {
        if (lower_ascii((unsigned char)*start++)!=(unsigned char)*word++) return 0;
    }
    return start==end && !*word;
}

int beam_restart_query(const char *query)
{
    const char *start,*end,*scope;
    if (!query) return BEAM_RESTART_NONE;
    start=query;
    while (*start==' ' || *start=='\t') ++start;
    end=start+strlen(start);
    while (end>start && (end[-1]==' ' || end[-1]=='\t')) --end;
    if ((unsigned int)(end-start)<9U ||
        !equal_word(start,start+7,"restart")) return BEAM_RESTART_NONE;
    scope=start+7;
    if (*scope==':') ++scope;
    else if (*scope==' ' || *scope=='\t') {
        while (scope<end && (*scope==' ' || *scope=='\t')) ++scope;
    } else return BEAM_RESTART_NONE;
    while (scope<end && (*scope==' ' || *scope=='\t')) ++scope;
    if (equal_word(scope,end,"amiga")) return BEAM_RESTART_AMIGA;
    if (equal_word(scope,end,"workbench")) return BEAM_RESTART_WORKBENCH;
    return BEAM_RESTART_NONE;
}

int beam_shell_query(const char *query,const char **command)
{
    static const char prefix[]="shell:";
    int i;
    const char *p;
    if (!query) return 0;
    if (query[0]=='>') {
        p=query+1;
        while (*p==' ' || *p=='\t') ++p;
        if (command) *command=p;
        return 1;
    }
    for (i=0;prefix[i];i++)
        if (!query[i] || lower_ascii((unsigned char)query[i])!=prefix[i])
            return 0;
    p=query+i;
    while (*p==' ' || *p=='\t') ++p;
    if (command) *command=p;
    return 1;
}

int beam_shell_command_copy(const char *query,char *command,unsigned int cap)
{
    const char *start,*end;
    unsigned int n;
    if (!command || !cap) return 0;
    command[0]=0;
    if (!beam_shell_query(query,&start)) return 0;
    end=start+strlen(start);
    while (end>start && (end[-1]==' ' || end[-1]=='\t')) --end;
    n=(unsigned int)(end-start);
    if (n>=cap) return 0;
    if (n) memcpy(command,start,n);
    command[n]=0;
    return 1;
}

int beam_parent_drawer(const char *path,char *parent,unsigned int cap)
{
    const char *colon,*slash,*p;
    unsigned int len;
    if (!path || !*path || !parent || !cap) return 0;
    colon=strchr(path,':');
    if (!colon || colon==path || !colon[1]) return 0;
    slash=NULL;
    for (p=colon+1;*p;p++) if (*p=='/') slash=p;
    len=(unsigned int)(slash?(slash-path):(colon-path+1));
    if (!len || len>=cap) return 0;
    memcpy(parent,path,len); parent[len]=0;
    return 1;
}
/* Tab cycles: Launch -> Run with arguments -> Open drawer -> Information ->
 * Copy path ->
 * [custom action] -> Launch. */
int beam_next_action(int current,int has_custom)
{
    if (current==BEAM_ACTION_LAUNCH) return BEAM_ACTION_ARGS;
    if (current==BEAM_ACTION_ARGS) return BEAM_ACTION_DRAWER;
    if (current==BEAM_ACTION_DRAWER) return BEAM_ACTION_DOPUS;
    if (current==BEAM_ACTION_DOPUS) return BEAM_ACTION_INFO;
    if (current==BEAM_ACTION_INFO) return BEAM_ACTION_COPY;
    if (current==BEAM_ACTION_COPY && has_custom) return BEAM_ACTION_CUSTOM;
    return BEAM_ACTION_LAUNCH;
}
const char *beam_action_name(int action,const char *custom_label)
{
    if (action==BEAM_ACTION_DRAWER) return beam_str(MSG_ACTION_OPEN_DRAWER);
    if (action==BEAM_ACTION_INFO) return beam_str(MSG_ACTION_INFO);
    if (action==BEAM_ACTION_ARGS) return beam_str(MSG_ACTION_ARGS);
    if (action==BEAM_ACTION_DOPUS) return beam_str(MSG_ACTION_DOPUS);
    if (action==BEAM_ACTION_COPY) return beam_str(MSG_ACTION_COPY);
    if (action==BEAM_ACTION_CUSTOM && custom_label && *custom_label)
        return custom_label;
    return beam_str(MSG_ACTION_LAUNCH);
}

static int append_text(char *out,unsigned int cap,unsigned int *used,
                       const char *text,int quote)
{
    unsigned int n=(unsigned int)strlen(text);
    const unsigned char *p=(const unsigned char *)text;
    for (;*p;p++) if (*p<32 || *p==127 ||
        (quote && (*p=='"' || *p=='*' || *p=='`'))) return 0;
    if (*used+n+(quote?2U:0U)>=cap) return 0;
    if (quote) out[(*used)++]='\"';
    memcpy(out+*used,text,n); *used+=n;
    if (quote) out[(*used)++]='\"';
    out[*used]=0;
    return 1;
}

int beam_action_expand(const char *template_text,const char *path,
                       char *output,unsigned int cap)
{
    const char *p,*next;
    unsigned int used=0,n;
    char drawer[BEAM_MAX_PATH];
    if (!template_text || !*template_text || !path || !*path || !output || cap<2)
        return 0;
    output[0]=0; p=template_text;
    while (*p) {
        next=strchr(p,'{');
        if (!next) return append_text(output,cap,&used,p,0);
        n=(unsigned int)(next-p);
        if (used+n>=cap) return 0;
        memcpy(output+used,p,n); used+=n; output[used]=0;
        if (!strncmp(next,"{path}",6)) {
            if (!append_text(output,cap,&used,path,1)) return 0;
            p=next+6;
        } else if (!strncmp(next,"{drawer}",8)) {
            if (!beam_parent_drawer(path,drawer,sizeof(drawer)) ||
                !append_text(output,cap,&used,drawer,1)) return 0;
            p=next+8;
        } else {
            if (!append_text(output,cap,&used,"{",0)) return 0;
            p=next+1;
        }
    }
    return 1;
}
