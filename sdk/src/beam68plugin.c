/* beam68plugin.c - Beam68 plugin SDK helpers, protocol 1.  Public domain /
 * MIT, like Beam68 itself.  AmigaOS 3.0+, any 680x0, vbcc or gcc. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include "beam68plugin.h"

#ifndef BEAM68_NO_LOCALE
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/locale.h>
struct LocaleBase *LocaleBase;
static struct Catalog *plugin_catalog;

void beam68_catalog_open(const char *catalog_name)
{
    if (plugin_catalog || !catalog_name) return;
    LocaleBase=(struct LocaleBase *)OpenLibrary((STRPTR)"locale.library",38);
    if (LocaleBase) plugin_catalog=OpenCatalogA(NULL,(STRPTR)catalog_name,NULL);
}

const char *beam68_str(long id, const char *english)
{
    return plugin_catalog?(const char *)GetCatalogStr(plugin_catalog,id,(STRPTR)english):english;
}

void beam68_catalog_close(void)
{
    if (plugin_catalog) { CloseCatalog(plugin_catalog); plugin_catalog=NULL; }
    if (LocaleBase) { CloseLibrary((struct Library *)LocaleBase); LocaleBase=NULL; }
}
#else
void beam68_catalog_open(const char *catalog_name) { (void)catalog_name; }
const char *beam68_str(long id, const char *english) { (void)id; return english; }
void beam68_catalog_close(void) { }
#endif

static int lower(int c) { return c>='A' && c<='Z' ? c+32 : c; }

static int hex(int c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}

static int parse_serial(const char *s, unsigned long *serial)
{
    unsigned long v=0;
    if (!s || !*s) return 0;
    while (*s) {
        unsigned long digit;
        if (*s<'0' || *s>'9') return 0;
        digit=(unsigned long)(*s++-'0');
        if (v>(0xffffffffUL-digit)/10UL) return 0;
        v=v*10UL+digit;
    }
    *serial=v;
    return 1;
}

int beam68_plugin_input(int argc, char **argv, char *input, unsigned int cap)
{
    const char *s;
    unsigned int n=0;
    unsigned long serial;
    if (argc!=5 || !argv || !argv[1] || !argv[2] || !argv[3] || !argv[4] ||
        strcmp(argv[3],BEAM68_REPLY_PORT) || !parse_serial(argv[4],&serial) || !input || !cap) return 0;
    s=argv[1];
    if (s[0]=='-' && !s[1]) { input[0]=0; return 1; }
    while (*s) {
        int a=hex((unsigned char)s[0]),b;
        if (a<0 || !s[1]) return 0;
        b=hex((unsigned char)s[1]);
        if (b<0 || !((a<<4)|b) || n>=BEAM68_PLUGIN_MAX_INPUT || n+1>=cap) return 0;
        input[n++]=(char)((a<<4)|b);
        s+=2;
    }
    input[n]=0;
    return 1;
}

int beam68_plugin_text(int argc, char **argv, const char *text)
{
    beam68_rows_begin();
    if (!beam68_row(BEAM68_ROW_TEXT,text,NULL)) return 20;
    return beam68_rows_send(argc,argv,0);
}

int beam68_plugin_error(int argc, char **argv, const char *text)
{
    beam68_rows_begin();
    if (!beam68_row(BEAM68_ROW_TEXT,text,NULL)) return 20;
    return beam68_rows_send(argc,argv,1);
}

char *beam68_trim(char *text)
{
    char *end;
    while (*text==' ' || *text=='\t') text++;
    end=text+strlen(text);
    while (end>text && (end[-1]==' ' || end[-1]=='\t' || end[-1]=='\r' || end[-1]=='\n')) --end;
    *end=0;
    return text;
}

int beam68_equal(const char *a, const char *b)
{
    while (*a && *b) if (lower((unsigned char)*a++)!=lower((unsigned char)*b++)) return 0;
    return !*a && !*b;
}

int beam68_prefix(const char *text, const char *prefix)
{
    while (*prefix)
        if (!*text || lower((unsigned char)*text++)!=lower((unsigned char)*prefix++)) return 0;
    return 1;
}

int beam68_contains(const char *text, const char *needle)
{
    if (!*needle) return 1;
    for (;*text;text++) if (beam68_prefix(text,needle)) return 1;
    return 0;
}

void beam68_copy(char *dst, const char *src, unsigned int cap)
{
    unsigned int i=0;
    if (!cap) return;
    while (src && src[i] && i+1<cap) { dst[i]=src[i]; i++; }
    dst[i]=0;
}

/* ---------------------------------------------------------- protocol 1 */

static struct Beam68ReplyRow pending_rows[BEAM68_MAX_ROWS];
static int pending_count;

void beam68_rows_begin(void) { pending_count=0; }

int beam68_row(int kind, const char *text, const char *value)
{
    struct Beam68ReplyRow *r;
    unsigned int i;
    if (pending_count>=BEAM68_MAX_ROWS || kind<BEAM68_ROW_TEXT ||
        kind>BEAM68_ROW_QUERY || !text || !*text ||
        (kind!=BEAM68_ROW_TEXT && (!value || !*value || strlen(value)>=240))) return 0;
    r=&pending_rows[pending_count++];
    r->kind=(unsigned char)kind;
    for (i=0;text[i] && i<BEAM68_PLUGIN_MAX_RESULT;i++) {
        unsigned char c=(unsigned char)text[i];
        r->text[i]=(char)((c<32 || c==127)?' ':c);
    }
    r->text[i]=0;
    beam68_copy(r->value,value?value:"",sizeof(r->value));
    return 1;
}

#ifdef BEAM68_NO_PORTS
static int send_message(int argc, char **argv, int is_error)
{ (void)argc; (void)argv; (void)is_error; return 0; }
#else
#include <exec/memory.h>
#include <proto/exec.h>
static int send_message(int argc, char **argv, int is_error)
{
    struct Beam68ReplyMessage *m;
    struct MsgPort *port;
    int i;
    unsigned long serial;
    if (argc!=5 || !argv || !argv[3] || strcmp(argv[3],BEAM68_REPLY_PORT) ||
        !parse_serial(argv[4],&serial)) return 0;
    m=(struct Beam68ReplyMessage *)AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    if (!m) return 0;
    m->message.mn_Node.ln_Type=NT_MESSAGE;
    m->message.mn_Length=(UWORD)sizeof(*m);
    m->message.mn_ReplyPort=NULL;
    m->magic=BEAM68_REPLY_MAGIC;
    m->version=BEAM68_REPLY_VERSION;
    m->serial=serial;
    m->is_error=(unsigned short)(is_error?1:0);
    m->count=(unsigned short)pending_count;
    for (i=0;i<pending_count;i++) m->rows[i]=pending_rows[i];
    Forbid();
    port=FindPort((STRPTR)BEAM68_REPLY_PORT);
    if (port) PutMsg(port,(struct Message *)m);
    Permit();
    if (!port) { FreeVec(m); return 0; }
    return 1;
}
#endif

int beam68_rows_send(int argc, char **argv, int is_error)
{
    if (!pending_count) beam68_row(BEAM68_ROW_TEXT,is_error?"Error":"Done",NULL);
    if (send_message(argc,argv,is_error)) return 0;
    /* A failed message delivery must not silently lose selectable rows. */
    return 20;
}
