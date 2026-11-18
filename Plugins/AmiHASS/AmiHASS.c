/* Local SDK IPC; device actions always require confirmation. */
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam68plugin.h"
#include "amihass.h"
static struct AmiHASSClient client;
static char input[121],original[121],query[121],text[121],filter[64],entity[96];
static struct AmiHASSEntity selected;
static struct AmiHASSMessage *request(int kind,const char *id,const char *action,int offset,int refresh,const char *search)
{
 struct AmiHASSMessage *m;int ticks;
 if(!amihass_send(&client,kind,"Beam68",id,action,offset,refresh,NULL,search))return NULL;
 for(ticks=0;ticks<75;++ticks){
  m=amihass_reply(&client);
  if(m){int i;m->item.id[95]=0;m->item.name[63]=0;m->item.state[39]=0;m->item.unit[15]=0;m->status.server[159]=0;
   for(i=0;i<m->count;++i){m->items[i].id[95]=0;m->items[i].name[63]=0;m->items[i].state[39]=0;m->items[i].unit[15]=0;}return m;}
  if(!client.pending || (SetSignal(0,0)&SIGBREAKF_CTRL_C))break;
  Delay(2);
 }
 amihass_client_close(&client);return NULL;
}
static void row(const char *label,const char *value){beam68_row(BEAM68_ROW_QUERY,label,value);}
static int finish(int argc,char **argv,int error){amihass_client_close(&client);return beam68_rows_send(argc,argv,error);}
static int problem(int argc,char **argv,struct AmiHASSMessage *m)
{
 int retry=m && (m->result==AMIHASS_WAIT||m->result==AMIHASS_BUSY);
 const char *s=m?amihass_problem(m->result):"AmiHASS unavailable or timed out. Start/configure AmiHASS first.";
 if(retry){
  snprintf(query,sizeof(query),"ha:%.117s",original);row("Updating/busy: press Enter to retry",query);
 }else beam68_row(BEAM68_ROW_TEXT,s,s);
 if(m)FreeVec(m);
 return finish(argc,argv,!retry);
}
static int valid_id(const char *s)
{
 const char *p=s;int dot=0;if(!s||!*s||strlen(s)>95)return 0;
 for(;*p;++p){if(*p=='.'){if(dot||p==s||!p[1])return 0;dot=1;}
  else if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='_'))return 0;}
 return dot;
}
static int switchable(const char *id)
{
 static const char *domains[]={"light.","switch.","fan.","input_boolean.","group.","media_player.",NULL};int i;
 for(i=0;domains[i];++i)if(beam68_prefix(id,domains[i]))return 1;
 return 0;
}
static int activatable(const char *id){return beam68_prefix(id,"scene.")||beam68_prefix(id,"script.");}
static void action_row(const char *name,const char *action)
{snprintf(query,sizeof(query),"ha:do %s %s",action,entity);row(name,query);}
static int confirm_action(const char *action,const char *id)
{
#ifdef BEAM_AMIHASS_QA
 extern int beam_amihass_qa_confirm(void);
 (void)action;(void)id;
 return beam_amihass_qa_confirm()==1;
#else
 static struct EasyStruct prompt={sizeof(struct EasyStruct),0,"Beam68 - AmiHASS","%s\n%s\nSend this action?","Send|Cancel"};
 ULONG args[2];args[0]=(ULONG)action;args[1]=(ULONG)id;
 return EasyRequestArgs(NULL,&prompt,NULL,args)==1;
#endif
}
int main(int argc,char **argv)
{
 struct AmiHASSMessage *m;char *s,*space;int i,offset=0,mode=0;
 const char *action=NULL,*expected=NULL;
 if(!beam68_plugin_input(argc,argv,input,sizeof(input)))return 20;
 beam68_copy(original,input,sizeof(original));
 beam68_rows_begin();s=beam68_trim(input);
 if(beam68_equal(s,"status")){
  m=request(AMIHASS_STATUS,NULL,NULL,0,0,NULL);if(!m||m->result!=AMIHASS_OK)return problem(argc,argv,m);
  snprintf(text,sizeof(text),"AmiHASS: %s",m->item.state[0]?m->item.state:(m->status.connected?"connected":"disconnected"));beam68_row(BEAM68_ROW_TEXT,text,text);
  snprintf(text,sizeof(text),"Server: %.110s",m->status.server);beam68_row(BEAM68_ROW_TEXT,text,text);FreeVec(m);return finish(argc,argv,0);
 }
 if(beam68_prefix(s,"do ")){
  s+=3;space=strchr(s,' ');if(!space)return problem(argc,argv,NULL);*space++=0;
  if(beam68_equal(s,"on"))action="Turn on";else if(beam68_equal(s,"off"))action="Turn off";
  else if(beam68_equal(s,"toggle"))action="Toggle";else if(beam68_equal(s,"activate"))action="Activate";
  if(!action)return problem(argc,argv,NULL);
  s=space;mode=1;
 }else if(beam68_prefix(s,"entity ")){s+=7;mode=2;}
 if(mode){
  if(!valid_id(s))return problem(argc,argv,NULL);
  beam68_copy(entity,s,sizeof(entity));
  m=request(AMIHASS_QUERY,entity,NULL,0,AMIHASS_QUERY_ONCE,NULL);
  if(!m||m->result!=AMIHASS_OK)return problem(argc,argv,m);
  if(strcmp(m->item.id,entity)){
   FreeVec(m);beam68_row(BEAM68_ROW_TEXT,"Mismatched AmiHASS entity reply.","");return finish(argc,argv,1);
  }
  selected=m->item;FreeVec(m);
  if(mode==1){
   if((!strcmp(action,"Activate")&&!activatable(entity))||(strcmp(action,"Activate")&&!switchable(entity))||!strcmp(selected.state,"unavailable")||!strcmp(selected.state,"unknown")){
    beam68_row(BEAM68_ROW_TEXT,"No supported action for this entity/state.","");return finish(argc,argv,1);}
   if(!confirm_action(action,entity)){
    beam68_row(BEAM68_ROW_TEXT,"Action cancelled.","");return finish(argc,argv,0);
   }
   if(!strcmp(action,"Turn on"))expected="on";else if(!strcmp(action,"Turn off"))expected="off";
   else if(!strcmp(action,"Toggle")){if(!strcmp(selected.state,"on"))expected="off";else if(!strcmp(selected.state,"off"))expected="on";}
   m=request(AMIHASS_ACTION,entity,action,0,0,NULL);
   if(!m||(m->result!=AMIHASS_WAIT&&m->result!=AMIHASS_OK))return problem(argc,argv,m);
   FreeVec(m);
   if(expected)for(i=0;i<4;++i){
    m=request(AMIHASS_QUERY,entity,NULL,0,i?AMIHASS_QUERY_ONCE:AMIHASS_QUERY_FRESH,NULL);if(!m)break;
    if(m->result==AMIHASS_OK&&!strcmp(m->item.id,entity)&&!strcmp(m->item.state,expected)){
     snprintf(text,sizeof(text),"Confirmed state: %s",expected);FreeVec(m);beam68_row(BEAM68_ROW_TEXT,text,text);return finish(argc,argv,0);}
    FreeVec(m);Delay(25);
   }
   beam68_row(BEAM68_ROW_TEXT,"Action accepted; completion not confirmed. Refresh to check.","");
   snprintf(query,sizeof(query),"ha:entity %s",entity);row("Refresh entity state",query);return finish(argc,argv,0);
  }
  snprintf(text,sizeof(text),"%.60s: %.35s %.15s",selected.name,selected.state,selected.unit);beam68_row(BEAM68_ROW_TEXT,text,text);beam68_row(BEAM68_ROW_TEXT,entity,entity);
  if(strcmp(selected.state,"unavailable")&&strcmp(selected.state,"unknown")){
   if(switchable(entity)){action_row("Turn on...","on");action_row("Turn off...","off");action_row("Toggle...","toggle");}
   else if(activatable(entity))action_row("Activate...","activate");}
  snprintf(query,sizeof(query),"ha:entity %s",entity);row("Refresh state",query);return finish(argc,argv,0);
 }
 if(beam68_prefix(s,"page ")){char *end;long n;s+=5;n=strtol(s,&end,10);
  if(end==s||*end!=' '||n<0||n>100000)return problem(argc,argv,NULL);
  offset=(int)n;s=end+1;}
 if(strlen(s)>=sizeof(filter)){beam68_row(BEAM68_ROW_TEXT,"Use a search of at most 63 characters.","");return finish(argc,argv,1);}
 beam68_copy(filter,s,sizeof(filter));m=request(AMIHASS_LIST,NULL,NULL,offset,0,filter);
 if(!m||m->result!=AMIHASS_OK)return problem(argc,argv,m);
 for(i=0;i<m->count&&i<6;++i){if(!valid_id(m->items[i].id))continue;
  snprintf(text,sizeof(text),"%.60s: %.35s %.15s",m->items[i].name,m->items[i].state,m->items[i].unit);
  snprintf(query,sizeof(query),"ha:entity %s",m->items[i].id);row(text,query);}
 if(offset){snprintf(query,sizeof(query),"ha:page %d %s",offset>6?offset-6:0,filter);row("Previous results",query);}
 if(offset+6<m->total){snprintf(query,sizeof(query),"ha:page %d %s",offset+6,filter);row("Next results",query);}
 if(!m->count)beam68_row(BEAM68_ROW_TEXT,"No matching entities. Check ha:status if unexpected.","");
 FreeVec(m);return finish(argc,argv,0);
}
