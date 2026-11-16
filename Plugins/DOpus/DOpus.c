#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "plugin_common.h"
#include "plugin_protocol.h"
#include "beam68plugin.h"
static struct BeamConfig cfg;
static char input[BEAM_PLUGIN_MAX_INPUT+1];
static void setting(const char *file,const char *wanted,char *out,unsigned int cap)
{
    BPTR f;char line[256],*eq,*key;
    f=Open((STRPTR)file,MODE_OLDFILE);if(!f)return;
    while(FGets(f,line,sizeof(line))) {
        key=beam_plugin_trim(line);eq=strchr(key,'=');
        if(!eq || *key=='#' || *key==';')continue;
        *eq++=0;
        if(beam_ascii_equal(beam_plugin_trim(key),wanted))
            beam_plugin_copy(out,beam_plugin_trim(eq),cap);
    }
    Close(f);
}
static int safe(const char *p) { return p && *p && !strpbrk(p,"\"*`\r\n"); }
#include <rexx/storage.h>
#include <proto/rexxsyslib.h>
#include "rexx_message.h"
#include "rexx_sequence.h"
struct RxsLib *RexxSysBase=NULL;
static LONG send_command(const char *port,const char *command,
                         char *output,unsigned int capacity)
{
    struct MsgPort *reply,*target;
    struct RexxMsg *request,*response;
    LONG result=20;int delivered=0;
    if (output && capacity) output[0]=0;
    if (!RexxSysBase || !port || !*port || !command || !*command) return 20;
    reply=CreateMsgPort();if (!reply) return 20;
    request=beam_rexx_message_create(reply,command,output && capacity);
    if (!request) { DeleteMsgPort(reply);return 20; }
    Forbid();target=FindPort((STRPTR)port);
    if (target) { PutMsg(target,(struct Message *)request);delivered=1; }
    Permit();
    if (delivered) {
        int waited;
        response=NULL;
        /* A hung player must not hang the plugin forever (~10 s limit). */
        for (waited=0;waited<75;waited++) {
            response=(struct RexxMsg *)GetMsg(reply);
            if (response) break;
            Delay(2);
        }
        if (!response) {
            /* The player still owns the message and will reply later; it is
               unsafe to free it.  Make the port silent and leave both
               allocated (a small, bounded leak) rather than crash. */
            Forbid();
            reply->mp_Flags=PA_IGNORE;
            Permit();
            if (output && capacity) beam_plugin_copy(output,"Directory Opus did not answer.",capacity);
            return 20;
        }
        if (response==request) {
            result=request->rm_Result1;
            if (!result && request->rm_Result2) {
                if (!result && output && capacity)
                    beam_plugin_copy(output,(STRPTR)request->rm_Result2,capacity);
                DeleteArgstring((STRPTR)request->rm_Result2);
                request->rm_Result2=0;
            }
        }
    }
    beam_rexx_message_delete(request);DeleteMsgPort(reply);return result;
}

static int open_drawer(const char *drawer,const char *select)
{
    char port[32],command[BEAM_MAX_PATH+80],handle[32];
    int stage=REXX_DOPUS_NEW,next,ok;
    if(!safe(drawer) || (select && *select && !safe(select)))return 20;
    beam_plugin_copy(port,cfg.dopus_port,sizeof(port));
    setting("ENVARC:Beam68/Plugins/DOpus.cfg","PORT",port,sizeof(port));
    if(!*port)return 20;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);if(!RexxSysBase)return 20;
    sprintf(command,"lister new \"%s\"",drawer);handle[0]=0;
    for(;;) {
        ok=send_command(port,command,stage==REXX_DOPUS_NEW?handle:NULL,sizeof(handle))==0;
        next=beam_rexx_next(stage,ok,handle,drawer,select?select:"",command,sizeof(command));
        if(next<=0){if(next<0)ok=0;break;}stage=next;
    }
    CloseLibrary((struct Library *)RexxSysBase);RexxSysBase=NULL;return ok?0:20;
}
int main(int argc,char **argv)
{
    char *q;int rc;
    beam_config_defaults(&cfg);beam_config_read_active(&cfg);
    if(argc>=3 && argc<=4 && beam_ascii_equal(argv[1],"OPEN"))
        return open_drawer(argv[2],argc==4?argv[3]:NULL);
    if(!beam_plugin_input(argc,argv,input,sizeof(input)))return 20;
    q=beam_plugin_trim(input);if(beam_ascii_prefix(q,"open "))q=beam_plugin_trim(q+5);
    if(!*q)return beam_plugin_error(argc,argv,"Use dopus:<drawer> to open a drawer in Directory Opus.");
    rc=open_drawer(q,NULL);
    return rc?beam_plugin_error(argc,argv,"Directory Opus failed or did not answer. Check plugin Settings."):
        beam_plugin_text(argc,argv,"Opened in Directory Opus.");
}
