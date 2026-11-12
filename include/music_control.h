#ifndef BEAM68_MUSIC_CONTROL_H
#define BEAM68_MUSIC_CONTROL_H

#define BEAM_MUSIC_AUTO 0
#define BEAM_MUSIC_HIPPO 1
#define BEAM_MUSIC_AMIGAAMP 2
#define BEAM_MUSIC_EAGLE 3

#define BEAM_MUSIC_STATUS 1
#define BEAM_MUSIC_PLAY 2
#define BEAM_MUSIC_PAUSE 3
#define BEAM_MUSIC_STOP 4
#define BEAM_MUSIC_NEXT 5
#define BEAM_MUSIC_PREVIOUS 6
#define BEAM_MUSIC_VOLUME_UP 7
#define BEAM_MUSIC_VOLUME_DOWN 8
#define BEAM_MUSIC_VOLUME_SET 9
#define BEAM_MUSIC_PLAYERS 10

struct BeamMusicRequest {
    int player;
    int action;
    int volume;
};

int beam_music_parse(const char *text,struct BeamMusicRequest *request);
int beam_music_player_from_name(const char *name);
const char *beam_music_player_name(int player);
const char *beam_music_player_port(int player);
int beam_music_build_script(int player,const struct BeamMusicRequest *request,
                            char *script,unsigned int capacity);

#endif
