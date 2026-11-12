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
