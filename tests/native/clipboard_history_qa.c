/* Actual ClipboardTools process and clipboard.device, disposable guest only. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam68plugin.h"
#include "clip.h"
#include "stack_guard.h"
static struct MsgPort *control,*replies;static unsigned long serial;
static int check(int ok,const char *s){printf("%s: %s\n",ok?"PASS":"FAIL",s);return ok;}
static struct Beam68ReplyMessage *request(const char *text,int background)
{
    char hex[241],command[500];unsigned int i;int ticks;static const char digits[]="0123456789ABCDEF";
    for(i=0;text[i];i++){unsigned char c=(unsigned char)text[i];hex[2*i]=digits[c>>4];hex[2*i+1]=digits[c&15];}hex[2*i]=0;
    sprintf(command,"%s\"Builds:ClipboardTools\" %s NIL: BEAM68_REPLY %lu",background?"Run >NIL: ":"",hex,++serial);
    if(SystemTags((STRPTR)command,NP_StackSize,32768UL,TAG_DONE)!=0)return NULL;
    for(ticks=0;ticks<200;ticks++){
        struct Beam68ReplyMessage *m=(struct Beam68ReplyMessage *)GetMsg(replies);
        if(m)return m;
        Delay(1);
    }
    return NULL;
}
static int history_port(void){int found;Forbid();found=FindPort("BEAM68.CLIPBOARD.HISTORY")!=NULL;Permit();return found;}
static int qa(int argc,char **argv)
{
    struct Beam68ReplyMessage *m;char restored[128],id[40],restore[80];int rc=20,ticks;BPTR l;
    (void)argc;(void)argv;
    if(FindPort("BEAM68.CONTROL") || FindPort(BEAM68_REPLY_PORT))return 20;
    control=CreateMsgPort();replies=CreateMsgPort();if(!control || !replies)goto cleanup;
    control->mp_Node.ln_Name="BEAM68.CONTROL";replies->mp_Node.ln_Name=BEAM68_REPLY_PORT;AddPort(control);AddPort(replies);
    m=request("history status",0);if(!check(m && strstr(m->rows[0].text,"off"),"Recording off by default"))goto cleanup;FreeVec(m);
    if(!beam_clip_write("baseline before opt-in"))goto cleanup;
    m=request("history on",1);if(!check(m && history_port(),"Explicit opt-in starts recorder and acknowledges promptly"))goto cleanup;FreeVec(m);Delay(60);
    if(!beam_clip_write("clipboard history native fixture"))goto cleanup;
    Delay(70);
    m=request("history fixture",0);
    if(!check(m && m->count==1 && m->rows[0].kind==BEAM68_ROW_QUERY && strstr(m->rows[0].value,"clip:restore "),"Captured text searchable through plugin rows"))goto cleanup;
    strcpy(id,m->rows[0].value+13);FreeVec(m);sprintf(restore,"restore %s",id);
    if(!beam_clip_write("replacement fixture"))goto cleanup;
    m=request(restore,0);if(!check(m && strstr(m->rows[0].text,"restored"),"Explicit restore command succeeds"))goto cleanup;FreeVec(m);
    if(!check(beam_clip_read(restored,sizeof(restored)) && !strcmp(restored,"clipboard history native fixture"),"Restored clipboard preserves original text"))goto cleanup;
    m=request("history clear",0);if(!m)goto cleanup;FreeVec(m);Delay(70);
    if(!beam_clip_write("new after clear"))goto cleanup;
    Delay(70);
    m=request(restore,0);if(!check(m && strstr(m->rows[0].text,"expired or was cleared"),"Old result ID cannot restore another entry after clearing"))goto cleanup;FreeVec(m);
    m=request("history off",0);if(!check(m && !history_port(),"Stop waits for recorder exit"))goto cleanup;FreeVec(m);
    l=Lock("RAM:Beam68-ClipboardHistory",ACCESS_READ);if(l)UnLock(l);if(!check(!l,"Stop removes temporary history"))goto cleanup;
    m=request("history on",1);if(!m)goto cleanup;FreeVec(m);Delay(60);
    RemPort(control);DeleteMsgPort(control);control=NULL;
    for(ticks=0;ticks<180 && history_port();ticks++)Delay(1);
    if(!check(!history_port(),"Recorder stops when launcher control port disappears"))goto cleanup;
    rc=0;
cleanup:
    Forbid();{struct MsgPort *p=FindPort("BEAM68.CLIPBOARD.HISTORY");if(p)Signal(p->mp_SigTask,SIGBREAKF_CTRL_C);}Permit();
    for(ticks=0;ticks<180 && history_port();ticks++)Delay(1);
    if(control){RemPort(control);DeleteMsgPort(control);}
    if(replies){struct Message *p;while((p=GetMsg(replies)))FreeVec(p);RemPort(replies);DeleteMsgPort(replies);}
    puts(rc?"Clipboard recorder QA: FAIL":"Clipboard recorder QA: PASS");return rc;
}
int main(int argc,char **argv){return beam_run_with_stack(32768UL,qa,argc,argv);}
