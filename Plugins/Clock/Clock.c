/* AmigaDOS clock example with an optional, fully editable plugin config. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

#define CLOCK_CFG "ENVARC:Beam68/Plugins/Clock.cfg"
static int seconds_enabled(void)
{
    BPTR f;
    char line[160];
    int answer=0;
    f=Open((STRPTR)CLOCK_CFG,MODE_OLDFILE);
    if (!f) return 0;
    while (FGets(f,line,sizeof(line))) {
        if (!strncmp(line,"SHOW_SECONDS=YES",16) ||
            !strncmp(line,"SHOW_SECONDS=TRUE",17) ||
            !strncmp(line,"SHOW_SECONDS=1",14)) answer=1;
        else if (!strncmp(line,"SHOW_SECONDS=NO",15) ||
                 !strncmp(line,"SHOW_SECONDS=FALSE",17) ||
                 !strncmp(line,"SHOW_SECONDS=0",14)) answer=0;
    }
    Close(f);
    return answer;
}
int main(int argc, char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],reply[96];
    struct DateStamp ds;
    int seconds;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    seconds=seconds_enabled();
    DateStamp(&ds);
    if (seconds)
        sprintf(reply,plugin_str(PLMSG_CLOCK_SECONDS),
                ds.ds_Minute/60,ds.ds_Minute%60,ds.ds_Tick/50);
    else
        sprintf(reply,plugin_str(PLMSG_CLOCK),
                ds.ds_Minute/60,ds.ds_Minute%60);
    return beam_plugin_line(argc,argv,reply);
}
