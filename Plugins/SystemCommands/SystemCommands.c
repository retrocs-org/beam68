#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "dos_compat.h"
#include "plugin_locale.h"

static int confirmed(const char *s,const char *command)
{
    unsigned int n=(unsigned int)strlen(command);
    if (!beam_ascii_prefix(s,command)) return 0;
    s+=n;while (*s==' '||*s=='\t'||*s==':') s++;
    return beam_ascii_equal(s,"confirm");
}

static LONG quiet_system(const char *command)
{
    return (LONG)beam_system(command,NULL,NULL,0);
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *s;
    ULONG chip,fast;
    struct Library *dos,*intuition,*workbench;
    BPTR lock;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (beam_ascii_equal(s,"avail")) {
        chip=AvailMem(MEMF_CHIP)/1024UL;fast=AvailMem(MEMF_FAST)/1024UL;
        sprintf(out,plugin_str(PLMSG_SYS_MEMORY),chip,fast);
        return beam_plugin_text(argc,argv,out);
    }
    if (beam_ascii_equal(s,"version")) {
        dos=OpenLibrary("dos.library",0);intuition=OpenLibrary("intuition.library",0);
        workbench=OpenLibrary("workbench.library",0);
        sprintf(out,"Exec %u.%u / DOS %u.%u / Intuition %u.%u / Workbench %u.%u",
            (unsigned int)SysBase->LibNode.lib_Version,(unsigned int)SysBase->LibNode.lib_Revision,
            dos?(unsigned int)dos->lib_Version:0,dos?(unsigned int)dos->lib_Revision:0,
            intuition?(unsigned int)intuition->lib_Version:0,intuition?(unsigned int)intuition->lib_Revision:0,
            workbench?(unsigned int)workbench->lib_Version:0,workbench?(unsigned int)workbench->lib_Revision:0);
        if (workbench) CloseLibrary(workbench);
        if (intuition) CloseLibrary(intuition);
        if (dos) CloseLibrary(dos);
        return beam_plugin_text(argc,argv,out);
    }
    if (beam_ascii_equal(s,"flush")) {
        if (quiet_system("C:Avail FLUSH")!=0) return beam_plugin_error(argc,argv,plugin_str(PLMSG_SYS_FLUSH_FAILED));
        chip=AvailMem(MEMF_CHIP)/1024UL;fast=AvailMem(MEMF_FAST)/1024UL;
        sprintf(out,plugin_str(PLMSG_SYS_FLUSHED),chip,fast);
        return beam_plugin_text(argc,argv,out);
    }
    if (beam_ascii_prefix(s,"reboot")) {
        if (!confirmed(s,"reboot")) return beam_plugin_error(argc,argv,plugin_str(PLMSG_SYS_REBOOT_CONFIRM));
        if (beam_plugin_text(argc,argv,plugin_str(PLMSG_SYS_REBOOTING))!=0) return 20;
        /* Let FFS flush delayed bitmap/header writes before the reset. */
        Delay(100);ColdReboot();return 0;
    }
    if (beam_ascii_prefix(s,"shutdown")) {
        if (!confirmed(s,"shutdown")) return beam_plugin_error(argc,argv,plugin_str(PLMSG_SYS_SHUTDOWN_CONFIRM));
        lock=Lock("C:ShutDown",ACCESS_READ);
        if (!lock) return beam_plugin_error(argc,argv,"C:ShutDown is not installed on this system");
        UnLock(lock);
        if (beam_plugin_text(argc,argv,plugin_str(PLMSG_SYS_SHUTDOWN))!=0) return 20;
        Delay(15);quiet_system("C:ShutDown");return 0;
    }
    return beam_plugin_error(argc,argv,plugin_str(PLMSG_SYS_USAGE));
}
