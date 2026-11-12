#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "music_control.h"

static int equal(const char *a,const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}

static char *trim(char *s)
{
    char *end;
    while (*s==' ' || *s=='\t') ++s;
    end=s+strlen(s);
    while (end>s && (end[-1]==' ' || end[-1]=='\t')) --end;
    *end=0;
    return s;
}

static char *next_word(char **cursor)
{
    char *word=*cursor;
    while (*word==' ' || *word=='\t') ++word;
    *cursor=word;
    while (**cursor && **cursor!=' ' && **cursor!='\t') ++*cursor;
    if (**cursor) *(*cursor)++=0;
    return word;
}

int beam_music_player_from_name(const char *name)
{
    if (!name) return -1;
    if (equal(name,"auto")) return BEAM_MUSIC_AUTO;
    if (equal(name,"hip") || equal(name,"hippo") ||
        equal(name,"hippoplayer")) return BEAM_MUSIC_HIPPO;
    if (equal(name,"amp") || equal(name,"amigaamp"))
        return BEAM_MUSIC_AMIGAAMP;
    if (equal(name,"ep") || equal(name,"eagle") ||
        equal(name,"eagleplayer") || equal(name,"eagleplayer2"))
        return BEAM_MUSIC_EAGLE;
    return -1;
}

const char *beam_music_player_name(int player)
{
    if (player==BEAM_MUSIC_HIPPO) return "HippoPlayer";
    if (player==BEAM_MUSIC_AMIGAAMP) return "AmigaAMP";
    if (player==BEAM_MUSIC_EAGLE) return "EaglePlayer 2";
    return "Auto";
}

const char *beam_music_player_port(int player)
{
    if (player==BEAM_MUSIC_HIPPO) return "HIPPOPLAYER";
    if (player==BEAM_MUSIC_AMIGAAMP) return "AMIGAAMP";
    if (player==BEAM_MUSIC_EAGLE) return "rexx_EP";
    return "";
}

int beam_music_parse(const char *text,struct BeamMusicRequest *request)
{
    char buffer[121],*cursor,*word,*arg,*end;
    long value;int player;
    if (!text || !request || strlen(text)>=sizeof(buffer)) return 0;
    strcpy(buffer,text);cursor=trim(buffer);
    request->player=BEAM_MUSIC_AUTO;
    request->action=BEAM_MUSIC_STATUS;
    request->volume=0;
    if (!*cursor) return 1;
    word=next_word(&cursor);
    player=beam_music_player_from_name(word);
    if (player>=BEAM_MUSIC_AUTO) {
        request->player=player;
        word=next_word(&cursor);
        if (!*word) return 1;
    }
    if (equal(word,"status") || equal(word,"now")) request->action=BEAM_MUSIC_STATUS;
    else if (equal(word,"players") || equal(word,"player")) request->action=BEAM_MUSIC_PLAYERS;
    else if (equal(word,"play") || equal(word,"resume")) request->action=BEAM_MUSIC_PLAY;
    else if (equal(word,"pause") || equal(word,"toggle") || equal(word,"playpause")) request->action=BEAM_MUSIC_PAUSE;
    else if (equal(word,"stop")) request->action=BEAM_MUSIC_STOP;
    else if (equal(word,"next")) request->action=BEAM_MUSIC_NEXT;
    else if (equal(word,"previous") || equal(word,"prev")) request->action=BEAM_MUSIC_PREVIOUS;
    else if (equal(word,"volup")) request->action=BEAM_MUSIC_VOLUME_UP;
    else if (equal(word,"voldown")) request->action=BEAM_MUSIC_VOLUME_DOWN;
    else if (equal(word,"volume") || equal(word,"vol")) {
        arg=next_word(&cursor);
        if (equal(arg,"up") || equal(arg,"+")) request->action=BEAM_MUSIC_VOLUME_UP;
        else if (equal(arg,"down") || equal(arg,"-")) request->action=BEAM_MUSIC_VOLUME_DOWN;
        else {
            if (!*arg) return 0;
            value=strtol(arg,&end,10);
            if (*end || value<0 || value>100) return 0;
            request->action=BEAM_MUSIC_VOLUME_SET;
            request->volume=(int)value;
        }
    } else return 0;
    cursor=trim(cursor);
    return !*cursor;
}

static int put_script(char *destination,unsigned int capacity,const char *source)
{
    if (!destination || !capacity || strlen(source)+1>capacity) return 0;
    strcpy(destination,source);return 1;
}

