#include <exec/types.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "control.h"

/* Sender allocates, receiver frees. No waiting, so no risk of an abandoned
 * reply port when Preferences is quit while the launcher is busy. The named
 * port is looked up AND queued while multitasking is forbidden to avoid a
 * FindPort / RemPort race. Never allocate or call DOS while in Forbid(). */
int beam_control_running(void)
{
    int running;
    Forbid();
    running=FindPort((STRPTR)BEAM_PORT_NAME)!=NULL;
    Permit();
    return running;
}
static int queue_message(struct BeamControlMessage *m)
{
    struct MsgPort *p;
    Forbid();
    p=FindPort((STRPTR)BEAM_PORT_NAME);
    if (p) PutMsg(p,(struct Message *)m);
    Permit();
    if (!p) FreeVec(m);
    return p!=NULL;
}

int beam_control_send(unsigned long command)
{
    struct BeamControlMessage *m;
    if (command<BEAM_CTL_RELOAD || command>BEAM_CTL_RELOAD_PLUGINS) return 0;
    m=(struct BeamControlMessage *)AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    if (!m) return 0;
    m->message.mn_Node.ln_Type=NT_MESSAGE;
    m->message.mn_Length=(UWORD)sizeof(*m);
    m->message.mn_ReplyPort=NULL;
    m->magic=BEAM_CONTROL_MAGIC;
    m->version=BEAM_CONTROL_VERSION;
    m->command=(ULONG)command;
    return queue_message(m);
}

int beam_control_send_theme(unsigned long theme,
                            const unsigned long *custom_colors)
{
    struct BeamThemeControlMessage *m;
    int i;
    if (theme>=BEAM_THEME_COUNT || !custom_colors) return 0;
    m=(struct BeamThemeControlMessage *)AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    if (!m) return 0;
    m->control.message.mn_Node.ln_Type=NT_MESSAGE;
    m->control.message.mn_Length=(UWORD)sizeof(*m);
    m->control.message.mn_ReplyPort=NULL;
    m->control.magic=BEAM_CONTROL_MAGIC;
    m->control.version=BEAM_CONTROL_VERSION;
    m->control.command=BEAM_CTL_PREVIEW_THEME;
    m->theme=(ULONG)theme;
    for (i=0;i<BEAM_THEME_COLOR_COUNT;i++)
        m->custom_colors[i]=(ULONG)custom_colors[i];
    return queue_message(&m->control);
}
