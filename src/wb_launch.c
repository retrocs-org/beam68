/* Minimal WBStartup launcher for AmigaOS 3.0/3.1 (workbench.library < V44). */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <string.h>
#include "wb_launch.h"

#define WB_LAUNCH_ARGS 2
#define WB_LAUNCH_NAME 108
#define WB_LAUNCH_MIN_STACK 4096UL

struct BeamWBLaunch {
    struct WBStartup startup;
    struct WBArg args[WB_LAUNCH_ARGS];
    char names[WB_LAUNCH_ARGS][WB_LAUNCH_NAME];
    char process_name[WB_LAUNCH_NAME];
};

static int outstanding;

int beam_wb_launch_outstanding(void) { return outstanding; }

static void free_launch(struct BeamWBLaunch *l)
{
    int i;
    if (!l) return;
    for (i=0;i<WB_LAUNCH_ARGS;i++) if (l->args[i].wa_Lock) UnLock(l->args[i].wa_Lock);
    if (l->startup.sm_Segment) UnLoadSeg(l->startup.sm_Segment);
    FreeVec(l);
}

void beam_wb_launch_reply(struct Message *msg)
{
    struct BeamWBLaunch *l=(struct BeamWBLaunch *)msg;
    if (!l) return;
    if (outstanding>0) --outstanding;
    free_launch(l);
}

/* Fill one WBArg with a lock on the object's parent drawer and its name. */
/* The object itself need not exist: WHDLoad games are icon-only projects
 * (Game.info without a Game file), so the parent drawer is locked by name. */
static int set_arg(struct BeamWBLaunch *l,int i,const char *path)
{
    char parent[256];
    const char *name=(const char *)FilePart((STRPTR)path);
    unsigned int n=(unsigned int)((const char *)PathPart((STRPTR)path)-path);
    if (!*name || n>=sizeof(parent) || strlen(name)>=WB_LAUNCH_NAME) return 0;
    memcpy(parent,path,n); parent[n]=0;
    if (!n) return 0;
    l->args[i].wa_Lock=Lock((STRPTR)parent,ACCESS_READ);
    if (!l->args[i].wa_Lock) return 0;
    strcpy(l->names[i],name);
    l->args[i].wa_Name=(STRPTR)l->names[i];
    return 1;
}

/* Workbench resolves a project's default tool relative to the project's
 * drawer, then along its search path.  Use the standard OS drawers. */
static int resolve_tool(const char *project,const char *tool,char *out,unsigned int cap)
{
    static const char *drawers[]={"SYS:Utilities","SYS:Tools","SYS:System",
                                  "SYS:Prefs","C:","SYS:Rexxc",NULL};
    BPTR lock;
    int i;
    if (!tool || !*tool || strlen(tool)>=cap) return 0;
    if (strchr(tool,':')) {
        strcpy(out,tool);
    } else {
        unsigned int n=(unsigned int)((const char *)PathPart((STRPTR)project)-project);
        if (n>=cap) return 0;
        memcpy(out,project,n); out[n]=0;
        if (!AddPart((STRPTR)out,(STRPTR)tool,(LONG)cap)) return 0;
        lock=Lock((STRPTR)out,ACCESS_READ);
        for (i=0;!lock && drawers[i];i++) {
            if (strlen(drawers[i])+1>=cap) continue;
            strcpy(out,drawers[i]);
            if (!AddPart((STRPTR)out,(STRPTR)tool,(LONG)cap)) continue;
            lock=Lock((STRPTR)out,ACCESS_READ);
        }
        if (!lock) return 0;
        UnLock(lock);
    }
    return 1;
}

int beam_wb_launch(struct MsgPort *reply_port,const char *path)
{
    struct DiskObject *icon,*tool_icon;
    struct BeamWBLaunch *l;
    struct Process *proc;
    char tool[256];
    unsigned long stack=WB_LAUNCH_MIN_STACK;
    BPTR home;
    int ok=0;
    if (!reply_port || !path || !*path) return 0;
    icon=GetDiskObject((STRPTR)path);
    if (!icon) return 0;
    l=(struct BeamWBLaunch *)AllocVec(sizeof(*l),MEMF_PUBLIC|MEMF_CLEAR);
    if (!l) { FreeDiskObject(icon); return 0; }
    if (icon->do_Type==WBTOOL) {
        if (strlen(path)<sizeof(tool)) { strcpy(tool,path); ok=set_arg(l,0,path); }
        l->startup.sm_NumArgs=1;
        if (icon->do_StackSize>0) stack=(unsigned long)icon->do_StackSize;
    } else if (icon->do_Type==WBPROJECT &&
               resolve_tool(path,(const char *)icon->do_DefaultTool,tool,sizeof(tool))) {
        ok=set_arg(l,0,tool) && set_arg(l,1,path);
        l->startup.sm_NumArgs=2;
        if (icon->do_StackSize>0) stack=(unsigned long)icon->do_StackSize;
        tool_icon=GetDiskObject((STRPTR)tool);
        if (tool_icon) {
            if ((unsigned long)tool_icon->do_StackSize>stack)
                stack=(unsigned long)tool_icon->do_StackSize;
            FreeDiskObject(tool_icon);
        }
    }
    FreeDiskObject(icon);
    if (!ok) { free_launch(l); return 0; }
    if (stack<WB_LAUNCH_MIN_STACK) stack=WB_LAUNCH_MIN_STACK;
    l->startup.sm_Segment=LoadSeg((STRPTR)tool);
    if (!l->startup.sm_Segment) { free_launch(l); return 0; }
    strcpy(l->process_name,l->names[0]);
    /* PROGDIR: of the new program is its own drawer; DOS owns this lock. */
    home=DupLock(l->args[0].wa_Lock);
    proc=CreateNewProcTags(NP_Seglist,(ULONG)l->startup.sm_Segment,
                           NP_FreeSeglist,FALSE,
                           NP_StackSize,stack,
                           NP_Name,(ULONG)l->process_name,
                           NP_HomeDir,(ULONG)home,
                           NP_Cli,FALSE,
                           TAG_DONE);
    if (!proc) {
        if (home) UnLock(home);
        free_launch(l);
        return 0;
    }
    l->startup.sm_Message.mn_Node.ln_Type=NT_MESSAGE;
    l->startup.sm_Message.mn_ReplyPort=reply_port;
    l->startup.sm_Message.mn_Length=(UWORD)sizeof(struct WBStartup);
    l->startup.sm_Process=&proc->pr_MsgPort;
    l->startup.sm_ToolWindow=NULL;
    l->startup.sm_ArgList=l->args;
    ++outstanding;
    /* The new process's startup code waits on pr_MsgPort for this message
       and replies it to reply_port when the program exits. */
    PutMsg(&proc->pr_MsgPort,(struct Message *)&l->startup);
    return 1;
}
