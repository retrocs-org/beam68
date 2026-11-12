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
int main(int argc,char **argv)
{
    struct MsgPort *reply,*host;struct RexxMsg *m;int i,rc=20;
    if(argc!=2 || (strcmp(argv[1],"TIMING") && strcmp(argv[1],"INDEX") && strcmp(argv[1],"PLUGINS")))return 10;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);if(!RexxSysBase)return 20;
    reply=CreateMsgPort();if(!reply){CloseLibrary((struct Library *)RexxSysBase);return 20;}
    m=beam_rexx_message_create(reply,argv[1],1);
    if(!m){DeleteMsgPort(reply);CloseLibrary((struct Library *)RexxSysBase);return 20;}
    printf("rexxsyslib=%u.%u IsRexxMsg=%ld\n",((struct Library *)RexxSysBase)->lib_Version,((struct Library *)RexxSysBase)->lib_Revision,(long)IsRexxMsg(m));
    Forbid();host=FindPort("BEAM68");if(host)PutMsg(host,(struct Message *)m);Permit();
    if(host) {
        for(i=0;i<250;i++){if(GetMsg(reply))break;Delay(1);}
        if(i==250){Forbid();reply->mp_Flags=PA_IGNORE;Permit();puts("Probe timeout");return 20;}
        rc=(int)m->rm_Result1;
        printf("%s: %s\n",argv[1],!rc && m->rm_Result2?(char *)m->rm_Result2:"(no result)");
        if(!rc && m->rm_Result2)DeleteArgstring((UBYTE *)m->rm_Result2);
    } else puts("BEAM68 port missing");
    beam_rexx_message_delete(m);DeleteMsgPort(reply);CloseLibrary((struct Library *)RexxSysBase);
    return rc;
}
