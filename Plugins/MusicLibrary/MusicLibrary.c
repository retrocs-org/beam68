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
#include "sources.h"
#include "plugin_walk.h"
#include <dos/dostags.h>

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
    static char root[BEAM_MAX_PATH],player[120],line[400],command[512];
    BPTR settings;
    struct BeamWalk walker;char path[BEAM_MAX_PATH];
    int i,kind,index_root;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    needle=beam_plugin_trim(input);
    beam_config_defaults(&cfg);beam_config_read_active(&cfg);
    beam_plugin_copy(player,cfg.module_command,sizeof(player));
    settings=Open((STRPTR)"ENVARC:Beam68/Plugins/MusicLibrary.cfg",MODE_OLDFILE);
    if (settings) {
        while (FGets(settings,line,sizeof(line))) {
            char *key=beam_plugin_trim(line),*value=strchr(key,'=');
            if (!value || *key=='#' || *key==';') continue;
            *value++=0;value=beam_plugin_trim(value);key=beam_plugin_trim(key);
            if (beam_ascii_equal(key,"DRAWER")) beam_plugin_copy(root,value,sizeof(root));
            if (beam_ascii_equal(key,"PLAYER_COMMAND")) beam_plugin_copy(player,value,sizeof(player));
        }
        Close(settings);
    }
    if (beam_ascii_prefix(needle,"play ")) {
        BPTR file;
        needle=beam_plugin_trim(needle+5);
        file=Lock((STRPTR)needle,ACCESS_READ);
        if (!file) return beam_plugin_error(argc,argv,"Music file is unavailable.");
        UnLock(file);
        if (!beam_module_name(needle) || !strstr(player,"{file}") ||
            !beam_template_fill(player,"{file}",needle,command,sizeof(command)))
            return beam_plugin_error(argc,argv,"Invalid player command or music path.");
        if (SystemTags((STRPTR)command,TAG_DONE)!=0)
            return beam_plugin_error(argc,argv,"Could not start the music player. Check plugin Settings.");
        return beam_plugin_text(argc,argv,"Music sent to the player.");
    }
    if (!*needle) return beam_plugin_error(argc,argv,"Use mod:<name>, then select a file and press Enter to play.");
    matches=unreadable=0;first[0]=0;
    beam_walk_begin(&walker,&cfg,root);
    while(found_count<BEAM68_MAX_ROWS && beam_walk_next(&walker,path,sizeof(path),&kind,&index_root)) {
        if(kind<0 && beam_module_name(FilePart((STRPTR)path)) &&
            beam_ascii_contains(FilePart((STRPTR)path),needle)) {
            matches++;beam_plugin_copy(found[found_count++],path,BEAM_MAX_PATH);
        }
    }
    unreadable=walker.unreadable;beam_walk_end(&walker);
    if (!matches) {
        if (unreadable) sprintf(out,"No music found; %lu drawers could not be read.",unreadable);
        else strcpy(out,"No matching music modules found.");
        return beam_plugin_error(argc,argv,out);
    }
    beam68_rows_begin();
    for (i=0;i<found_count;i++) {
        if (strlen(found[i])+9>BEAM_PLUGIN_MAX_INPUT) {
            beam68_row(BEAM68_ROW_OPEN,FilePart((STRPTR)found[i]),found[i]);
        } else {
            strcpy(command,"mod:play ");
            beam_plugin_copy(command+9,found[i],sizeof(command)-9);
            beam68_row(BEAM68_ROW_QUERY,FilePart((STRPTR)found[i]),command);
        }
    }
    return beam68_rows_send(argc,argv,0);
}
