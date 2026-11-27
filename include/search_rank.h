#ifndef BEAM_SEARCH_RANK_H
#define BEAM_SEARCH_RANK_H
/* Bounded ranking; positive scores only. pinned_position is -1 if not pinned.
 * Empty search displays ONLY favorites ordered by pin position. */
int beam_search_rank(const char *query,const char *name,
                     int pinned_position,unsigned int history_bonus);
int beam_search_rank_alias(const char *query,const char *name,
                           const char *alias_words,int pinned_position,
                           unsigned int history_bonus);

/* Search words for built-in application names; NULL for other names. */
const char *beam_search_builtin_alias(const char *name);

/* Per-term character masks for a cheap, sound early reject (see matcher.h). */
#define BEAM_QUERY_MASK_TERMS 8
struct BeamQueryMasks { unsigned long term[BEAM_QUERY_MASK_TERMS]; int count; };
void beam_query_masks_build(const char *query,struct BeamQueryMasks *m);
int beam_query_masks_reject(const struct BeamQueryMasks *m,unsigned long item_mask);
/* Highlight positions of name for a (possibly multi-word) query. */
unsigned long beam_query_marks(const char *query,const char *name);
struct BeamItem;
/* Resolve the selected path after result reordering or index compaction. */
/* Returns -1 when the old target disappeared; callers must not launch a replacement. */
int beam_search_find_selection(const struct BeamItem *items,int count,const int *results,
                               int result_count,const char *path);
int beam_search_selection(const struct BeamItem *items,int count,const int *results,
                          int result_count,const char *path);
/* Stable alphabetical/path order for equal scores, independent of scan order. */
int beam_search_before(int score,const char *name,const char *path,
                       int oldscore,const char *oldname,const char *oldpath);
/* Bounded sorted collector. shared_prefix is a verified real-name query
 * prefix length; pass zero when collecting arbitrary scores. */
void beam_search_top_add(const struct BeamItem *items,int index,int score,
                         int *results,int *scores,int *count,int capacity,
                         unsigned int shared_prefix);
#endif
