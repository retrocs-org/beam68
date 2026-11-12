#include <string.h>
#include "completion.h"

static int lower_ascii(int c)
{ return c>='A' && c<='Z' ? c+32 : c; }

static int prefix_ci(const char *text,const char *prefix)
{
    while (*prefix) {
        if (!*text || lower_ascii((unsigned char)*text++)!=
                      lower_ascii((unsigned char)*prefix++)) return 0;
    }
    return 1;
}

static int equal_ci(const char *a,const char *b)
{
    while (*a && *b)
        if (lower_ascii((unsigned char)*a++)!=lower_ascii((unsigned char)*b++))
            return 0;
    return !*a && !*b;
}

void beam_completion_reset(struct BeamCompletions *list)
{
    if (!list) return;
    memset(list,0,sizeof(*list));
}

int beam_completion_add(struct BeamCompletions *list,const char *query,
                        const char *candidate)
{
    unsigned int i,n;
    if (!list || !query || !*query || !candidate) return 0;
    n=(unsigned int)strlen(candidate);
    if (n<=strlen(query) || n>BEAM_QUERY_LEN || !prefix_ci(candidate,query))
        return 0;
    for (i=0;i<list->count;i++)
        if (equal_ci(list->values[i],candidate)) return 0;
    if (list->count>=BEAM_COMPLETION_CAP) return 0;
    strcpy(list->values[list->count++],candidate);
    return 1;
}

int beam_completion_move(struct BeamCompletions *list,int direction)
{
    int next;
    if (!list || !list->count || !direction) return -1;
    next=(int)list->selected+(direction<0?-1:1);
    if (next<0) next=(int)list->count-1;
    if (next>=(int)list->count) next=0;
    list->selected=(unsigned char)next;
    return next;
}

const char *beam_completion_selected(const struct BeamCompletions *list)
{
    if (!list || !list->count || list->selected>=list->count) return NULL;
    return list->values[list->selected];
}
