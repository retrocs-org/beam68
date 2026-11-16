#ifndef BEAM_PLUGIN_WALK_H
#define BEAM_PLUGIN_WALK_H
#include "beam.h"
struct BeamWalkFrame;
struct BeamWalk {
    const struct BeamConfig *config;
    struct BeamWalkFrame *top;
    const char *override;
    int root,unreadable;
};
void beam_walk_begin(struct BeamWalk *walk,const struct BeamConfig *config,const char *override);
int beam_walk_next(struct BeamWalk *walk,char *path,unsigned int cap,int *kind,int *root);
void beam_walk_end(struct BeamWalk *walk);
#endif
