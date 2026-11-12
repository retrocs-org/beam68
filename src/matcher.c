#include "matcher.h"

/* A 256-byte fold table replaces a compare-and-branch per character; on a
 * 68000 every character comparison in the hot search loop goes through it. */
static unsigned char lower_table[256];
static int lower_ready;

static void lower_init(void)
{
    int c;
    for (c=0;c<256;c++)
        lower_table[c]=(unsigned char)((c>='A' && c<='Z')?c+('a'-'A'):c);
    lower_ready=1;
}

#define lower_ascii(c) ((int)lower_table[(unsigned char)(c)])

unsigned long beam_text_mask(const char *s)
{
    unsigned long mask=0;
    if (!lower_ready) lower_init();
    if (!s) return 0;
    while (*s) {
        int c=lower_ascii(*s++);
        if (c>='a' && c<='z') mask|=1UL<<(c-'a');
        else if (c>='0' && c<='9') mask|=1UL<<26;
        else if (c!=' ' && c!='\t' && c!=',' && c!=';') mask|=1UL<<27;
    }
    return mask;
}

static int word_char(int c)
{
    return c && c!=' ' && c!='-' && c!='_' && c!='.' && c!='/' && c!=':';
}

/* True when the two bounded strings differ by one insertion, deletion,
 * substitution or adjacent transposition.  This is intentionally limited to
 * one edit: it catches ordinary typing slips without making short queries
 * noisy or allocating a Levenshtein matrix on a small Amiga stack. */
static int one_edit(const char *a,int an,const char *b,int bn)
{
    int i=0,j=0,edits=0;
    if (an-bn>1 || bn-an>1) return 0;
    while (i<an && j<bn) {
        if (lower_ascii((unsigned char)a[i])==lower_ascii((unsigned char)b[j])) {
            ++i; ++j; continue;
        }
        if (++edits>1) return 0;
        if (an==bn && i+1<an && j+1<bn &&
            lower_ascii((unsigned char)a[i])==lower_ascii((unsigned char)b[j+1]) &&
            lower_ascii((unsigned char)a[i+1])==lower_ascii((unsigned char)b[j])) {
            i+=2; j+=2;
        } else if (an>bn) ++i;
        else if (bn>an) ++j;
        else { ++i; ++j; }
    }
    if (i<an || j<bn) ++edits;
    return edits==1;
}

static int fuzzy_word_prefix(const char *needle,const char *haystack)
{
    int qn=0,j;
    while (needle[qn]) ++qn;
    if (qn<3) return 0;
    for (j=0;haystack[j];++j) {
        int available=0,n;
        int boundary=(j==0 || !word_char((unsigned char)haystack[j-1]));
        if (!boundary) continue;
        while (word_char((unsigned char)haystack[j+available])) ++available;
        for (n=qn-1;n<=qn+1;n++) {
            if (n>0 && available>=n && one_edit(needle,qn,haystack+j,n))
                return 3000-j;
        }
    }
    return 0;
}

