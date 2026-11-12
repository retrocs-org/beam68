#ifndef BEAM68_COMPLETION_H
#define BEAM68_COMPLETION_H
#include "beam.h"

#define BEAM_COMPLETION_CAP 6
struct BeamCompletions {
    char values[BEAM_COMPLETION_CAP][BEAM_QUERY_LEN+1];
    unsigned char count;
    unsigned char selected;
};

void beam_completion_reset(struct BeamCompletions *list);
int beam_completion_add(struct BeamCompletions *list,const char *query,
                        const char *candidate);
int beam_completion_move(struct BeamCompletions *list,int direction);
const char *beam_completion_selected(const struct BeamCompletions *list);

#endif
