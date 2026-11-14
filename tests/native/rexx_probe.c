/* Read-only live diagnostics without requiring the RX interpreter. */
#include <exec/types.h>
#include <exec/ports.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
struct RxsLib *RexxSysBase;
#include "rexx_message.h"
#include "rexx_commands.h"
int main(int argc,char **argv)
{
    struct MsgPort *reply,*host;struct RexxMsg *m;int i,rc=20;
    struct BeamRexxCommand parsed;
    char command[64];
    if (argc<2 || argc>3 || strlen(argv[1])+(argc==3?strlen(argv[2])+1:0)>=sizeof(command)) return 10;
    strcpy(command,argv[1]);
    if (argc==3) { strcat(command," ");strcat(command,argv[2]); }
    if (!beam_rexx_parse(command,&parsed)) return 10;
    switch (parsed.action) {
        case BEAM_REXX_STATUS:case BEAM_REXX_VERSION:case BEAM_REXX_HELP:
        case BEAM_REXX_TIMING:case BEAM_REXX_INDEX:case BEAM_REXX_PLUGINS:
        case BEAM_REXX_PLUGIN:case BEAM_REXX_REQUESTS:case BEAM_REXX_REQUEST:break;
        default:return 10; /* Never send commands which change launcher state. */
    }
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);if(!RexxSysBase)return 20;
    reply=CreateMsgPort();if(!reply){CloseLibrary((struct Library *)RexxSysBase);return 20;}
    m=beam_rexx_message_create(reply,command,1);
    if(!m){DeleteMsgPort(reply);CloseLibrary((struct Library *)RexxSysBase);return 20;}
    printf("rexxsyslib=%u.%u IsRexxMsg=%ld\n",((struct Library *)RexxSysBase)->lib_Version,((struct Library *)RexxSysBase)->lib_Revision,(long)IsRexxMsg(m));
    Forbid();host=FindPort("BEAM68");if(host)PutMsg(host,(struct Message *)m);Permit();
    if(host) {
        for(i=0;i<250;i++){if(GetMsg(reply))break;Delay(1);}
        if(i==250){Forbid();reply->mp_Flags=PA_IGNORE;Permit();puts("Probe timeout");return 20;}
        rc=(int)m->rm_Result1;
        printf("%s: %s\n",command,!rc && m->rm_Result2?(char *)m->rm_Result2:"(no result)");
        if(!rc && m->rm_Result2)DeleteArgstring((UBYTE *)m->rm_Result2);
    } else puts("BEAM68 port missing");
    beam_rexx_message_delete(m);DeleteMsgPort(reply);CloseLibrary((struct Library *)RexxSysBase);
    return rc;
}
