#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "query_history.h"
#define PATH "ENVARC:Beam68/Queries.txt"
int main(void)
{
    struct BeamQueryHistory h,r; const char *q; int i; char line[32];
    beam_query_history_reset(&h);
    assert(!beam_query_history_older(&h));
    beam_query_history_record(&h,"screen mode");
    beam_query_history_record(&h,">version");
    beam_query_history_record(&h,"screen mode");
    assert(h.count==2 && !strcmp(h.entries[1],"screen mode"));
    q=beam_query_history_older(&h); assert(q && !strcmp(q,"screen mode"));
    q=beam_query_history_older(&h); assert(q && !strcmp(q,">version"));
    q=beam_query_history_newer(&h); assert(q && !strcmp(q,"screen mode"));
    q=beam_query_history_newer(&h); assert(q && !*q);
    for(i=0;i<20;i++){sprintf(line,"query %d",i);beam_query_history_record(&h,line);}
    assert(h.count==BEAM_QUERY_HISTORY_CAP);
    assert(beam_query_history_save(&h,PATH));
    assert(beam_query_history_load(&r,PATH));
    assert(r.count==BEAM_QUERY_HISTORY_CAP && !strcmp(r.entries[r.count-1],"query 19"));
    puts("Recent query history, recall, dedup and persistence: PASS");
    return 0;
}
