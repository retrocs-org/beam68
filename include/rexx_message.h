#ifndef BEAM68_REXX_MESSAGE_H
#define BEAM68_REXX_MESSAGE_H
/* Direct host clients must identify packets as REXX; CreateRexxMsg alone
 * does not do this on rexxsyslib 47.2. Keep the marker in public allocated
 * memory so an unanswered packet never points into unloaded program data. */
#include <rexx/storage.h>
#include <proto/rexxsyslib.h>
#include <string.h>
static void beam_rexx_message_delete(struct RexxMsg *message)
{
    if (message->rm_Node.mn_Node.ln_Name)
        DeleteArgstring((UBYTE *)message->rm_Node.mn_Node.ln_Name);
    if (message->rm_Args[0]) DeleteArgstring((UBYTE *)message->rm_Args[0]);
    DeleteRexxMsg(message);
}
static struct RexxMsg *beam_rexx_message_create(struct MsgPort *reply,
    const char *command,int result)
{
    struct RexxMsg *message=CreateRexxMsg(reply,NULL,NULL);
    if (!message) return NULL;
    message->rm_Node.mn_Node.ln_Name=(STRPTR)CreateArgstring((STRPTR)"REXX",4);
    message->rm_Args[0]=(STRPTR)CreateArgstring((STRPTR)command,(ULONG)strlen(command));
    if (!message->rm_Node.mn_Node.ln_Name || !message->rm_Args[0]) {
        beam_rexx_message_delete(message); return NULL;
    }
    message->rm_Action=RXCOMM|(result?RXFF_RESULT:0);
    return message;
}
#endif
