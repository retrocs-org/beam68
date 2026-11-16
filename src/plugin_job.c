#include <string.h>
#include <stdio.h>
#include "plugin_job.h"
#include "plugin_protocol.h"

void beam_job_begin_timeout(struct BeamPluginJob *job, int seconds)
{
    if (seconds<5 || seconds>120) seconds=30;
    job->age=0;
    job->timeout_ticks=(unsigned int)seconds*BEAM_PLUGIN_TICKS_PER_SECOND;
    job->active=1;
}
void beam_job_begin(struct BeamPluginJob *job)
{
    beam_job_begin_timeout(job,30);
}
void beam_job_end(struct BeamPluginJob *job)
{
    job->active=0;
    job->age=0;
}
int beam_job_tick(struct BeamPluginJob *job)
{
    if (!job->active) return 0;
    if (job->age<job->timeout_ticks) ++job->age;
    return job->age>=job->timeout_ticks;
}
unsigned long beam_job_elapsed_ms(const struct BeamPluginJob *job)
{
    return job ? (unsigned long)job->age*(1000UL/BEAM_PLUGIN_TICKS_PER_SECOND) : 0;
}
int beam_plugin_hex_argument(const char *input, char *out, unsigned int cap)
{
    static const char hex[]="0123456789ABCDEF";
    unsigned int i,n=(unsigned int)strlen(input);
    if (n>BEAM_PLUGIN_MAX_INPUT || cap<2*n+1 || !cap) return 0;
    if (n==0) {
        if (cap<2) return 0;
        out[0]='-'; out[1]=0; return 1;
    }
    for (i=0;i<n;i++) {
        unsigned char c=(unsigned char)input[i];
        out[2*i]=hex[c>>4]; out[2*i+1]=hex[c&15];
    }
    out[2*n]=0;
    return 1;
}
int beam_plugin_parse_reply(const char *line, char *result, unsigned int cap,
                            int *is_error)
{
    unsigned int i, n;
    const char *data;
    if (!line || !result || !is_error || !cap) return -1;
    n=(unsigned int)strlen(line);
    if (!n || line[n-1]!='\n') return 0; /* may still be writing */
    if (!strncmp(line,"TEXT=",5)) { data=line+5; *is_error=0; }
    else if (!strncmp(line,"ERROR=",6)) { data=line+6; *is_error=1; }
    else return -1;
    n=(unsigned int)strlen(data);
    if (n && data[n-1]=='\n') --n;
    if (n && data[n-1]=='\r') --n;
    /* An empty reply cannot convey a useful result and otherwise looks like
     * a successful plugin that silently did nothing. */
    if (!n || n>=cap || n>BEAM_PLUGIN_MAX_RESULT) return -1;
    for (i=0;i<n;i++) {
        unsigned char c=(unsigned char)data[i];
        /* Don't let third-party plugins inject control codes into a UI. */
        if (c<32 || c==127) return -1;
    }
    memcpy(result,data,n);
    result[n]=0;
    return 1;
}

