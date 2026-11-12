/* External plugin: free Chip and Fast RAM (KiB). */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

int main(int argc, char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],reply[100];
    ULONG chip,fast;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    chip=AvailMem(MEMF_CHIP)/1024;
    fast=AvailMem(MEMF_FAST)/1024;
    sprintf(reply,plugin_str(PLMSG_MEMORY),chip,fast);
    return beam_plugin_line(argc,argv,reply);
}