int beam_match_score(const char *needle, const char *haystack)
{
    int i, j, first = -1, matched = 0, gaps = 0, prev = -1;
    int best = 0;
    if (!needle || !haystack || !needle[0]) return 0;
    if (!lower_ready) lower_init();

    /* Exact name is always first, then starts-with. */
    i = 0;
    while (needle[i] && haystack[i] &&
           lower_ascii((unsigned char)needle[i]) == lower_ascii((unsigned char)haystack[i]))
        ++i;
    if (!needle[i] && !haystack[i]) return 10000;
    if (!needle[i]) return 8000 - i;

    /* Word-boundary prefix and ordinary substring. */
    for (j = 0; haystack[j]; ++j) {
        int n = 0;
        while (needle[n] && haystack[j+n] &&
               lower_ascii((unsigned char)needle[n]) == lower_ascii((unsigned char)haystack[j+n]))
            ++n;
        if (!needle[n]) {
            int boundary = (j == 0 || haystack[j-1] == ' ' ||
                            haystack[j-1] == '-' || haystack[j-1] == '_');
            int score = (boundary ? 6000 : 4000) - j;
            if (score > best) best = score;
        }
    }
    if (best > 0) return best;

    /* Initial-letter abbreviations: "dp" finds "Deluxe Paint", "wb"
     * finds "WorkBench". A capital after lowercase also starts a word.
     * Require at least two initials; ordinary single-letter matching already
     * has higher-quality prefix / substring paths above. */
    if (needle[0] && needle[1]) {
        int n = 0, initials = 0, start = -1, last = -1;
        for (j=0; haystack[j] && needle[n]; ++j) {
            int c=(unsigned char)haystack[j];
            int prevc=j?(unsigned char)haystack[j-1]:0;
            int boundary=(j==0 || prevc==' ' || prevc=='-' ||
                          prevc=='_' || prevc=='.' || prevc=='/' ||
                          (c>='A' && c<='Z' &&
                           ((prevc>='a' && prevc<='z') ||
                            (prevc>='0' && prevc<='9'))));
            if (!boundary) continue;
            ++initials;
            if (lower_ascii(c)==lower_ascii((unsigned char)needle[n])) {
                if (start<0) start=j;
                last=j;
                ++n;
            }
        }
        if (!needle[n] && n>=2 && initials>=2)
            return 5500 - (start>0?start*2:0) - (last-start);
    }

    /* One-edit typo tolerance at a word prefix.  It deliberately ranks below
     * substrings and abbreviations, but above a loose subsequence. */
    best=fuzzy_word_prefix(needle,haystack);
    if (best>0) return best;

    /* Subsequent letters in sequence, with a penalty for gaps. */
    for (j = 0; haystack[j] && needle[matched]; ++j) {
        if (lower_ascii((unsigned char)needle[matched]) == lower_ascii((unsigned char)haystack[j])) {
            if (first < 0) first = j;
            if (prev >= 0) gaps += j-prev-1;
            prev = j;
            ++matched;
        }
    }
    if (needle[matched] || first < 0) return 0;
    return 2000 - first * 4 - gaps * 3 > 1 ? 2000 - first * 4 - gaps * 3 : 1;
}

/* Bit i set = haystack[i] took part in the match (first 32 characters).
 * Mirrors the classes of beam_match_score so the highlighted letters are
 * the ones that produced the score. */
static unsigned long span(int from,int count)
{
    unsigned long m=0;
    int i;
    for (i=from;i<from+count && i<32;i++) if (i>=0) m|=1UL<<i;
    return m;
}

unsigned long beam_match_marks(const char *needle,const char *haystack)
{
    int i,j,n=0,best=0,best_j=-1,qn;
    unsigned long marks=0;
    if (!needle || !haystack || !needle[0]) return 0;
    if (!lower_ready) lower_init();
    while (needle[n]) ++n;
    qn=n;
    for (i=0;needle[i] && haystack[i] && lower_ascii(needle[i])==lower_ascii(haystack[i]);++i) { }
    if (!needle[i]) return span(0,qn);                       /* exact/prefix */
    for (j=0;haystack[j];++j) {                              /* substring */
        for (n=0;needle[n] && haystack[j+n] &&
                 lower_ascii(needle[n])==lower_ascii(haystack[j+n]);++n) { }
        if (!needle[n]) {
            int boundary=(j==0 || haystack[j-1]==' ' || haystack[j-1]=='-' || haystack[j-1]=='_');
            int score=(boundary?6000:4000)-j;
            if (score>best) { best=score; best_j=j; }
        }
    }
    if (best_j>=0) return span(best_j,qn);
    if (needle[1]) {                                         /* initials */
        int initials=0;
        n=0;
        for (j=0;haystack[j] && needle[n];++j) {
            int c=(unsigned char)haystack[j],prevc=j?(unsigned char)haystack[j-1]:0;
            int boundary=(j==0 || prevc==' ' || prevc=='-' || prevc=='_' || prevc=='.' ||
                          prevc=='/' || (c>='A' && c<='Z' &&
                          ((prevc>='a' && prevc<='z') || (prevc>='0' && prevc<='9'))));
            if (!boundary) continue;
            ++initials;
            if (lower_ascii(c)==lower_ascii(needle[n])) { if (j<32) marks|=1UL<<j; ++n; }
        }
        if (!needle[n] && n>=2 && initials>=2) return marks;
        marks=0;
    }
    if (qn>=3) {                                             /* one typo */
        for (j=0;haystack[j];++j) {
            int available=0;
            if (j && word_char((unsigned char)haystack[j-1])) continue;
            while (word_char((unsigned char)haystack[j+available])) ++available;
            for (n=qn-1;n<=qn+1;n++)
                if (n>0 && available>=n && one_edit(needle,qn,haystack+j,n)) return span(j,n);
        }
    }
    n=0;                                                     /* subsequence */
    for (j=0;haystack[j] && needle[n];++j)
        if (lower_ascii(needle[n])==lower_ascii(haystack[j])) { if (j<32) marks|=1UL<<j; ++n; }
    return needle[n]?0:marks;
}
