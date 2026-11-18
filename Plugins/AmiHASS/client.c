/* Adapted from AmiHASS SDK; see LICENSE. No service auto-start. */
/* Bounded asynchronous local requests. Late replies own their abandoned port. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "amihass.h"
#include "nd_reply.h"
static unsigned long ticks(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*3000UL+(unsigned long)d.ds_Tick;}
void amihass_erase(void *p,unsigned long n)
{volatile unsigned char *v=p;while(n--)*v++=0;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter)
{
 struct AmiHASSMessage *m;struct MsgPort *server;int sent=0;
 if(!c || c->pending)return 0;
 if(!c->port)c->port=nd_reply_port();
 if(!c->port)return 0;
 m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return 0;
 m->magic=AMIHASS_MAGIC;m->abi=AMIHASS_ABI;m->kind=kind;m->offset=offset;m->refresh=refresh;
 if(owner){strncpy(m->owner,owner,sizeof(m->owner)-1);}if(entity){strncpy(m->entity,entity,sizeof(m->entity)-1);}if(action){strncpy(m->action,action,sizeof(m->action)-1);}
 if(domain)strncpy(m->domain,domain,sizeof(m->domain)-1);
 if(filter)strncpy(m->filter,filter,sizeof(m->filter)-1);
 m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=c->port;
 Forbid();server=(kind!=AMIHASS_STOP && kind!=AMIHASS_STATUS && kind!=AMIHASS_DIAGNOSTICS && FindPort("AmiHASS.Maintenance")) ? NULL:FindPort(AMIHASS_PORT);if(server){PutMsg(server,(struct Message *)m);sent=1;}Permit();
 if(!sent){FreeVec(m);return 0;}c->pending=m;c->started=ticks();return 1;
}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c)
{struct AmiHASSMessage *m;if(!c || !c->port)return NULL;m=(struct AmiHASSMessage *)GetMsg(c->port);if(m){c->pending=NULL;if(m->magic!=AMIHASS_MAGIC || m->abi!=AMIHASS_ABI || m->msg.mn_Length!=sizeof(*m) || m->count<0 || m->count>AMIHASS_PAGE || m->total<0){FreeVec(m);return NULL;}return m;}if(c->pending && ticks()-c->started>500UL)amihass_client_close(c);return NULL;}
void amihass_client_close(struct AmiHASSClient *c)
{
 struct AmiHASSMessage *m;if(!c || !c->port)return;
 Forbid();while((m=(struct AmiHASSMessage *)GetMsg(c->port))!=NULL){c->pending=NULL;FreeVec(m);}
 if(c->pending)nd_reply_abandon(c->port);else nd_reply_delete(c->port);
 Permit();memset(c,0,sizeof(*c));
}
const char *amihass_problem(int r)
{switch(r){case 9:return "Install/start AmiHASS service";case AMIHASS_OK:return "Connected";case AMIHASS_UNAVAILABLE:return "Entity unavailable; no action sent";case AMIHASS_WAIT:return "Updating...";case AMIHASS_UNCONFIGURED:return "Set up Home Assistant connection";case AMIHASS_STALE:return "Stale; last reading is not current";case AMIHASS_BUSY:return "Busy; try again shortly";case AMIHASS_UNSUPPORTED:return "Action is not supported for this entity";default:return "Connection/authentication failed; check server, token and curl";}}
