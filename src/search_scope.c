#include <string.h>
#include "search_scope.h"

static int lower_char(int c)
{ return (c>='A' && c<='Z')?c+32:c; }

static int prefix(const char *text,const char *word)
{
    while (*word) if (lower_char((unsigned char)*text++)!=
                       lower_char((unsigned char)*word++)) return 0;
    return 1;
}

int beam_search_scope_parse(const char *query,const char **text)
{
    static const struct { const char *name; int scope; } scopes[]={
        {"app:",BEAM_SCOPE_APP},{"file:",BEAM_SCOPE_FILE},
        {"drawer:",BEAM_SCOPE_DRAWER},{"plugin:",BEAM_SCOPE_PLUGIN},
        {"recent:",BEAM_SCOPE_RECENT},{"volume:",BEAM_SCOPE_VOLUME},
        {"fav:",BEAM_SCOPE_FAVORITE},{"game:",BEAM_SCOPE_GAME},
        {"doc:",BEAM_SCOPE_DOC},{"hidden:",BEAM_SCOPE_HIDDEN},
        {"mod:",BEAM_SCOPE_MODULE},{"script:",BEAM_SCOPE_SCRIPT},{"web:",BEAM_SCOPE_WEB},
        {"snip:",BEAM_SCOPE_SNIPPET},{"prefs:",BEAM_SCOPE_PREFS},{"stats:",BEAM_SCOPE_STATS},{NULL,BEAM_SCOPE_ALL}
    };
    int i;
    if (!query) query="";
    for (i=0;scopes[i].name;i++) {
        unsigned int n=(unsigned int)strlen(scopes[i].name);
        if (strlen(query)>=n && prefix(query,scopes[i].name)) {
            query+=n;while (*query==' ' || *query=='\t') ++query;
            if (text) *text=query;
            return scopes[i].scope;
        }
    }
    if (text) *text=query;
    return BEAM_SCOPE_ALL;
}

int beam_search_scope_kind_allowed(int scope,int kind)
{
    if (scope==BEAM_SCOPE_APP)
        return kind==BEAM_ITEM_WORKBENCH || kind==BEAM_ITEM_SHELL;
    if (scope==BEAM_SCOPE_FILE) return kind==BEAM_ITEM_FILE;
    if (scope==BEAM_SCOPE_DRAWER) return kind==BEAM_ITEM_DRAWER;
    if (scope==BEAM_SCOPE_VOLUME) return kind==BEAM_ITEM_VOLUME;
    if (scope==BEAM_SCOPE_GAME) return kind==BEAM_ITEM_GAME;
    if (scope==BEAM_SCOPE_MODULE) return kind==BEAM_ITEM_MODULE;
    if (scope==BEAM_SCOPE_SCRIPT) return kind==BEAM_ITEM_SCRIPT;
    if (scope==BEAM_SCOPE_WEB) return kind==BEAM_ITEM_BOOKMARK;
    if (scope==BEAM_SCOPE_SNIPPET) return kind==BEAM_ITEM_SNIPPET;
    if (scope==BEAM_SCOPE_PREFS) return kind==BEAM_ITEM_WORKBENCH || kind==BEAM_ITEM_SHELL;
    if (scope==BEAM_SCOPE_PLUGIN) return 0;
    return 1;
}

static int glob_match(const char *pattern,const char *text)
{
    while (*pattern) {
        if (*pattern=='*') {
            while (*pattern=='*') ++pattern;
            if (!*pattern) return 1;
            while (*text) {
                if (glob_match(pattern,text)) return 1;
                ++text;
            }
            return 0;
        }
        if (!*text) return 0;
        if (*pattern!='?' && lower_char((unsigned char)*pattern)!=
            lower_char((unsigned char)*text)) return 0;
        ++pattern;++text;
    }
    return !*text;
}

int beam_filename_patterns_match(const char *name,const char *patterns)
{
    char part[64];
    const char *p=patterns;
    int any=0;
    if (!name || !patterns || !*patterns) return 1;
    while (*p) {
        unsigned int n=0;
        while (*p==' ' || *p=='\t' || *p==',' || *p==';') ++p;
        while (*p && *p!=',' && *p!=';') {
            if (n+1<sizeof(part)) part[n++]=*p;
            ++p;
        }
        while (n && (part[n-1]==' ' || part[n-1]=='\t')) --n;
        if (n) {
            part[n]=0;any=1;
            if (glob_match(part,name)) return 1;
        }
    }
    return !any;
}

int beam_path_is_excluded(const char *path,
                          const char excluded[][BEAM_MAX_PATH],int count)
{
    int i;
    if (!path || !excluded || count<1) return 0;
    for (i=0;i<count;i++) {
        unsigned int n=(unsigned int)strlen(excluded[i]);
        if (!n || strlen(path)<n || !prefix(path,excluded[i])) continue;
        if (!path[n] || path[n]=='/' || path[n]==':') return 1;
    }
    return 0;
}
