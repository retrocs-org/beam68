#include <string.h>
#include "launcher_ui.h"

/* Returns 1 if text OR cursor changes; 0 for ignored/invalid operations.
 * The buffer always remains NUL-terminated and no heap work is required. */
int beam_query_edit(char *text, unsigned int capacity, int *cursor,
                    int operation, char value)
{
    unsigned int n,at;
    if (!text || !cursor || capacity<2) return 0;
    n=(unsigned int)strlen(text);
    if (n>=capacity) return 0;
    if (*cursor<0) *cursor=0;
    if ((unsigned int)*cursor>n) *cursor=(int)n;
    at=(unsigned int)*cursor;
    switch (operation) {
    case BEAM_EDIT_INSERT:
        if ((unsigned char)value<32 || (unsigned char)value>126 || n+1>=capacity)
            return 0;
        memmove(text+at+1,text+at,(size_t)(n-at+1));
        text[at]=value;
        ++*cursor;
        return 1;
    case BEAM_EDIT_BACKSPACE:
        if (!at) return 0;
        memmove(text+at-1,text+at,(size_t)(n-at+1));
        --*cursor;
        return 1;
    case BEAM_EDIT_DELETE:
        if (at>=n) return 0;
        memmove(text+at,text+at+1,(size_t)(n-at));
        return 1;
    case BEAM_EDIT_LEFT:
        if (!at) return 0;
        --*cursor; return 1;
    case BEAM_EDIT_RIGHT:
        if (at>=n) return 0;
        ++*cursor; return 1;
    case BEAM_EDIT_HOME:
        if (!at) return 0;
        *cursor=0; return 1;
    case BEAM_EDIT_END:
        if (at==n) return 0;
        *cursor=(int)n; return 1;
    case BEAM_EDIT_CLEAR:
        if (!n) return 0;
        text[0]=0; *cursor=0; return 1;
    default: return 0;
    }
}

/* Navigation has no heap state and works with the bounded search list.
 * direction < 0 moves up, > 0 moves down. Ctrl takes priority over Shift.
 * Single-row movement wraps; deliberate page and edge jumps remain bounded. */
int beam_result_navigate(int current, int total, int direction,
                         int shift, int control, int page_size)
{
    int step;
    if (total<=0) return -1;
    if (current<0) current=0;
    if (current>=total) current=total-1;
    if (!direction) return current;
    if (control) return direction<0 ? 0 : total-1;
    step=(shift && page_size>1) ? page_size : 1;
    if (!shift) {
        if (direction<0) return current==0?total-1:current-1;
        return current==total-1?0:current+1;
    }
    if (direction<0) return current<step ? 0 : current-step;
    return total-1-current<step ? total-1 : current+step;
}

/* Number of complete rows left between input line and status bar. */
int beam_result_rows(int row_top, int lineheight, int usable_bottom,
                     int maximum)
{
    int rows;
    if (lineheight<=0 || maximum<=0 || usable_bottom<=row_top) return 0;
    rows=(usable_bottom-row_top)/lineheight;
    return rows<maximum?rows:maximum;
}
int beam_result_scroll(int selected, int first, int row_count, int total)
{
    if (row_count<=0 || total<=0) return 0;
    if (selected<0) selected=0;
    if (selected>=total) selected=total-1;
    if (first>selected) first=selected;
    if (selected>=first+row_count) first=selected-row_count+1;
    if (first>total-row_count) first=total-row_count;
    return first<0?0:first;
}

/* Hit testing shares the same geometry as the drawing loop. A click in
 * the input field, below the visible rows, or in the status bar is ignored. */
int beam_hit_result(int y, int row_top, int lineheight,
                    int usable_bottom, int result_count)
{
    int index;
    if (lineheight<=0 || y<row_top-2 || y>=usable_bottom) return -1;
    index=(y-(row_top-2))/lineheight;
    if (index<0 || index>=result_count) return -1;
    if (row_top+index*lineheight+lineheight>usable_bottom) return -1;
    return index;
}

unsigned int beam_text_line(const char *text,unsigned int fit,unsigned int *next)
{
    unsigned int n=0,end;
    if (next) *next=0;
    if (!text || !fit) return 0;
    while (n<fit && text[n] && text[n]!='\n') ++n;
    end=n;
    if (n==fit && text[n] && text[n]!=' ' && text[n]!='\n') {
        unsigned int word=n;
        while (word && text[word-1]!=' ') --word;
        if (word) end=n=word;
    }
    while (end && text[end-1]==' ') --end;
    while (text[n]==' ' || text[n]=='\n') ++n;
    if (next) *next=n;
    return end;
}