int beam_music_build_script(int player,const struct BeamMusicRequest *request,
                            char *script,unsigned int capacity)
{
    char generated[640],*marker;int value;
    if (!request || player<BEAM_MUSIC_HIPPO || player>BEAM_MUSIC_EAGLE) return 0;
    generated[0]=0;
    if (player==BEAM_MUSIC_HIPPO) {
        if (request->action==BEAM_MUSIC_STATUS)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET PLAY\nSAY 'HippoPlayer: play state' RESULT\n");
        else if (request->action==BEAM_MUSIC_PLAY)
            strcpy(generated,"ADDRESS 'HIPPOPLAYER'\nPLAY\nSAY 'HippoPlayer: play'\n");
        else if (request->action==BEAM_MUSIC_PAUSE)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET PLAY\nIF RESULT=1 THEN DO\nSTOP\nEND\nELSE DO\nCONT\nEND\nSAY 'HippoPlayer: play/pause toggled'\n");
        else if (request->action==BEAM_MUSIC_STOP)
            strcpy(generated,"ADDRESS 'HIPPOPLAYER'\nSTOP\nSAY 'HippoPlayer: stopped'\n");
        else if (request->action==BEAM_MUSIC_NEXT)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET NFIL\nmaxmod=RESULT\nGET CFIL\ncurrent=RESULT\nIF current<maxmod THEN DO\nCHOOSE current+1\nPLAY\nEND\nSAY 'HippoPlayer: next module'\n");
        else if (request->action==BEAM_MUSIC_PREVIOUS)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET CFIL\ncurrent=RESULT\nIF current>1 THEN DO\nCHOOSE current-1\nPLAY\nEND\nSAY 'HippoPlayer: previous module'\n");
        else if (request->action==BEAM_MUSIC_VOLUME_UP)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET VOLU\nv=RESULT+4\nIF v>64 THEN v=64\nVOLUME v\nSAY 'HippoPlayer: volume' v\n");
        else if (request->action==BEAM_MUSIC_VOLUME_DOWN)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'HIPPOPLAYER'\nGET VOLU\nv=RESULT-4\nIF v<0 THEN v=0\nVOLUME v\nSAY 'HippoPlayer: volume' v\n");
        else if (request->action==BEAM_MUSIC_VOLUME_SET) {
            value=(request->volume*64+50)/100;
            sprintf(generated,"ADDRESS 'HIPPOPLAYER'\nVOLUME %d\nSAY 'HippoPlayer: volume %d%%'\n",value,request->volume);
        }
    } else if (player==BEAM_MUSIC_AMIGAAMP) {
        if (request->action==BEAM_MUSIC_STATUS)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'AMIGAAMP'\nSTATUS\nSAY 'AmigaAMP:' RESULT\n");
        else if (request->action==BEAM_MUSIC_PLAY)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nPLAY\nSAY 'AmigaAMP: play'\n");
        else if (request->action==BEAM_MUSIC_PAUSE)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nPAUSE\nSAY 'AmigaAMP: play/pause toggled'\n");
        else if (request->action==BEAM_MUSIC_STOP)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nSTOP\nSAY 'AmigaAMP: stopped'\n");
        else if (request->action==BEAM_MUSIC_NEXT)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nNEXT\nSAY 'AmigaAMP: next track'\n");
        else if (request->action==BEAM_MUSIC_PREVIOUS)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nPREVIOUS\nSAY 'AmigaAMP: previous track'\n");
        else if (request->action==BEAM_MUSIC_VOLUME_UP)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nINCVOLUME\nSAY 'AmigaAMP: volume up'\n");
        else if (request->action==BEAM_MUSIC_VOLUME_DOWN)
            strcpy(generated,"ADDRESS 'AMIGAAMP'\nDECVOLUME\nSAY 'AmigaAMP: volume down'\n");
        else if (request->action==BEAM_MUSIC_VOLUME_SET) {
            value=(request->volume*26+50)/100;
            sprintf(generated,"ADDRESS 'AMIGAAMP'\nVOLUME %d\nSAY 'AmigaAMP: volume %d%%'\n",value,request->volume);
        }
    } else {
        if (request->action==BEAM_MUSIC_STATUS)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'rexx_EP'\nSTATUS G PLY\nstate=RESULT\nSTATUS M FIL\nSAY 'EaglePlayer 2:' state '-' RESULT\n");
        else if (request->action==BEAM_MUSIC_PLAY)
            strcpy(generated,"ADDRESS 'rexx_EP'\nPLAY 1\nSAY 'EaglePlayer 2: play'\n");
        else if (request->action==BEAM_MUSIC_PAUSE)
            strcpy(generated,"ADDRESS 'rexx_EP'\nPLAY\nSAY 'EaglePlayer 2: play/pause toggled'\n");
        else if (request->action==BEAM_MUSIC_STOP)
            strcpy(generated,"ADDRESS 'rexx_EP'\nSTOP\nSAY 'EaglePlayer 2: stopped'\n");
        else if (request->action==BEAM_MUSIC_NEXT)
            strcpy(generated,"ADDRESS 'rexx_EP'\nNEXTMODULE\nSAY 'EaglePlayer 2: next module'\n");
        else if (request->action==BEAM_MUSIC_PREVIOUS)
            strcpy(generated,"ADDRESS 'rexx_EP'\nPREVMODULE\nSAY 'EaglePlayer 2: previous module'\n");
        else if (request->action==BEAM_MUSIC_VOLUME_UP)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'rexx_EP'\nSTATUS G VOL\nv=RESULT+8\nIF v>255 THEN v=255\nVOLUME v\nSAY 'EaglePlayer 2: volume' v\n");
        else if (request->action==BEAM_MUSIC_VOLUME_DOWN)
            strcpy(generated,"OPTIONS RESULTS\nADDRESS 'rexx_EP'\nSTATUS G VOL\nv=RESULT-8\nIF v<0 THEN v=0\nVOLUME v\nSAY 'EaglePlayer 2: volume' v\n");
        else if (request->action==BEAM_MUSIC_VOLUME_SET) {
            value=(request->volume*255+50)/100;
            sprintf(generated,"ADDRESS 'rexx_EP'\nVOLUME %d\nSAY 'EaglePlayer 2: volume %d%%'\n",value,request->volume);
        }
    }
    if (!generated[0]) return 0;
    /* The wrapper accepts output only when our script reached its final SAY.
       This distinguishes a useful host reply from Rx's own error text. */
    marker=strstr(generated,"SAY '");
    if (!marker || strlen(generated)+7>=sizeof(generated)) return 0;
    marker+=5;
    memmove(marker+7,marker,strlen(marker)+1);
    memcpy(marker,"BEAMOK:",7);
    return put_script(script,capacity,generated);
}
