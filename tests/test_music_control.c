#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "music_control.h"

int main(void)
{
    struct BeamMusicRequest r;
    char script[640];
    assert(beam_music_parse("play",&r) && r.action==BEAM_MUSIC_PLAY && r.player==BEAM_MUSIC_AUTO);
    assert(beam_music_parse("hippo next",&r) && r.player==BEAM_MUSIC_HIPPO && r.action==BEAM_MUSIC_NEXT);
    assert(beam_music_parse("amigaamp volume 50",&r) && r.player==BEAM_MUSIC_AMIGAAMP && r.action==BEAM_MUSIC_VOLUME_SET && r.volume==50);
    assert(beam_music_parse("eagle pause",&r) && r.player==BEAM_MUSIC_EAGLE && r.action==BEAM_MUSIC_PAUSE);
    assert(beam_music_parse("players",&r) && r.action==BEAM_MUSIC_PLAYERS);
    assert(!beam_music_parse("volume 101",&r));
    assert(!beam_music_parse("play extra",&r));
    r.action=BEAM_MUSIC_NEXT;r.volume=0;
    assert(beam_music_build_script(BEAM_MUSIC_HIPPO,&r,script,sizeof(script)));
    assert(strstr(script,"HIPPOPLAYER") && strstr(script,"CHOOSE current+1"));
    r.action=BEAM_MUSIC_STATUS;
    assert(beam_music_build_script(BEAM_MUSIC_HIPPO,&r,script,sizeof(script)));
    assert(strstr(script,"BEAMOK:HippoPlayer: play state"));
    r.action=BEAM_MUSIC_PAUSE;
    assert(beam_music_build_script(BEAM_MUSIC_AMIGAAMP,&r,script,sizeof(script)));
    assert(strstr(script,"AMIGAAMP") && strstr(script,"PAUSE"));
    r.action=BEAM_MUSIC_VOLUME_SET;r.volume=100;
    assert(beam_music_build_script(BEAM_MUSIC_EAGLE,&r,script,sizeof(script)));
    assert(strstr(script,"VOLUME 255"));
    assert(!beam_music_build_script(BEAM_MUSIC_AUTO,&r,script,sizeof(script)));
    puts("Music controls parsing and player-specific ARexx mapping: PASS");
    return 0;
}
