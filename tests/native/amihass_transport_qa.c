/* Exercise the real AmiHASS SDK transport using an isolated local fake port. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include "../../Plugins/AmiHASS/amihass.h"
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 20;}}while(0)
int main(void)
{
 struct MsgPort *server,*orphan;
 struct AmiHASSClient c;
 struct AmiHASSMessage *m,*reply;
 memset(&c,0,sizeof(c));
 CHECK(!amihass_send(&c,AMIHASS_LIST,"Beam68",NULL,NULL,0,0,NULL,"kitchen"));
 amihass_client_close(&c);
 server=CreateMsgPort();CHECK(server!=NULL);
 server->mp_Node.ln_Name=(STRPTR)AMIHASS_PORT;AddPort(server);
 CHECK(amihass_send(&c,AMIHASS_LIST,"Beam68",NULL,NULL,6,0,NULL,"kitchen"));
 CHECK(!amihass_send(&c,AMIHASS_STATUS,"Beam68",NULL,NULL,0,0,NULL,NULL));
 m=(struct AmiHASSMessage *)GetMsg(server);CHECK(m!=NULL);
 CHECK(m->magic==AMIHASS_MAGIC&&m->abi==1&&m->offset==6&&!strcmp(m->filter,"kitchen"));
 m->result=AMIHASS_OK;m->count=1;m->total=1;strcpy(m->items[0].id,"light.kitchen");ReplyMsg((struct Message *)m);
 reply=amihass_reply(&c);CHECK(reply==m&&reply->count==1&&!c.pending);FreeVec(reply);
 CHECK(amihass_send(&c,AMIHASS_STATUS,"Beam68",NULL,NULL,0,0,NULL,NULL));
 m=(struct AmiHASSMessage *)GetMsg(server);CHECK(m!=NULL);m->count=17;ReplyMsg((struct Message *)m);
 CHECK(amihass_reply(&c)==NULL&&!c.pending);
 CHECK(amihass_send(&c,AMIHASS_STATUS,"Beam68",NULL,NULL,0,0,NULL,NULL));
 m=(struct AmiHASSMessage *)GetMsg(server);CHECK(m!=NULL);orphan=m->msg.mn_ReplyPort;
 c.started-=501UL;CHECK(amihass_reply(&c)==NULL&&!c.port&&!c.pending);
 /* Same reclamation contract used by the service's nd_reply_finish. */
 CHECK(orphan->mp_Flags==PA_IGNORE&&orphan->mp_SigTask==NULL&&orphan->mp_Node.ln_Pri==-101);
 FreeVec(m);FreeVec(orphan);
 amihass_client_close(&c);RemPort(server);DeleteMsgPort(server);
 puts("PASS: AmiHASS native IPC envelopes, single-request limit, malformed replies and timeout ownership.");return 0;
}
