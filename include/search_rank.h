#ifndef BEAM_SEARCH_RANK_H
#define BEAM_SEARCH_RANK_H
/* Bounded ranking; positive scores only. pinned_position is -1 if not pinned.
 * Empty search displays ONLY favorites ordered by pin position. */
int beam_search_rank(const char *query,const char *name,
                     int pinned_position,unsigned int history_bonus);
int beam_search_rank_alias(const char *query,const char *name,
                           const char *alias_words,int pinned_position,
                           unsigned int history_bonus);

/* Per-term character masks for a cheap, sound early reject (see matcher.h). */
#define BEAM_QUERY_MASK_TERMS 8
struct BeamQueryMasks { unsigned long term[BEAM_QUERY_MASK_TERMS]; int count; };
void beam_query_masks_build(const char *query,struct BeamQueryMasks *m);
int beam_query_masks_reject(const struct BeamQueryMasks *m,unsigned long item_mask);
/* Highlight positions of name for a (possibly multi-word) query. */
unsigned long beam_query_marks(const char *query,const char *name);
#endif
