#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "control.h"
static struct MsgPort launcher,*reply_port;
static int running=1,forbidden,allocations,ports,fail_alloc,mode,delays;
static const char *reported_version;
void *AllocVec(unsigned long size,unsigned long flags)
{ void *p;(void)flags;assert(!forbidden);if(fail_alloc)return NULL;p=calloc(1,size);if(p)allocations++;return p; }
void FreeVec(void *p){assert(!forbidden);if(p){allocations--;free(p);}}
void Forbid(void){assert(!forbidden);forbidden=1;}
void Permit(void){assert(forbidden);forbidden=0;}
void *FindTask(STRPTR name){(void)name;return &launcher;}
struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));assert(!forbidden && p);ports++;return p;}
void DeleteMsgPort(struct MsgPort *p){assert(!forbidden && !p->test_head);ports--;free(p);}
void AddPort(struct MsgPort *p){assert(forbidden && !reply_port);reply_port=p;}
void RemPort(struct MsgPort *p){assert(forbidden && p==reply_port);reply_port=NULL;}
struct MsgPort *FindPort(STRPTR name){assert(forbidden);if(!strcmp(name,BEAM_PORT_NAME))return running?&launcher:NULL;return reply_port && !strcmp(name,reply_port->mp_Node.ln_Name)?reply_port:NULL;}
void PutMsg(struct MsgPort *p,struct Message *m)
{
 assert(forbidden && p && m);m->mn_Node.ln_Succ=NULL;
 if(p!=&launcher && reported_version)strcpy(((struct BeamVersionReply *)m)->launcher_version,reported_version);
 if(p->test_tail)p->test_tail->mn_Node.ln_Succ=(struct Node *)m;else p->test_head=m;
 p->test_tail=m;
}
struct Message *GetMsg(struct MsgPort *p)
{ struct Message *m=p->test_head;if(m){p->test_head=(struct Message *)m->mn_Node.ln_Succ;if(!p->test_head)p->test_tail=NULL;}return m; }
void ReplyMsg(struct Message *m){(void)m;assert(0);}
void DateStamp(struct DateStamp *s){memset(s,0,sizeof(*s));s->ds_Days=100;s->ds_Tick=10;}
static void service(void)
{
 struct Message *m;
 while((m=GetMsg(&launcher))) {
  struct BeamVersionRequest *request=(struct BeamVersionRequest *)m;
  assert(!m->mn_ReplyPort && request->control.command==BEAM_CTL_QUERY_VERSION);
  if(mode!=2) { if(mode==3)request->serial++;beam_control_reply_version(&request->control); }
  FreeVec(m);
 }
}
void Delay(unsigned long ticks){assert(ticks==1 && !forbidden);delays++;if(mode!=1)service();}
static void clean(void){assert(!allocations && !ports && !reply_port && !launcher.test_head && !forbidden);delays=0;reported_version=NULL;mode=0;}
int main(void)
{
 char version[64];struct BeamVersionRequest malformed;
 running=0;assert(beam_control_version(version,sizeof(version))==0 && !*version);clean();
 running=1;assert(beam_control_version(version,sizeof(version))==1 && !strcmp(version,BEAM68_VERSION));assert(delays==1);clean();
 reported_version="0.1";assert(beam_control_version(version,sizeof(version))==1 && !strcmp(version,"0.1"));clean();
 mode=2;assert(beam_control_version(version,sizeof(version))==-1 && !*version);assert(delays==25);clean();
 mode=3;assert(beam_control_version(version,sizeof(version))==-1 && !*version);clean();
 mode=1;assert(beam_control_version(version,sizeof(version))==-1);assert(allocations==1 && !ports);service();clean();
 fail_alloc=1;assert(beam_control_version(version,sizeof(version))==-1);fail_alloc=0;clean();
 memset(&malformed,0,sizeof(malformed));beam_control_reply_version(&malformed.control);clean();
 malformed.control.message.mn_Length=sizeof(malformed);memset(malformed.reply_port,'x',sizeof(malformed.reply_port));beam_control_reply_version(&malformed.control);clean();
 assert(beam_control_version(NULL,0)==-1);
 puts("Running launcher version: live reply, distinct version, old/busy launcher, timeout cleanup and late reply: PASS");return 0;
}
