#include <exec/types.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
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

/* A late response is discarded by the server once this private reply port
 * has been removed. The sender and receiver each own their own allocation. */
void beam_control_reply_version(const struct BeamControlMessage *base)
{
    const struct BeamVersionRequest *request=(const struct BeamVersionRequest *)base;
    struct BeamVersionReply *reply;
    struct MsgPort *port;
    if(base->message.mn_Length<sizeof(*request) ||
       !memchr(request->reply_port,0,sizeof(request->reply_port)) ||
       strncmp(request->reply_port,BEAM_VERSION_REPLY_PREFIX,strlen(BEAM_VERSION_REPLY_PREFIX))) return;
    reply=(struct BeamVersionReply *)AllocVec(sizeof(*reply),MEMF_PUBLIC|MEMF_CLEAR);
    if(!reply) return;
    reply->message.mn_Node.ln_Type=NT_MESSAGE;
    reply->message.mn_Length=sizeof(*reply);
    reply->magic=BEAM_CONTROL_MAGIC;reply->version=BEAM_CONTROL_VERSION;
    reply->serial=request->serial;
    strcpy(reply->launcher_version,BEAM68_VERSION);
    Forbid();
    port=FindPort((STRPTR)request->reply_port);
    if(port) PutMsg(port,(struct Message *)reply);
    Permit();
    if(!port) FreeVec(reply);
}
static void release_version_reply(struct Message *message)
{
    struct BeamVersionReply *reply=(struct BeamVersionReply *)message;
    if(message->mn_Length>=sizeof(*reply) && reply->magic==BEAM_CONTROL_MAGIC)
        FreeVec(message);
    else if(message->mn_ReplyPort) ReplyMsg(message);
}
int beam_control_version(char *out,unsigned int cap)
{
    struct BeamVersionRequest *request;
    struct BeamVersionReply *reply;
    struct Message *message;
    struct MsgPort *port;
    struct DateStamp stamp;
    char name[64];ULONG serial;int tick,result=-1;
    static ULONG sequence;
    if(!out || !cap) return -1;
    out[0]=0;
    if(!beam_control_running()) return 0;
    port=CreateMsgPort();if(!port) return -1;
    DateStamp(&stamp);
    serial=(ULONG)stamp.ds_Days*4320000UL+(ULONG)stamp.ds_Minute*3000UL+
        (ULONG)stamp.ds_Tick+(++sequence);
    sprintf(name,BEAM_VERSION_REPLY_PREFIX "%08lx.%08lx",(unsigned long)FindTask(NULL),(unsigned long)serial);
    port->mp_Node.ln_Name=name;
    Forbid();
    if(FindPort(name)) { Permit();DeleteMsgPort(port);return -1; }
    AddPort(port);Permit();
    request=(struct BeamVersionRequest *)AllocVec(sizeof(*request),MEMF_PUBLIC|MEMF_CLEAR);
    if(request) {
        request->control.message.mn_Node.ln_Type=NT_MESSAGE;
        request->control.message.mn_Length=sizeof(*request);
        request->control.magic=BEAM_CONTROL_MAGIC;
        request->control.version=BEAM_CONTROL_VERSION;
        request->control.command=BEAM_CTL_QUERY_VERSION;
        request->serial=serial;strcpy(request->reply_port,name);
        if(queue_message(&request->control)) {
            for(tick=0;tick<=25 && result<0;tick++) {
                while((message=GetMsg(port))) {
                    reply=(struct BeamVersionReply *)message;
                    if(message->mn_Length>=sizeof(*reply) && reply->magic==BEAM_CONTROL_MAGIC &&
                       reply->version==BEAM_CONTROL_VERSION && reply->serial==serial &&
                       memchr(reply->launcher_version,0,sizeof(reply->launcher_version)) &&
                       reply->launcher_version[0] && strlen(reply->launcher_version)<cap) {
                        strcpy(out,reply->launcher_version);result=1;
                    }
                    release_version_reply(message);
                }
                if(result<0 && tick<25) Delay(1);
            }
        } else result=0;
    }
    Forbid();RemPort(port);Permit();
    while((message=GetMsg(port))) release_version_reply(message);
    DeleteMsgPort(port);return result;
}
