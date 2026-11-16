#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "plugin_common.h"
#include "plugin_protocol.h"
#include "beam68plugin.h"
static struct BeamConfig cfg;
static char input[BEAM_PLUGIN_MAX_INPUT+1];
static void setting(const char *file,const char *wanted,char *out,unsigned int cap)
{
    BPTR f;char line[256],*eq,*key;
    f=Open((STRPTR)file,MODE_OLDFILE);if(!f)return;
    while(FGets(f,line,sizeof(line))) {
        key=beam_plugin_trim(line);eq=strchr(key,'=');
        if(!eq || *key=='#' || *key==';')continue;
        *eq++=0;
        if(beam_ascii_equal(beam_plugin_trim(key),wanted))
            beam_plugin_copy(out,beam_plugin_trim(eq),cap);
    }
    Close(f);
}
static int safe(const char *p) { return p && *p && !strpbrk(p,"\"*`\r\n"); }
#include "whdload.h"
#include "plugin_walk.h"
#include "icon_info.h"
static int play(const char *path)
{
    char command[640],drawer[BEAM_MAX_PATH],tool[64];
    BPTR lock;
    if(!safe(path) || strlen(path)>=sizeof(drawer))return 20;
    lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 20;UnLock(lock);
    beam_plugin_copy(drawer,path,sizeof(drawer));
    if(!PathPart((STRPTR)drawer))return 20;
    *PathPart((STRPTR)drawer)=0;
    beam_plugin_copy(tool,cfg.whdload_command,sizeof(tool));
    setting("ENVARC:Beam68/Plugins/WHDLoad.cfg","COMMAND",tool,sizeof(tool));
    if(!safe(tool))return 20;
    sprintf(command,"\"%s\" SLAVE=\"%s\" DATA=\"%s\" PRELOAD",tool,path,drawer);
    return SystemTags((STRPTR)command,SYS_Asynch,TRUE,SYS_Input,0,SYS_Output,0,TAG_DONE)==0?0:20;
}
static char found[BEAM68_MAX_ROWS][BEAM_MAX_PATH];
static int count,unreadable;
static int game_path(char *path)
{
    static unsigned char bytes[4096];char tool[64],info[BEAM_MAX_PATH];
    unsigned int n=strlen(path);BPTR f;LONG got;
    if(n>6 && beam_ascii_equal(path+n-6,".slave")) {
        beam_plugin_copy(info,path,sizeof(info));strcpy(info+n-6,".info");
        f=Lock((STRPTR)info,ACCESS_READ);if(f){UnLock(f);return 0;}
        return 1;
    }
    if(n<=5 || !beam_ascii_equal(path+n-5,".info"))return 0;
    f=Open((STRPTR)path,MODE_OLDFILE);if(!f)return 0;
    got=Read(f,bytes,sizeof(bytes));Close(f);
    if(got>0 && beam_icon_info(bytes,(unsigned long)got,tool,sizeof(tool))==4 &&
        beam_icon_is_whdload_tool(tool)){path[n-5]=0;return 1;}
    return 0;
}
static int index_library(const char *output)
{
    struct BeamWalk walker;struct MsgPort *port,*existing;
    char temp[BEAM_MAX_PATH],path[BEAM_MAX_PATH],line[BEAM_MAX_PATH+32];
    BPTR file;int root,kind,number=0,ok=1;
    if(!beam_ascii_prefix(output,"T:Beam68-WHD-") || !safe(output) || strlen(output)+5>=sizeof(temp))return 20;
    SetTaskPri(FindTask(NULL),-5);
    port=CreateMsgPort();if(!port)return 20;
    port->mp_Node.ln_Name=(STRPTR)"BEAM68_WHD_INDEX";
    Forbid();existing=FindPort(port->mp_Node.ln_Name);if(!existing)AddPort(port);Permit();
    if(existing){DeleteMsgPort(port);return 20;}
    sprintf(temp,"%s.new",output);file=Open((STRPTR)temp,MODE_NEWFILE);
    if(!file){Forbid();RemPort(port);Permit();DeleteMsgPort(port);return 20;}
    if(FPuts(file,(STRPTR)"BEAM68WHD1\n")<0)ok=0;
    beam_walk_begin(&walker,&cfg,NULL);
    while(ok && number<cfg.max_items && beam_walk_next(&walker,path,sizeof(path),&kind,&root)) {
        if(kind>=0 || !safe(path) || strchr(path,'\t') || !game_path(path))continue;
        sprintf(line,"%d\t%s\n",root,path);
        if(FPuts(file,(STRPTR)line)<0)ok=0;else number++;
    }
    /* Incomplete scans must never remove previously indexed games. */
    sprintf(line,"END %d %d\n",number,walker.unreadable || number>=cfg.max_items);
    if(FPuts(file,(STRPTR)line)<0)ok=0;
    beam_walk_end(&walker);if(!Close(file))ok=0;
    if(ok){DeleteFile((STRPTR)output);ok=Rename((STRPTR)temp,(STRPTR)output)!=0;}
    if(!ok)DeleteFile((STRPTR)temp);
    Forbid();RemPort(port);Permit();DeleteMsgPort(port);return ok?0:20;
}
static void search_games(const char *needle)
{
    struct BeamWalk walker;char path[BEAM_MAX_PATH];int kind,root;
    beam_walk_begin(&walker,&cfg,NULL);
    while(count<BEAM68_MAX_ROWS && beam_walk_next(&walker,path,sizeof(path),&kind,&root))
        if(kind<0 && game_path(path) && beam_ascii_contains(path,needle))
            beam_plugin_copy(found[count++],path,BEAM_MAX_PATH);
    unreadable=walker.unreadable;beam_walk_end(&walker);
}

int main(int argc,char **argv)
{
    char command[BEAM_PLUGIN_MAX_INPUT+1],*q;int i,rc;
    beam_config_defaults(&cfg);beam_config_read_active(&cfg);
    if(argc==3 && beam_ascii_equal(argv[1],"PLAY"))return play(argv[2]);
    if(argc==3 && beam_ascii_equal(argv[1],"INDEX"))return index_library(argv[2]);
    if(!beam_plugin_input(argc,argv,input,sizeof(input)))return 20;
    q=beam_plugin_trim(input);
    if(beam_ascii_prefix(q,"play ")) {
        rc=play(beam_plugin_trim(q+5));
        return rc?beam_plugin_error(argc,argv,"WHDLoad failed. Check the game and plugin Settings."):
            beam_plugin_text(argc,argv,"WHDLoad finished.");
    }
    if(!*q)return beam_plugin_error(argc,argv,"Use whd:<game name> to find installed games.");
    search_games(q);
    if(!count)return beam_plugin_error(argc,argv,unreadable?"No games found; some drawers were unreadable.":"No matching WHDLoad games found.");
    beam68_rows_begin();
    for(i=0;i<count;i++) {
        if(!(strlen(found[i])>=6 && beam_ascii_equal(found[i]+strlen(found[i])-6,".slave"))) {
            beam68_row(BEAM68_ROW_OPEN,FilePart((STRPTR)found[i]),found[i]);continue;
        }
        if(strlen(found[i])+9>BEAM_PLUGIN_MAX_INPUT)continue;
        strcpy(command,"whd:play ");beam_plugin_copy(command+9,found[i],sizeof(command)-9);
        beam68_row(BEAM68_ROW_QUERY,FilePart((STRPTR)found[i]),command);
    }
    return beam68_rows_send(argc,argv,0);
}
