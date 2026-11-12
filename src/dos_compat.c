#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "dos_compat.h"


long beam_system(const char *command,const char *in_name,
                 const char *out_name,int asynchronous)
{
    struct TagItem tags[4];
    BPTR in=0,out=0;
    LONG rc;
    if (!command) return -1;
    if (!in_name) in_name="NIL:";
    if (!out_name) out_name="NIL:";
    tags[0].ti_Tag=SYS_Asynch; tags[0].ti_Data=asynchronous?TRUE:FALSE;
    if (DOSBase->dl_lib.lib_Version>=47) {
        tags[1].ti_Tag=SYS_InName; tags[1].ti_Data=(ULONG)in_name;
        tags[2].ti_Tag=SYS_OutName; tags[2].ti_Data=(ULONG)out_name;
        tags[3].ti_Tag=TAG_END; tags[3].ti_Data=0;
        return (long)SystemTagList((STRPTR)command,tags);
    }
    in=Open((STRPTR)in_name,MODE_OLDFILE);
    out=Open((STRPTR)out_name,MODE_NEWFILE);
    if (!in || !out) {
        if (in) Close(in);
        if (out) Close(out);
        return -1;
    }
    tags[1].ti_Tag=SYS_Input; tags[1].ti_Data=(ULONG)in;
    tags[2].ti_Tag=SYS_Output; tags[2].ti_Data=(ULONG)out;
    tags[3].ti_Tag=TAG_END; tags[3].ti_Data=0;
    rc=SystemTagList((STRPTR)command,tags);
    /* An asynchronous child owns and closes its streams; a synchronous call
       or a failed start leaves them with the caller. */
    if (!asynchronous || rc==-1) { Close(in); Close(out); }
    return (long)rc;
}
