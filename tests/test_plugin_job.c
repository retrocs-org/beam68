#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "plugin_job.h"
#include "plugin_protocol.h"
static void test_stale_and_recovery(void)
{
    struct BeamPluginJob job={0,0,0};
    char unterminated[8],control[8]="bad\177";
    memset(unterminated,'x',sizeof(unterminated));
    beam_job_begin_timeout(&job,5);
    assert(beam_job_accept_reply(&job,10,10,1,1));
    assert(!beam_job_accept_reply(&job,10,9,1,1));
    assert(!beam_job_accept_reply(&job,10,10,0,1));
    assert(!beam_job_accept_reply(&job,10,10,1,0));
    /* Edit away and back, close, or reload: detaching invalidates even
       an identical query's late packet, before a replacement starts. */
    beam_job_end(&job);
    assert(!beam_job_accept_reply(&job,10,10,1,1));
    beam_job_begin_timeout(&job,5);
    assert(!beam_job_accept_reply(&job,11,10,1,1));
    assert(beam_job_accept_reply(&job,11,11,1,1));
    assert(!beam_job_elapsed(&job,4999));
    assert(beam_job_elapsed(&job,5000)); /* no dependence on tick count */
    assert(job.age==job.timeout_ticks);
    assert(!beam_job_accept_reply(&job,11,11,1,1));
    beam_job_end(&job);
    assert(!beam_job_accept_reply(&job,11,11,1,1));
    beam_job_begin_timeout(&job,5);
    assert(beam_job_accept_reply(&job,12,12,1,1));
    assert(beam_job_elapsed(&job,~0UL));
    assert(job.age==job.timeout_ticks);
    assert(beam_job_validate_row(0,"Ready",6,"",1));
    assert(beam_job_validate_row(1,"Open",5,"SYS:Tools",10));
    assert(beam_job_validate_row(2,"Search",7,"app:Ed",7));
    assert(!beam_job_validate_row(3,"Bad",4,"",1));
    assert(!beam_job_validate_row(0,"",1,"",1));
    assert(!beam_job_validate_row(1,"Open",5,"",1));
    assert(!beam_job_validate_row(2,"Query",6,"",1));
    assert(!beam_job_validate_row(0,unterminated,sizeof(unterminated),"",1));
    assert(!beam_job_validate_row(1,"Open",5,unterminated,sizeof(unterminated)));
    assert(!beam_job_validate_row(0,control,sizeof(control),"",1));
    assert(!beam_job_validate_row(1,"Open",5,"bad\npath",9));
}

static void test_outstanding_request_limit(void)
{
    struct BeamPluginRequests requests={{0}};
    struct BeamPluginJob visible={0,0,0};
    unsigned long serial;
    int first,second,manual;
    assert(beam_requests_count(&requests)==0);
    assert(beam_requests_reserve(&requests,0,1)==-1);
    first=beam_requests_reserve(&requests,1,1);
    assert(first>=0);
    beam_job_begin_timeout(&visible,5);
    beam_job_end(&visible); /* editing/close/reload detaches, not completes */
    assert(beam_requests_count(&requests)==1);
    second=beam_requests_reserve(&requests,2,1);
    assert(second>=0 && second!=first);
    assert(beam_requests_reserve(&requests,2,0)==-1); /* duplicate serial */
    assert(beam_requests_reserve(&requests,3,1)==-1);
    manual=beam_requests_reserve(&requests,3,0);
    assert(manual>=0 && beam_requests_count(&requests)==3);
    beam_job_begin_timeout(&visible,5);
    assert(beam_job_elapsed(&visible,6000));
    beam_job_end(&visible); /* timeout must not recycle possibly live work */
    for (serial=4;serial<10004;serial++) {
        assert(beam_requests_reserve(&requests,serial,serial%2)==-1);
        assert(beam_requests_count(&requests)==3);
    }
    assert(beam_requests_complete(&requests,99999)==-1);
    assert(beam_requests_complete(&requests,1)==first); /* old completion */
    assert(beam_requests_count(&requests)==2);
    assert(!beam_requests_can_start(&requests,1)); /* keep manual reserve */
    assert(beam_requests_complete(&requests,2)==second);
    assert(beam_requests_can_start(&requests,1));
    assert(beam_requests_reserve(&requests,10004,1)>=0);
    assert(beam_requests_complete(&requests,2)==-1); /* duplicate old reply */
    assert(beam_requests_find(&requests,10004)>=0);
    assert(beam_requests_complete(&requests,10004)>=0); /* failed spawn rollback */
    assert(beam_requests_complete(&requests,3)==manual);
    assert(beam_requests_count(&requests)==0);
    assert(beam_requests_reserve(&requests,~0UL,1)>=0);
    assert(beam_requests_complete(&requests,~0UL)>=0);
}

