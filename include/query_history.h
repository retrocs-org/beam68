#ifndef BEAM68_QUERY_HISTORY_H
#define BEAM68_QUERY_HISTORY_H
#include "beam.h"
#define BEAM_QUERY_HISTORY_CAP 12
#define BEAM_QUERY_HISTORY_PATH "ENVARC:Beam68/Queries.txt"
struct BeamQueryHistory {
    char entries[BEAM_QUERY_HISTORY_CAP][BEAM_QUERY_LEN+1];
    unsigned short count, pending, cursor;
};
void beam_query_history_reset(struct BeamQueryHistory *h);
void beam_query_history_record(struct BeamQueryHistory *h,const char *query);
const char *beam_query_history_older(struct BeamQueryHistory *h);
const char *beam_query_history_newer(struct BeamQueryHistory *h);
void beam_query_history_end(struct BeamQueryHistory *h);
int beam_query_history_load(struct BeamQueryHistory *h,const char *path);
int beam_query_history_save(struct BeamQueryHistory *h,const char *path);
#endif
