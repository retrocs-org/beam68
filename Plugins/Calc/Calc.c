/* External plugin: integer math; evaluated only after Enter. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "calc_engine.h"
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

int main(int argc, char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1];
    char reply[BEAM_PLUGIN_MAX_RESULT+32];
    long answer;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    if (!beam_calculate(input,&answer))
        return beam_plugin_line(argc,argv,plugin_str(PLMSG_CALC_ERROR));
    strcpy(reply,"TEXT=");
    beam_calc_format(answer,reply+5,BEAM_PLUGIN_MAX_RESULT);
    strcat(reply,"\n");
    return beam_plugin_line(argc,argv,reply);
}
