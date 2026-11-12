#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rexx_message.h"
#include "music_control.h"
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

struct RxsLib *RexxSysBase=NULL;

static int port_exists(const char *name)
{
    struct MsgPort *port;
    Forbid();port=FindPort((STRPTR)name);Permit();
    return port!=0;
}

static int preferred_player(void)
{
    static const char *paths[]={"ENVARC:Beam68/Plugins/MusicControls.cfg",
        "ENV:Beam68/Plugins/MusicControls.cfg","PROGDIR:MusicControls.cfg",
        "PROGDIR:Plugins/MusicControls.cfg",NULL};
    char line[160],copy[160],*s,*eq;BPTR file=0;int i,player;
    for (i=0;paths[i] && !file;i++) file=Open((STRPTR)paths[i],MODE_OLDFILE);
    if (!file) return BEAM_MUSIC_AUTO;
    while (FGets(file,line,sizeof(line))) {
        beam_plugin_copy(copy,line,sizeof(copy));s=beam_plugin_trim(copy);
        if (!*s || *s=='#' || *s==';') continue;
        eq=strchr(s,'=');if (!eq) continue;*eq++=0;
        if (!beam_ascii_equal(beam_plugin_trim(s),"PREFERRED_PLAYER")) continue;
        s=beam_plugin_trim(eq);
        if (beam_ascii_equal(s,"EaglePlayer 2")) player=BEAM_MUSIC_EAGLE;
        else player=beam_music_player_from_name(s);
        Close(file);return player>=0?player:BEAM_MUSIC_AUTO;
    }
    Close(file);return BEAM_MUSIC_AUTO;
}

static int choose_player(int requested)
{
    int preferred;
    if (requested>BEAM_MUSIC_AUTO)
        return port_exists(beam_music_player_port(requested))?requested:BEAM_MUSIC_AUTO;
    preferred=preferred_player();
    if (preferred>BEAM_MUSIC_AUTO && port_exists(beam_music_player_port(preferred))) return preferred;
    if (port_exists(beam_music_player_port(BEAM_MUSIC_HIPPO))) return BEAM_MUSIC_HIPPO;
    if (port_exists(beam_music_player_port(BEAM_MUSIC_AMIGAAMP))) return BEAM_MUSIC_AMIGAAMP;
    if (port_exists(beam_music_player_port(BEAM_MUSIC_EAGLE))) return BEAM_MUSIC_EAGLE;
    return BEAM_MUSIC_AUTO;
}

static int player_list(int argc,char **argv)
{
    char result[121];int found=0,preferred=preferred_player();
    strcpy(result,plugin_str(PLMSG_MUSIC_RUNNING));
    if (port_exists(beam_music_player_port(BEAM_MUSIC_HIPPO))) {
        strcat(result,"HippoPlayer");found=1;
    }
    if (port_exists(beam_music_player_port(BEAM_MUSIC_AMIGAAMP))) {
        if (found) strcat(result,", ");
        strcat(result,"AmigaAMP");found=1;
    }
    if (port_exists(beam_music_player_port(BEAM_MUSIC_EAGLE))) {
        if (found) strcat(result,", ");
        strcat(result,"EaglePlayer 2");found=1;
    }
    if (!found) strcat(result,"none");
    if (strlen(result)+strlen(beam_music_player_name(preferred))+16<sizeof(result)) {
        strcat(result,plugin_str(PLMSG_MUSIC_PREFERRED));strcat(result,beam_music_player_name(preferred));
    }
    return beam_plugin_text(argc,argv,result);
}

static LONG send_command(const char *port,const char *command,
                         char *output,unsigned int capacity)
{
    struct MsgPort *reply,*target;
    struct RexxMsg *request,*response;
    LONG result=20;int delivered=0;
    if (output && capacity) output[0]=0;
    if (!RexxSysBase || !port || !*port || !command || !*command) return 20;
    reply=CreateMsgPort();if (!reply) return 20;
    request=beam_rexx_message_create(reply,command,output && capacity);
    if (!request) { DeleteMsgPort(reply);return 20; }
    Forbid();target=FindPort((STRPTR)port);
    if (target) { PutMsg(target,(struct Message *)request);delivered=1; }
    Permit();
    if (delivered) {
        int waited;
        response=NULL;
        /* A hung player must not hang the plugin forever (~10 s limit). */
        for (waited=0;waited<200;waited++) {
            response=(struct RexxMsg *)GetMsg(reply);
            if (response) break;
            Delay(2);
        }
        if (!response) {
            /* The player still owns the message and will reply later; it is
               unsafe to free it.  Make the port silent and leave both
               allocated (a small, bounded leak) rather than crash. */
            Forbid();
            reply->mp_Flags=PA_IGNORE;
            Permit();
            if (output && capacity) beam_plugin_copy(output,plugin_str(PLMSG_MUSIC_NO_ANSWER),capacity);
            return 20;
        }
        if (response==request) {
            result=request->rm_Result1;
            if (!result && request->rm_Result2) {
                if (!result && output && capacity)
                    beam_plugin_copy(output,(STRPTR)request->rm_Result2,capacity);
                DeleteArgstring((STRPTR)request->rm_Result2);
                request->rm_Result2=0;
            }
        }
    }
    beam_rexx_message_delete(request);DeleteMsgPort(reply);return result;
}

