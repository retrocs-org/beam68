/* Opt-in live test. Entity must be supplied explicitly by the test operator.
 * No credentials, server addresses, entity names or response bodies are logged. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "../../Plugins/AmiHASS/amihass.h"
static struct AmiHASSClient client;
static FILE *qa_report;
static void report(const char *s){puts(s);if(qa_report){fprintf(qa_report,"%s\n",s);fflush(qa_report);}}
static int copy_setup(const char *source,const char *target)
{
 FILE *in=fopen(source,"rb"),*out;unsigned char data[256];size_t n;int ok=1;
 if(!in)return 0;
 out=fopen(target,"wb");if(!out){fclose(in);return 0;}
 while((n=fread(data,1,sizeof(data),in))>0)if(fwrite(data,1,n,out)!=n){ok=0;break;}
 if(ferror(in))ok=0;
 fclose(in);if(fclose(out))ok=0;
 {volatile unsigned char *p=data;unsigned int i;for(i=0;i<sizeof(data);++i)p[i]=0;}
 return ok;
}
static int setup(void)
{
 BPTR dir,in,out;LONG rc;
 dir=CreateDir("ENVARC:AmiHASS");if(dir)UnLock(dir);
 if(!copy_setup("Builds:AmiHASS.cfg","ENVARC:AmiHASS/AmiHASS.cfg") || !copy_setup("Builds:curl","C:curl"))return 0;
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(!in||!out){if(in)Close(in);if(out)Close(out);return 0;}
 rc=SystemTags("Builds:AmiHASSService SERVICE",SYS_Asynch,TRUE,SYS_Input,in,SYS_Output,out,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(in);Close(out);return 0;}return 1;
}
static struct AmiHASSMessage *request(int kind,const char *id,const char *action,int fresh)
{
 struct AmiHASSMessage *m;int i;
 if(!amihass_send(&client,kind,"Beam68LiveQA",id,action,0,fresh,NULL,NULL))return NULL;
 for(i=0;i<250;++i){m=amihass_reply(&client);if(m)return m;if(!client.pending)break;Delay(2);}
 amihass_client_close(&client);return NULL;
}
static int state_is(const char *id,const char *state,int fresh)
{
 int i;struct AmiHASSMessage *m;
 for(i=0;i<80;++i){
  m=request(AMIHASS_QUERY,id,NULL,i?AMIHASS_QUERY_ONCE:fresh);
  if(!m)return 0;
  if(m->result==AMIHASS_OK&&!strcmp(m->item.id,id)&&!strcmp(m->item.state,state)){FreeVec(m);return 1;}
  FreeVec(m);Delay(25);
 }return 0;
}
static int action(const char *id,const char *what)
{
 struct AmiHASSMessage *m=request(AMIHASS_ACTION,id,what,0);int ok;
 if(!m)return 0;
 ok=m->result==AMIHASS_OK||m->result==AMIHASS_WAIT;FreeVec(m);return ok;
}
int main(int argc,char **argv)
{
 struct AmiHASSMessage *m;int i,ok=0,touched=0,restored=1;
 if(argc!=3 || strncmp(argv[1],"light.",6) || strcmp(argv[2],"off"))return 20;
 qa_report=fopen("AutoRun:amihass-live-proof.txt","wb");if(!qa_report)return 20;
 report("Native live test started; no shell script wrapper.");
 if(!setup()){report("FAIL: private service setup.");fclose(qa_report);return 20;}
 for(i=0;i<100;++i){m=request(AMIHASS_STATUS,NULL,NULL,0);if(m){int connected=m->result==AMIHASS_OK&&m->status.connected;FreeVec(m);if(connected)break;}Delay(25);}
 if(i==100){report("FAIL: live Home Assistant connection not established.");goto cleanup;}
 report("PASS: live authenticated service connection.");
 for(i=0;i<80;++i){m=request(AMIHASS_LIST,NULL,NULL,0);if(m){int found=m->result==AMIHASS_OK&&m->count>0;FreeVec(m);if(found)break;}Delay(25);}
 if(i==80){report("FAIL: live entity discovery.");goto cleanup;}
 report("PASS: live entity discovery.");
 if(!state_is(argv[1],"off",AMIHASS_QUERY_FRESH)){report("FAIL: starting light state changed or unavailable; no action sent.");goto cleanup;}
 report("PASS: live query and original off state verified.");
 touched=1;
 if(!action(argv[1],"Turn on")||!state_is(argv[1],"on",AMIHASS_QUERY_FRESH)){report("FAIL: turn-on not confirmed.");goto cleanup;}
 report("PASS: authorized light turn-on confirmed by live state.");ok=1;
cleanup:
 if(touched){restored=action(argv[1],"Turn off")&&state_is(argv[1],"off",AMIHASS_QUERY_FRESH);
  report(restored?"PASS: original light state restored and confirmed.":"FAIL: original light state not confirmed; restore manually.");}
 m=request(AMIHASS_STOP,NULL,NULL,0);if(m)FreeVec(m);amihass_client_close(&client);
 fclose(qa_report);return ok&&restored?0:20;
}
