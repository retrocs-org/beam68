/* Native fake-service tests. Never contacts or changes Home Assistant. */
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#define BEAM_AMIHASS_QA
#define main plugin_main
#include "../../Plugins/AmiHASS/AmiHASS.c"
#undef main
static int rows,errors,actions,confirm_answer=1,accept_only,offline,waiting,page_offset,mismatch;
static char labels[8][121],values[8][240];
static char state[40]="off";
int beam_amihass_qa_confirm(void){return confirm_answer;}
int beam68_plugin_input(int argc,char **argv,char *out,unsigned int cap){if(argc<2)return 0;beam68_copy(out,argv[1],cap);return 1;}
void beam68_copy(char *out,const char *in,unsigned int cap){if(cap){unsigned int n=(unsigned int)strlen(in);if(n>=cap)n=cap-1;memcpy(out,in,n);out[n]=0;}}
char *beam68_trim(char *s){return s;}
int beam68_prefix(const char *s,const char *p){for(;*p;++s,++p)if(tolower((unsigned char)*s)!=tolower((unsigned char)*p))return 0;return 1;}
int beam68_equal(const char *s,const char *p){return strlen(s)==strlen(p)&&beam68_prefix(s,p);}
void beam68_rows_begin(void){rows=0;errors=0;}
int beam68_row(int kind,const char *label,const char *value){(void)kind;if(rows==8)return 0;beam68_copy(labels[rows],label,121);beam68_copy(values[rows++],value,240);return 1;}
int beam68_rows_send(int argc,char **argv,int error){(void)argc;(void)argv;errors=error;return 0;}
void amihass_client_close(struct AmiHASSClient *c){memset(c,0,sizeof(*c));}
const char *amihass_problem(int result){return result==AMIHASS_WAIT?"Updating...":"Service error";}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *id,const char *action,int offset,int refresh,const char *domain,const char *search)
{
 struct AmiHASSMessage *m;int i;(void)owner;(void)refresh;(void)domain;(void)search;
 if(offline)return 0;
 m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return 0;
 m->result=waiting?AMIHASS_WAIT:AMIHASS_OK;
 if(kind==AMIHASS_LIST){page_offset=offset;m->total=20;m->count=offset<20?(20-offset<16?20-offset:16):0;
  for(i=0;i<m->count;++i){sprintf(m->items[i].id,"light.lamp%d",offset+i);sprintf(m->items[i].name,"Lamp %d",offset+i);strcpy(m->items[i].state,"off");}}
 else if(kind==AMIHASS_QUERY){beam68_copy(m->item.id,mismatch?"light.other":id,96);strcpy(m->item.name,"Test light");strcpy(m->item.state,state);}
 else if(kind==AMIHASS_ACTION){++actions;m->result=AMIHASS_WAIT;if(!accept_only)strcpy(state,!strcmp(action,"Turn off")?"off":"on");}
 else if(kind==AMIHASS_STATUS){strcpy(m->item.state,"sample");strcpy(m->status.server,"Sample service");}
 c->pending=m;return 1;
}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=c->pending;c->pending=NULL;return m;}
static int run(const char *q){char *argv[]={"test",(char *)q,NULL};return plugin_main(2,argv);}
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 20;}}while(0)
int main(void)
{
 CHECK(run("kitchen")==0&&rows==7&&!errors&&!actions);CHECK(strstr(values[0],"entity light.lamp0")!=NULL);CHECK(!strcmp(values[6],"ha:page 6 kitchen"));
 CHECK(run("page 6 kitchen")==0&&rows==8&&page_offset==6);CHECK(!strcmp(values[6],"ha:page 0 kitchen"));CHECK(!strcmp(values[7],"ha:page 12 kitchen"));
 CHECK(run("entity light.lamp0")==0&&rows==6&&!actions);CHECK(strstr(values[2],"do on light.lamp0")!=NULL);
 confirm_answer=0;CHECK(run("do on light.lamp0")==0&&!actions&&strstr(labels[0],"cancelled"));
 confirm_answer=1;CHECK(run("do on light.lamp0")==0&&actions==1&&!strcmp(labels[0],"Confirmed state: on"));
 strcpy(state,"off");accept_only=1;CHECK(run("do on light.lamp0")==0&&actions==2&&strstr(labels[0],"not confirmed"));
 CHECK(run("do on lock.front")==0&&errors&&actions==2);CHECK(run("do activate script.evening")==0&&actions==3&&strstr(labels[0],"not confirmed"));
 CHECK(run("status")==0&&strstr(labels[0],"sample"));
 mismatch=1;CHECK(run("entity light.lamp0")==0&&errors&&strstr(labels[0],"Mismatched"));mismatch=0;
 waiting=1;CHECK(run("entity light.lamp0")==0&&!errors&&actions==3&&strstr(labels[0],"retry"));CHECK(run("do on light.lamp0")==0&&!errors&&!strcmp(values[0],"ha:do on light.lamp0")&&actions==3);waiting=0;
 offline=1;CHECK(run("kitchen")==0&&errors&&strstr(labels[0],"unavailable"));offline=0;
 CHECK(run("page -2 kitchen")==0&&errors);CHECK(run("entity light.bad;command")==0&&errors&&actions==3);
 puts("PASS: AmiHASS search, pagination, states, status, confirmation, actions and failure handling.");return 0;
}