static void test_request_diagnostics(void)
{
    char out[320],small[8],name[100];
    memset(name,'X',sizeof(name));name[sizeof(name)-1]=0;
    assert(beam_request_format(0,0,NULL,0,0,0,0,0,out,sizeof(out)));
    assert(!strcmp(out,"slot=1 state=free"));
    assert(beam_request_format(1,42,"Clock",1,0,1,120,5000,out,sizeof(out)));
    assert(strstr(out,"serial=42 state=active mode=preview transport=native"));
    assert(strstr(out,"age=120ms timeout=5000ms plugin=Clock"));
    assert(beam_request_format(1,42,"Clock",1,0,0,4000,5000,out,sizeof(out)));
    assert(strstr(out,"state=detached"));
    assert(beam_request_format(1,42,"Clock",1,0,1,5000,5000,out,sizeof(out)));
    assert(strstr(out,"state=timedout"));
    assert(beam_request_format(2,~0UL,"ARexx\nName",0,1,0,~0UL,120000,out,sizeof(out)));
    assert(strstr(out,"mode=manual transport=file") && strstr(out,"plugin=ARexx Name"));
    assert(!strchr(out,'\n'));
    assert(beam_request_format(2,99,name,0,0,0,1,5000,out,sizeof(out)));
    assert(strlen(strstr(out,"plugin=")+7)==BEAM_REQUEST_NAME_MAX-1);
    assert(!beam_request_format(0,1,"Clock",0,0,1,0,5000,small,sizeof(small)) && !small[0]);
    assert(!beam_request_format(3,1,"Clock",0,0,1,0,5000,out,sizeof(out)) && !out[0]);
}

int main(void)
{
    assert(beam_job_row_same(2,"ha:entity light.a","Light: off",2,"ha:entity light.a","Light: on"));
    assert(!beam_job_row_same(2,"ha:entity light.a","Light",2,"ha:entity light.b","Light"));
    assert(!beam_job_row_same(2,"a","Row",1,"a","Row"));
    assert(beam_job_row_same(0,"","Same",0,"","Same"));
    assert(!beam_job_row_same(0,"","Old",0,"","New"));

    struct BeamPluginJob job={0,0,0};
    char hex[BEAM_PLUGIN_MAX_INPUT*2+2],out[BEAM_PLUGIN_MAX_RESULT+1],decoded[BEAM_PLUGIN_MAX_INPUT+1];
    char oversized[BEAM_PLUGIN_MAX_INPUT+2];
    int error,i;
    assert(!beam_plugin_needs_confirmation("org.beam68.wikipedia","Amiga",1));
    assert(!beam_plugin_needs_confirmation("org.beam68.wikipedia","article en 1980",1));
    assert(!beam_plugin_needs_confirmation("org.beam68.amihass","kitchen",1));
    assert(!beam_plugin_needs_confirmation("org.beam68.musiclibrary","enigma",1));
    assert(!beam_plugin_needs_confirmation("org.beam68.musiclibrary","playful",1));
    assert(!beam_plugin_needs_confirmation("org.beam68.musiclibrary","p",1));
    assert(beam_plugin_needs_confirmation("org.beam68.musiclibrary","play Work:Music/mod.demo",1));
    assert(beam_plugin_needs_confirmation("org.beam68.musiclibrary","  PLAY\tWork:Music/mod.demo",1));
    assert(beam_plugin_needs_confirmation("org.beam68.musiclibrary","play",1));
    assert(beam_plugin_needs_confirmation("org.thirdparty.tool","search",1));
    assert(beam_plugin_needs_confirmation(NULL,"search",1));
    assert(beam_plugin_needs_confirmation("org.beam68.wikipedia",NULL,1));
    assert(!beam_plugin_needs_confirmation(NULL,NULL,0));
    test_stale_and_recovery();
    test_outstanding_request_limit();
    test_request_diagnostics();
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