static int execute_request(int player,const struct BeamMusicRequest *request,
                           char *result,unsigned int capacity)
{
    const char *port=beam_music_player_port(player);char value[121],command[64];
    LONG rc=20;int volume;
    if (!result || !capacity) return 0;
    result[0]=0;value[0]=0;
    if (request->action==BEAM_MUSIC_STATUS) {
        if (player==BEAM_MUSIC_HIPPO) rc=send_command(port,"GET PLAY",value,sizeof(value));
        else if (player==BEAM_MUSIC_AMIGAAMP) rc=send_command(port,"STATUS",value,sizeof(value));
        else rc=send_command(port,"STATUS G PLY",value,sizeof(value));
        if (rc) return 0;
        {
            char text[160];
            sprintf(text,"%s: %.120s",beam_music_player_name(player),*value?value:"ready");
            beam_plugin_copy(result,text,capacity);
        }
        return 1;
    }
    if (request->action==BEAM_MUSIC_PLAY) {
        strcpy(command,player==BEAM_MUSIC_EAGLE?"PLAY 1":"PLAY");
    } else if (request->action==BEAM_MUSIC_PAUSE) {
        if (player==BEAM_MUSIC_HIPPO) {
            rc=send_command(port,"GET PLAY",value,sizeof(value));if (rc) return 0;
            strcpy(command,atoi(value)==1?"STOP":"CONT");
        } else strcpy(command,player==BEAM_MUSIC_AMIGAAMP?"PAUSE":"PLAY");
    } else if (request->action==BEAM_MUSIC_STOP) strcpy(command,"STOP");
    else if (request->action==BEAM_MUSIC_NEXT) {
        if (player==BEAM_MUSIC_HIPPO) strcpy(command,"CHOOSENEXT");
        else strcpy(command,player==BEAM_MUSIC_AMIGAAMP?"NEXT":"NEXTMODULE");
    } else if (request->action==BEAM_MUSIC_PREVIOUS) {
        if (player==BEAM_MUSIC_HIPPO) strcpy(command,"CHOOSEPREV");
        else strcpy(command,player==BEAM_MUSIC_AMIGAAMP?"PREVIOUS":"PREVMODULE");
    } else if (request->action==BEAM_MUSIC_VOLUME_UP ||
               request->action==BEAM_MUSIC_VOLUME_DOWN) {
        if (player==BEAM_MUSIC_AMIGAAMP)
            strcpy(command,request->action==BEAM_MUSIC_VOLUME_UP?"INCVOLUME":"DECVOLUME");
        else {
            rc=send_command(port,player==BEAM_MUSIC_HIPPO?"GET VOLU":"STATUS G VOL",
                            value,sizeof(value));if (rc) return 0;
            volume=atoi(value)+(request->action==BEAM_MUSIC_VOLUME_UP?
                    (player==BEAM_MUSIC_HIPPO?4:8):-(player==BEAM_MUSIC_HIPPO?4:8));
            if (volume<0) volume=0;
            if (volume>(player==BEAM_MUSIC_HIPPO?64:255))
                volume=player==BEAM_MUSIC_HIPPO?64:255;
            sprintf(command,"VOLUME %d",volume);
        }
    } else if (request->action==BEAM_MUSIC_VOLUME_SET) {
        if (player==BEAM_MUSIC_HIPPO) volume=(request->volume*64+50)/100;
        else if (player==BEAM_MUSIC_AMIGAAMP) volume=(request->volume*26+50)/100;
        else volume=(request->volume*255+50)/100;
        sprintf(command,"VOLUME %d",volume);
    } else return 0;
    rc=send_command(port,command,NULL,0);if (rc) return 0;
    if (strlen(plugin_str(PLMSG_MUSIC_ACCEPTED))+strlen(beam_music_player_name(player))+1>capacity) return 0;
    sprintf(result,plugin_str(PLMSG_MUSIC_ACCEPTED),beam_music_player_name(player));return 1;
}

int main(int argc,char **argv)
{
    struct BeamMusicRequest request;
    char input[BEAM_PLUGIN_MAX_INPUT+1],result[121],message[121];
    int player,rc;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    if (!beam_music_parse(beam_plugin_trim(input),&request))
        return beam_plugin_error(argc,argv,plugin_str(PLMSG_MUSIC_USAGE));
    if (request.action==BEAM_MUSIC_PLAYERS) return player_list(argc,argv);
    player=choose_player(request.player);
    if (!player) {
        if (request.player)
            sprintf(message,plugin_str(PLMSG_MUSIC_NOT_RUNNING),beam_music_player_name(request.player));
        else strcpy(message,plugin_str(PLMSG_MUSIC_NONE));
        return beam_plugin_text(argc,argv,message);
    }
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    if (!RexxSysBase) return beam_plugin_error(argc,argv,"rexxsyslib.library is not available");
    rc=execute_request(player,&request,result,sizeof(result));
    CloseLibrary((struct Library *)RexxSysBase);RexxSysBase=NULL;
    if (!rc) return beam_plugin_error(argc,argv,plugin_str(PLMSG_MUSIC_REJECTED));
    return beam_plugin_text(argc,argv,result);
}
