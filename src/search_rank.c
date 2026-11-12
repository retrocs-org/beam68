#include "search_rank.h"
#include "matcher.h"

static int query_terms_score(const char *query,const char *name,
                             const char *alias_words)
{
    char term[96];
    const char *p=query;
    int terms=0,weakest=10000;
    while (*p) {
        unsigned int n=0;
        int score,alias_score=0;
        while (*p==' ' || *p=='\t' || *p==',' || *p==';') ++p;
        if (!*p) break;
        while (*p && *p!=' ' && *p!='\t' && *p!=',' && *p!=';') {
            if (n+1<sizeof(term)) term[n++]=*p;
            ++p;
        }
        if (!n) continue;
        term[n]=0;
        score=beam_match_score(term,name);
        if (alias_words) alias_score=beam_match_score(term,alias_words);
        if (alias_score>score) score=alias_score;
        if (!score) return 0;
        if (score<weakest) weakest=score;
        ++terms;
    }
    if (terms<2) return 0;
    /* Multi-term matches sit below a full word-boundary phrase, but above an
     * ordinary substring -- only when every term is itself at least a real
     * substring match.  Loose typo/subsequence terms stay in a low class so
     * scattered letters cannot outrank a genuine single substring hit. */
    if (weakest<4000) return 1000+weakest/4;
    weakest=5200+weakest/20;
    return weakest>5799?5799:weakest;
}

static int has_separator(const char *s)
{
    for (;*s;s++) if (*s==' ' || *s=='\t' || *s==',' || *s==';') return 1;
    return 0;
}

void beam_query_masks_build(const char *query,struct BeamQueryMasks *m)
{
    char term[96];
    const char *p=query;
    m->count=0;
    if (!query) return;
    while (*p && m->count<BEAM_QUERY_MASK_TERMS) {
        unsigned int n=0;
        while (*p==' ' || *p=='\t' || *p==',' || *p==';') ++p;
        if (!*p) break;
        while (*p && *p!=' ' && *p!='\t' && *p!=',' && *p!=';') {
            if (n+1<sizeof(term)) term[n++]=*p;
            ++p;
        }
        term[n]=0;
        m->term[m->count++]=beam_text_mask(term);
    }
}

int beam_query_masks_reject(const struct BeamQueryMasks *m,unsigned long item_mask)
{
    int i;
    for (i=0;i<m->count;i++) {
        unsigned long missing=m->term[i]&~item_mask;
        /* Two or more distinct absent characters: no match class can hit. */
        if (missing & (missing-1UL)) return 1;
    }
    return 0;
}

int beam_search_rank_alias(const char *query,const char *name,
                           const char *alias_words,int pinned_position,
                           unsigned int history_bonus)
{
    int score,alias_score=0;
    char word[96];
    unsigned int n=0;
    const char *p;
    if (!query || !name) return 0;
    if (!*query) return pinned_position<0?0:10000-pinned_position;
    score=beam_match_score(query,name);
    /* Alias matches assist discovery but never outrank an exact real name. */
    /* Aliases are capped at 7500: a stronger real-name match cannot be
     * improved, so avoid all alias fuzzy matching in this common case. */
    if (score<7500 && alias_words) for (p=alias_words;;p++) {
        int separator=!*p || *p==' ' || *p==',' || *p==';';
        if (!separator && n+1<sizeof(word)) word[n++]=*p;
        if (separator && n) {
            int candidate;
            word[n]=0; candidate=beam_match_score(query,word);
            if (candidate>alias_score) alias_score=candidate;
            n=0;
        }
        if (!*p) break;
    }
    if (alias_score>7500) alias_score=7500;
    if (alias_score>score) score=alias_score;
    if (score<5800 && has_separator(query)) {
        alias_score=query_terms_score(query,name,alias_words);
        if (alias_score>score) score=alias_score;
    }
    if (!score) return 0;
    if (pinned_position>=0) score+=180;
    /* A bounded tie-breaker; never outranks a different match class. */
    if (history_bonus>144) history_bonus=144;
    score+=(int)history_bonus;
    return score;
}

int beam_search_rank(const char *query,const char *name,
                     int pinned_position,unsigned int history_bonus)
{
    return beam_search_rank_alias(query,name,0,pinned_position,history_bonus);
}

unsigned long beam_query_marks(const char *query,const char *name)
{
    char term[96];
    const char *p=query;
    unsigned long marks=0;
    if (!query || !name || !*query) return 0;
    if (!has_separator(query)) return beam_match_marks(query,name);
    while (*p) {
        unsigned int n=0;
        while (*p==' ' || *p=='\t' || *p==',' || *p==';') ++p;
        if (!*p) break;
        while (*p && *p!=' ' && *p!='\t' && *p!=',' && *p!=';') {
            if (n+1<sizeof(term)) term[n++]=*p;
            ++p;
        }
        term[n]=0;
        marks|=beam_match_marks(term,name);
    }
    return marks;
}
