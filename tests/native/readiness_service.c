/* One read-only fake AmiHASS STATUS response; no Home Assistant connection. */
#include <exec/types.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "../../Plugins/AmiHASS/amihass.h"
int main(int argc,char **argv)
{
    struct MsgPort *port=CreateMsgPort();struct AmiHASSMessage *m=NULL;int i;
    if(!port)return 20;
    port->mp_Node.ln_Name=AMIHASS_PORT;
    Forbid();if(FindPort(AMIHASS_PORT)){Permit();DeleteMsgPort(port);return 20;}
    AddPort(port);Permit();
    for(i=0;i<500 && !(m=(struct AmiHASSMessage *)GetMsg(port));i++)Delay(1);
    if(m){m->result=argc>1 && argv[1][0]=='u'?AMIHASS_UNCONFIGURED:AMIHASS_OK;
        m->status.connected=m->result==AMIHASS_OK;m->count=m->total=0;
        if(m->kind!=AMIHASS_STATUS)m->result=AMIHASS_UNSUPPORTED;
        PutMsg(m->msg.mn_ReplyPort,(struct Message *)m);}
    RemPort(port);DeleteMsgPort(port);return m?0:20;
}
