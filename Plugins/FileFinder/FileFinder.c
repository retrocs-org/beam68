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

static unsigned long matches;
static unsigned long unreadable;
static char first[BEAM_MAX_PATH];
/* Protocol 1: the first matches become rows that open on Enter. */
static char found[BEAM68_MAX_ROWS][BEAM_MAX_PATH];
static int found_count;

/* One shared path buffer: each recursion level only appends its entry name
 * and truncates back, so a frame costs a few bytes instead of 240. */
static char walk[BEAM_MAX_PATH];

static void scan(const char *needle,int depth,int max_depth)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    unsigned int len=(unsigned int)strlen(walk);
    if (depth>max_depth || matches>9999UL) return;
    lock=Lock((STRPTR)walk,ACCESS_READ);
    if (!lock) { unreadable++;return; }
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (!fib) { UnLock(lock);unreadable++;return; }
    if (!Examine(lock,fib)) { FreeDosObject(DOS_FIB,fib);UnLock(lock);unreadable++;return; }
    while (ExNext(lock,fib)) {
        if (!strcmp(fib->fib_FileName,".") || !strcmp(fib->fib_FileName,"..")) continue;
        walk[len]=0;
        if (!AddPart(walk,fib->fib_FileName,sizeof(walk))) { unreadable++;continue; }
        if (beam_ascii_contains(fib->fib_FileName,needle)) {
            matches++;if (!first[0]) beam_plugin_copy(first,walk,sizeof(first));
            if (found_count<BEAM68_MAX_ROWS) beam_plugin_copy(found[found_count++],walk,BEAM_MAX_PATH);
        }
        /* Follow real drawers only: links can duplicate trees or loop. */
        if (fib->fib_DirEntryType==ST_USERDIR && depth<max_depth)
            scan(needle,depth+1,max_depth);
    }
    walk[len]=0;
    FreeDosObject(DOS_FIB,fib);UnLock(lock);
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *needle;
    static struct BeamConfig cfg; /* 14 KB: never on a 4 KB Shell stack */
    int i;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    needle=beam_plugin_trim(input);
    if (!*needle) return beam_plugin_error(argc,argv,plugin_str(PLMSG_FIND_USAGE));
    beam_config_defaults(&cfg);beam_config_read_active(&cfg);
    matches=unreadable=0;first[0]=0;
    for (i=0;i<cfg.dir_count;i++) {
        beam_plugin_copy(walk,cfg.dirs[i],sizeof(walk));
        scan(needle,0,cfg.max_depth);
    }
    if (!matches) {
        if (unreadable) sprintf(out,plugin_str(PLMSG_FIND_UNREADABLE),unreadable);
        else strcpy(out,plugin_str(PLMSG_FIND_NONE));
        return beam_plugin_error(argc,argv,out);
    }
    beam68_rows_begin();
    for (i=0;i<found_count;i++) beam68_row(BEAM68_ROW_OPEN,found[i],found[i]);
    return beam68_rows_send(argc,argv,0);
}
