#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

static void append_name(char *out,unsigned int cap,const char *name)
{
    unsigned int n=(unsigned int)strlen(out),i=0;
    if (!name || !*name || n>95) return;
    if (n && n+2<cap) { out[n++]=',';out[n++]=' '; }
    while (name[i] && n+1<cap) out[n++]=name[i++];
    out[n]=0;
}

static void list_tasks(char *out,unsigned int cap)
{
    struct Node *node;
    struct Task *current=FindTask(NULL);
    /* Exec moves tasks between TaskReady and TaskWait from interrupts
       (Signal()), so Forbid() is not enough to walk them safely. The walk
       only copies a few short names, keeping the Disable() window brief. */
    out[0]=0;Disable();
    append_name(out,cap,current->tc_Node.ln_Name?current->tc_Node.ln_Name:"(current)");
    node=SysBase->TaskReady.lh_Head;
    while (node && node->ln_Succ) { append_name(out,cap,node->ln_Name);node=node->ln_Succ; }
    node=SysBase->TaskWait.lh_Head;
    while (node && node->ln_Succ) { append_name(out,cap,node->ln_Name);node=node->ln_Succ; }
    Enable();
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121],name[80];
    char *s,*end;
    struct Task *task;
    unsigned int n;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (beam_ascii_prefix(s,"task:")) s=beam_plugin_trim(s+5);
    if (!*s || beam_ascii_equal(s,"tasks") || beam_ascii_equal(s,"list")) {
        list_tasks(out,sizeof(out));
        return beam_plugin_text(argc,argv,*out?out:plugin_str(PLMSG_TASK_NONE));
    }
    if (!beam_ascii_prefix(s,"break:")) return beam_plugin_error(argc,argv,plugin_str(PLMSG_TASK_USAGE));
    s+=6;end=strrchr(s,':');
    if (!end || !beam_ascii_equal(end+1,"confirm"))
        return beam_plugin_error(argc,argv,plugin_str(PLMSG_TASK_CONFIRM));
    *end=0;s=beam_plugin_trim(s);n=(unsigned int)strlen(s);
    if (!n || n>=sizeof(name)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_TASK_NAME));
    beam_plugin_copy(name,s,sizeof(name));
    Forbid();task=FindTask(name);
    if (task && task!=FindTask(NULL)) Signal(task,SIGBREAKF_CTRL_C);
    Permit();
    if (!task) return beam_plugin_error(argc,argv,plugin_str(PLMSG_TASK_NOT_FOUND));
    if (task==FindTask(NULL)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_TASK_SELF));
    sprintf(out,plugin_str(PLMSG_TASK_SENT),name);
    return beam_plugin_text(argc,argv,out);
}
