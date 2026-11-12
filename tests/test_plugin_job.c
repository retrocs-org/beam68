#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "plugin_job.h"
#include "plugin_protocol.h"
int main(void)
{
    struct BeamPluginJob job={0,0,0};
    char hex[BEAM_PLUGIN_MAX_INPUT*2+2],out[BEAM_PLUGIN_MAX_RESULT+1],decoded[BEAM_PLUGIN_MAX_INPUT+1];
    char oversized[BEAM_PLUGIN_MAX_INPUT+2];
    int error,i;
    assert(beam_plugin_hex_argument("12*(3+4)",hex,sizeof(hex)));
    assert(!strcmp(hex,"31322A28332B3429"));
    assert(beam_decode_hex(hex,decoded,sizeof(decoded)));
    assert(!strcmp(decoded,"12*(3+4)"));
    assert(beam_plugin_hex_argument("",hex,sizeof(hex)) && !strcmp(hex,"-"));
    assert(!beam_plugin_hex_argument("",hex,1));
    assert(!beam_plugin_hex_argument("ab",hex,4));
    memset(oversized,'x',sizeof(oversized));oversized[sizeof(oversized)-1]=0;
    assert(!beam_plugin_hex_argument(oversized,hex,sizeof(hex)));
    assert(beam_plugin_parse_reply("TEXT=42\n",out,sizeof(out),&error)==1);
    assert(!strcmp(out,"42") && !error);
    assert(beam_plugin_parse_reply("ERROR=No result\r\n",out,sizeof(out),&error)==1);
    assert(error && !strcmp(out,"No result"));
    assert(beam_plugin_parse_reply("TEXT=Unfinished",out,sizeof(out),&error)==0);
    assert(beam_plugin_parse_reply("INVALID=xyz\n",out,sizeof(out),&error)==-1);
    assert(beam_plugin_parse_reply("TEXT=\n",out,sizeof(out),&error)==-1);
    assert(beam_plugin_parse_reply("ERROR=\r\n",out,sizeof(out),&error)==-1);
    assert(beam_plugin_parse_reply("TEXT=evil\033[31m\n",out,sizeof(out),&error)==-1);
    assert(beam_plugin_parse_reply("TEXT=ok\n",out,2,&error)==-1);
    beam_job_begin(&job);
    assert(beam_job_elapsed_ms(&job)==0);
    assert(!beam_job_tick(&job));
    assert(beam_job_elapsed_ms(&job)==50);
    for(i=2;i<BEAM_PLUGIN_WAIT_TICKS;i++)assert(!beam_job_tick(&job));
    assert(beam_job_tick(&job));
    beam_job_end(&job);
    assert(!beam_job_tick(&job));
    beam_job_begin_timeout(&job,5);
    for(i=1;i<5*BEAM_PLUGIN_TICKS_PER_SECOND;i++) assert(!beam_job_tick(&job));
    assert(beam_job_tick(&job));
    beam_job_begin_timeout(&job,120);
    for(i=1;i<120*BEAM_PLUGIN_TICKS_PER_SECOND;i++) assert(!beam_job_tick(&job));
    assert(beam_job_tick(&job));
    beam_job_begin_timeout(&job,500); /* invalid => old 30s default */
    assert(job.timeout_ticks==BEAM_PLUGIN_WAIT_TICKS);
    {
        char command[128];
        struct { char value[40]; unsigned char guard[4]; } small;
        assert(beam_plugin_command("Work:My Plugin","4142","T:reply",0,"BEAM68_REPLY",42,command,sizeof(command)));
        assert(!strcmp(command,"\"Work:My Plugin\" 4142 \"T:reply\" BEAM68_REPLY 42"));
        memset(&small,0xa5,sizeof(small));
        assert(!beam_plugin_command("Work:My Plugin","4142","T:reply",0,"BEAM68_REPLY",42,small.value,sizeof(small.value)));
        assert(!small.value[0] && small.guard[0]==0xa5 && small.guard[3]==0xa5);
        assert(!beam_plugin_command("Work:Bad*","4142","T:reply",0,NULL,0,command,sizeof(command)));
        assert(!beam_plugin_command("Work:Good","41;bad","T:reply",0,NULL,0,command,sizeof(command)));
    }
    puts("Async plugin job, hex encoding, reply validation, timeout: PASS");
    return 0;
}
