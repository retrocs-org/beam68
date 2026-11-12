#include "rexx_commands.h"
#include <string.h>

static int upper(int c) { return (c>='a' && c<='z')?c-32:c; }
static int token_eq(const char *start, unsigned int len, const char *word)
{
    unsigned int i;
    for (i=0; i<len && word[i]; i++)
        if (upper((unsigned char)start[i]) != upper((unsigned char)word[i])) return 0;
    return i==len && word[i]==0;
}
int beam_rexx_parse(const char *input, struct BeamRexxCommand *out)
{
    const char *p,*start;
    unsigned int len=0;
    int action=0, number=0, digits=0;
    if (!out) return 0;
    out->action=0;out->plugin_index=-1;
    if (!input) return 0;
    p=input;
    while (*p==' ' || *p=='\t') p++;
    start=p;
    while ((*p>='A'&&*p<='Z') || (*p>='a'&&*p<='z')) {p++; if (++len>16) return 0;}
    if (!len || (*p!='\0' && *p!=' ' && *p!='\t')) return 0;
    if (token_eq(start,len,"SHOW")) action=BEAM_REXX_SHOW;
    else if (token_eq(start,len,"HIDE")) action=BEAM_REXX_HIDE;
    else if (token_eq(start,len,"RELOAD")) action=BEAM_REXX_RELOAD;
    else if (token_eq(start,len,"REINDEX")) action=BEAM_REXX_REINDEX;
    else if (token_eq(start,len,"RELOADPLUGINS")) action=BEAM_REXX_RELOAD_PLUGINS;
    else if (token_eq(start,len,"QUIT")) action=BEAM_REXX_QUIT;
    else if (token_eq(start,len,"STATUS")) action=BEAM_REXX_STATUS;
    else if (token_eq(start,len,"INDEX")) action=BEAM_REXX_INDEX;
    else if (token_eq(start,len,"VERSION")) action=BEAM_REXX_VERSION;
    else if (token_eq(start,len,"HELP")) action=BEAM_REXX_HELP;
    else if (token_eq(start,len,"PLUGINS")) action=BEAM_REXX_PLUGINS;
    else if (token_eq(start,len,"PLUGIN")) action=BEAM_REXX_PLUGIN;
    else if (token_eq(start,len,"TIMING")) action=BEAM_REXX_TIMING;
    else return 0;
    while (*p==' ' || *p=='\t') p++;
    if (action==BEAM_REXX_PLUGIN) {
        while (*p>='0' && *p<='9') {
            if (++digits>2) return 0;
            number=number*10+(*p++-'0');
        }
        while (*p==' ' || *p=='\t') p++;
        if (*p || !digits || number<1 || number>16) return 0;
        out->plugin_index=number-1;
    } else if (*p) return 0;
    out->action=action;
    return 1;
}
