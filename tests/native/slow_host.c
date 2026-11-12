/* Test-only ARexx host: one deliberately late reply, no system actions. */
#include <exec/types.h>
#include <exec/ports.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
struct RxsLib *RexxSysBase;
int main(void)
{
    struct MsgPort *port;struct RexxMsg *m=NULL;int i,occupied;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);if(!RexxSysBase)return 20;
    port=CreateMsgPort();if(!port){CloseLibrary((struct Library *)RexxSysBase);return 20;}
    port->mp_Node.ln_Name=(STRPTR)"DOPUS.1";
    Forbid();occupied=FindPort("DOPUS.1")!=NULL;if(!occupied)AddPort(port);Permit();
    if(occupied){DeleteMsgPort(port);CloseLibrary((struct Library *)RexxSysBase);return 10;}
    puts("Test host waiting (no commands are executed)");fflush(stdout);
    for(i=0;i<3000;i++){m=(struct RexxMsg *)GetMsg(port);if(m)break;Delay(1);}
    if(m){
        printf("received valid=%ld command=%s\n",(long)IsRexxMsg(m),m->rm_Args[0]);fflush(stdout);
        Delay(250);m->rm_Result1=0;m->rm_Result2=(LONG)CreateArgstring("42",2);
        ReplyMsg((struct Message *)m);puts("Late reply returned safely");
    }
    Forbid();RemPort(port);Permit();
    while((m=(struct RexxMsg *)GetMsg(port))){m->rm_Result1=20;m->rm_Result2=0;ReplyMsg((struct Message *)m);}
    DeleteMsgPort(port);CloseLibrary((struct Library *)RexxSysBase);return m?0:0;
}
