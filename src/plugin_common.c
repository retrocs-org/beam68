#include <string.h>
#include "plugin_protocol.h"
#include "beam68plugin.h"
#include "plugin_common.h"

static int lower_ascii(int c)
{ return c>='A' && c<='Z' ? c+32 : c; }

void beam_plugin_copy(char *dst,const char *src,unsigned int cap)
{
    unsigned int i=0;
    if (!cap) return;
    while (src && src[i] && i+1<cap) { dst[i]=src[i];i++; }
    dst[i]=0;
}

char *beam_plugin_trim(char *text)
{
    char *end;
    while (*text==' ' || *text=='\t') text++;
    end=text+strlen(text);
    while (end>text && (end[-1]==' ' || end[-1]=='\t' ||
                        end[-1]=='\r' || end[-1]=='\n')) --end;
    *end=0;
    return text;
}

int beam_ascii_equal(const char *a,const char *b)
{
    while (*a && *b)
        if (lower_ascii((unsigned char)*a++)!=lower_ascii((unsigned char)*b++)) return 0;
    return !*a && !*b;
}

int beam_ascii_prefix(const char *text,const char *prefix)
{
    while (*prefix) {
        if (!*text || lower_ascii((unsigned char)*text++)!=
            lower_ascii((unsigned char)*prefix++)) return 0;
    }
    return 1;
}

int beam_ascii_contains(const char *text,const char *needle)
{
    if (!*needle) return 1;
    while (*text) { if (beam_ascii_prefix(text,needle)) return 1;text++; }
    return 0;
}

int beam_plugin_input(int argc,char **argv,char *input,unsigned int cap)
{
    return beam68_plugin_input(argc,argv,input,cap);
}

int beam_plugin_text(int argc,char **argv,const char *text)
{ return beam68_plugin_text(argc,argv,text); }
int beam_plugin_error(int argc,char **argv,const char *text)
{ return beam68_plugin_error(argc,argv,text); }

/* Localized bundled strings retain their TEXT=/ERROR= marker internally.
 * Strip it before sending the same public row message as every native plugin. */
int beam_plugin_line(int argc,char **argv,const char *line)
{
    char text[BEAM_PLUGIN_MAX_RESULT+1];
    unsigned int n;
    int error;
    const char *start;
    if (!line) return 20;
    if (!strncmp(line,"TEXT=",5)) { start=line+5;error=0; }
    else if (!strncmp(line,"ERROR=",6)) { start=line+6;error=1; }
    else return 20;
    n=(unsigned int)strlen(start);
    if (n && start[n-1]=='\n') --n;
    if (!n || n>BEAM_PLUGIN_MAX_RESULT) return 20;
    memcpy(text,start,n);text[n]=0;
    return error?beam_plugin_error(argc,argv,text):beam_plugin_text(argc,argv,text);
}
