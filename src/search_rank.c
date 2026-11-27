#include "search_rank.h"
#include "matcher.h"
#include "beam.h"
#include <string.h>

const char *beam_search_builtin_alias(const char *name)
{
    const char *expected="beam68prefs";
    if (!name) return 0;
    while (*name && *expected) {
        unsigned char c=(unsigned char)*name++;
        if(c>='A' && c<='Z') c+=32;
        if(c!=(unsigned char)*expected++) return 0;
    }
    return !*name && !*expected?"beam68 preferences settings":0;
}

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
    /* Reserve separate bands: exact, prefix, alias, boundary, initials/terms,
     * substring, typo and subsequence. Favorites/frequency never cross bands.
     * Values remain below signed 16-bit INT_MAX on classic toolchains. */
    {
        int band=score>=10000?7:score>7500?6:score>6000?5:
                 score>=5800?4:score>=5000?3:score>=3500?2:score>=2500?1:0;
        int floor=band==7?10000:band==6?7501:band==5?6001:
                  band==4?5800:band==3?5000:band==2?3500:band==1?2500:0;
        score=band*4000+(score-floor)/2+1;
    }
    if (pinned_position>=0) score+=1800;
    if (history_bonus>144) history_bonus=144;
    score+=(int)history_bonus*4;
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

static int same_path(const char *a,const char *b)
{
    while(*a&&*b){unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x+=32;
        if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return 0;
    }return !*a&&!*b;
}
int beam_search_find_selection(const struct BeamItem *items,int count,const int *results,int n,const char *path)
{
    int i;if(!items||!results||!path||!*path)return -1;
    for(i=0;i<n;++i)if(results[i]>=0&&results[i]<count&&same_path(items[results[i]].path,path))return i;
    return -1;
}
int beam_search_selection(const struct BeamItem *items,int count,const int *results,int n,const char *path)
{ int found=beam_search_find_selection(items,count,results,n,path);return found<0?0:found; }

static int rank_text_compare(const char *a,const char *b)
{
    while(*a && *b){int x=(unsigned char)*a++,y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x+=32;
        if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return x-y;
    }
    return (unsigned char)*a-(unsigned char)*b;
}
int beam_search_before(int score,const char *name,const char *path,
                       int oldscore,const char *oldname,const char *oldpath)
{
    int order;
    if(score!=oldscore)return score>oldscore;
    order=rank_text_compare(name,oldname);
    if(order)return order<0;
    order=rank_text_compare(path,oldpath);
    return order?order<0:strcmp(path,oldpath)<0;
}

static int result_before(const struct BeamItem *items,int index,int score,
                         int oldindex,int oldscore,unsigned int shared_prefix)
{
    unsigned int skip=score==oldscore && score>=24000?shared_prefix:0;
    return beam_search_before(score,items[index].name+skip,items[index].path,
                               oldscore,items[oldindex].name+skip,items[oldindex].path);
}
/* Already sorted, bounded collector: cheap best/worst rejection, binary
 * placement and a single bulk move. Reverse directory order stays fast.
 * shared_prefix is the query length: only equal real-name prefix/exact
 * score bands skip those known-equal characters. Alias/fuzzy bands never do. */
void beam_search_top_add(const struct BeamItem *items,int index,int score,
                         int *results,int *scores,int *count,int capacity,
                         unsigned int shared_prefix)
{
    int position=0,n=*count,low,high,middle,moved;
    if(capacity<1)return;
    if(n && !result_before(items,index,score,results[0],scores[0],shared_prefix)) {
        if(n==capacity && !result_before(items,index,score,results[n-1],scores[n-1],shared_prefix))return;
        low=1;high=n;
        while(low<high) {
            middle=(low+high)/2;
            if(result_before(items,index,score,results[middle],scores[middle],shared_prefix))high=middle;
            else low=middle+1;
        }
        position=low;
    }
    if(position>=capacity)return;
    if(n<capacity)++n;
    moved=n-position-1;
    if(moved>0) {
        memmove(results+position+1,results+position,(unsigned int)moved*sizeof(*results));
        memmove(scores+position+1,scores+position,(unsigned int)moved*sizeof(*scores));
    }
    results[position]=index;scores[position]=score;*count=n;
}