static int command_path_safe(const char *s)
{
    const unsigned char *p=(const unsigned char *)s;
    if (!s || !*s) return 0;
    for (;*p;p++) if (*p<32 || *p==127 || *p=='"' || *p=='*' || *p=='`') return 0;
    return 1;
}
int beam_plugin_command(const char *program,const char *hex,const char *result,
                        int rexx,const char *port,unsigned long serial,
                        char *out,unsigned int cap)
{
    char suffix[96];
    unsigned long needed;
    const char *p;
    if (!out || !cap) return 0;
    out[0]=0; suffix[0]=0;
    if (!command_path_safe(program) || !command_path_safe(result) || !hex || !*hex) return 0;
    if (strcmp(hex,"-")) {
        if (strlen(hex)&1U) return 0;
        for (p=hex;*p;p++) if (!((*p>='0' && *p<='9') || (*p>='A' && *p<='F') || (*p>='a' && *p<='f'))) return 0;
    }
    if (port) {
        if (!*port || strlen(port)>63) return 0;
        for (p=port;*p;p++) if (!((*p>='A' && *p<='Z') || (*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='_')) return 0;
        sprintf(suffix," %s %lu",port,serial);
    }
    needed=(unsigned long)strlen(program)+strlen(hex)+strlen(result)+strlen(suffix)+6UL+(rexx?3UL:0UL);
    if (needed>=(unsigned long)cap) return 0;
    sprintf(out,"%s\"%s\" %s \"%s\"%s",rexx?"RX ":"",program,hex,result,suffix);
    return 1;
}

int beam_job_elapsed(struct BeamPluginJob *job,unsigned long milliseconds)
{
    unsigned long ticks=milliseconds/(1000UL/BEAM_PLUGIN_TICKS_PER_SECOND);
    if (!job || !job->active) return 0;
    job->age=ticks>job->timeout_ticks?job->timeout_ticks:(unsigned int)ticks;
    return job->age>=job->timeout_ticks;
}
int beam_job_accept_reply(const struct BeamPluginJob *job,unsigned long expected,
                          unsigned long received,int same_query,int visible)
{
    return job && job->active && job->age<job->timeout_ticks &&
           expected==received && same_query && visible;
}
static int bounded_text(const char *text,unsigned int cap,int required)
{
    unsigned int i;
    if (!text || !cap || (required && !text[0])) return 0;
    for (i=0;i<cap;i++) {
        unsigned char c=(unsigned char)text[i];
        if (!c) return 1;
        if (c<32 || c==127) return 0;
    }
    return 0;
}
int beam_job_validate_row(unsigned int kind,const char *text,unsigned int text_cap,
                          const char *value,unsigned int value_cap)
{
    return kind<=2 && bounded_text(text,text_cap,1) &&
           bounded_text(value,value_cap,kind!=0);
}

unsigned int beam_requests_count(const struct BeamPluginRequests *requests)
{
    unsigned int i,count=0;
    for (i=0;i<BEAM_PLUGIN_REQUEST_LIMIT;i++) if (requests->serial[i]) ++count;
    return count;
}
int beam_requests_can_start(const struct BeamPluginRequests *requests,int preview)
{
    return beam_requests_count(requests)<(preview?BEAM_PLUGIN_PREVIEW_LIMIT:BEAM_PLUGIN_REQUEST_LIMIT);
}
int beam_requests_find(const struct BeamPluginRequests *requests,unsigned long serial)
{
    unsigned int i;
    if (!serial) return -1;
    for (i=0;i<BEAM_PLUGIN_REQUEST_LIMIT;i++) if (requests->serial[i]==serial) return (int)i;
    return -1;
}
int beam_requests_reserve(struct BeamPluginRequests *requests,unsigned long serial,int preview)
{
    unsigned int i;
    if (!serial || beam_requests_find(requests,serial)>=0 || !beam_requests_can_start(requests,preview)) return -1;
    for (i=0;i<BEAM_PLUGIN_REQUEST_LIMIT;i++) if (!requests->serial[i]) {
        requests->serial[i]=serial;return (int)i;
    }
    return -1;
}
int beam_requests_complete(struct BeamPluginRequests *requests,unsigned long serial)
{
    int i=beam_requests_find(requests,serial);
    if (i>=0) requests->serial[i]=0;
    return i;
}

int beam_request_format(unsigned int slot,unsigned long serial,const char *name,
                         int preview,int file_reply,int active,unsigned long age_ms,
                         unsigned long timeout_ms,char *out,unsigned int capacity)
{
    char line[320],label[BEAM_REQUEST_NAME_MAX];
    unsigned int i=0;
    const char *state;
    if (!out || !capacity) return 0;
    out[0]=0;
    if (slot>=BEAM_PLUGIN_REQUEST_LIMIT) return 0;
    if (!serial) sprintf(line,"slot=%u state=free",slot+1U);
    else {
        if (!name || !*name) name="Unnamed";
        while (name[i] && i+1U<sizeof(label)) {
            unsigned char c=(unsigned char)name[i];
            label[i]=(char)((c<32 || c==127)?' ':c);++i;
        }
        label[i]=0;
        state=timeout_ms && age_ms>=timeout_ms?"timedout":active?"active":"detached";
        sprintf(line,"slot=%u serial=%lu state=%s mode=%s transport=%s age=%lums timeout=%lums plugin=%s",
            slot+1U,serial,state,preview?"preview":"manual",file_reply?"file":"native",
            age_ms,timeout_ms,label);
    }
    if (strlen(line)>=capacity) return 0;
    strcpy(out,line);return 1;
}

int beam_plugin_needs_confirmation(const char *id,const char *input,int system)
{
    unsigned int i;
    static const char play[]="play";
    if (!system) return 0;
    if (!id || !input) return 1;
    /* Wikipedia requests only fetch search results or article summaries.
     * Opening a browser is an explicit button in its own summary window. */
    if (!strcmp(id,"org.beam68.wikipedia")) return 0;
    if (strcmp(id,"org.beam68.musiclibrary")) return 1;
    while (*input==' ' || *input=='\t') ++input;
    for (i=0;i<4;i++) {
        unsigned char c=(unsigned char)input[i];
        if (c>='A' && c<='Z') c=(unsigned char)(c-'A'+'a');
        if (c!=(unsigned char)play[i]) return 0;
    }
    return !input[4] || input[4]==' ' || input[4]=='\t';
}
