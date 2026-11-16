#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "beam68plugin.h"
#include "plugin_locale.h"
#include "plugin_walk.h"

static unsigned long matches;
static unsigned long unreadable;
static char first[BEAM_MAX_PATH];
/* Protocol 1: the first matches become rows that open on Enter. */
static char found[BEAM68_MAX_ROWS][BEAM_MAX_PATH];
static int found_count;


int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *needle;
    static struct BeamConfig cfg; /* 14 KB: never on a 4 KB Shell stack */
    struct BeamWalk walker;char path[BEAM_MAX_PATH];
    int i,kind,root;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    needle=beam_plugin_trim(input);
    if (!*needle) return beam_plugin_error(argc,argv,plugin_str(PLMSG_FIND_USAGE));
    beam_config_defaults(&cfg);beam_config_read_active(&cfg);
    matches=unreadable=0;first[0]=0;
    beam_walk_begin(&walker,&cfg,NULL);
    while(found_count<BEAM68_MAX_ROWS && beam_walk_next(&walker,path,sizeof(path),&kind,&root)) {
        if(beam_ascii_contains(FilePart((STRPTR)path),needle)) {
            matches++;beam_plugin_copy(found[found_count++],path,BEAM_MAX_PATH);
        }
    }
    unreadable=walker.unreadable;beam_walk_end(&walker);
    if (!matches) {
        if (unreadable) sprintf(out,plugin_str(PLMSG_FIND_UNREADABLE),unreadable);
        else strcpy(out,plugin_str(PLMSG_FIND_NONE));
        return beam_plugin_error(argc,argv,out);
    }
    beam68_rows_begin();
    for (i=0;i<found_count;i++) beam68_row(BEAM68_ROW_OPEN,found[i],found[i]);
    return beam68_rows_send(argc,argv,0);
}
