#include <string.h>
#include "rexx_sequence.h"
int beam_rexx_handle_valid(const char *handle)
{
    unsigned int n=0;
    if (!handle || !*handle) return 0;
    while (handle[n]>='0' && handle[n]<='9') ++n;
    return n<24 && !handle[n];
}
static int add(char *out,unsigned int cap,const char *text)
{
    unsigned int a=(unsigned int)strlen(out),b=(unsigned int)strlen(text);
    if (a>=cap || b>=cap-a) return 0;
    memcpy(out+a,text,b+1); return 1;
}
int beam_rexx_next(int stage,int ok,const char *handle,const char *drawer,
    const char *select,char *out,unsigned int cap)
{
    int next=0;
    if (!out || !cap || !drawer || !select) return -1;
    out[0]=0;
    if (strpbrk(drawer,"\"*\r\n") || strpbrk(select,"\"*\r\n")) return -1;
    if (stage==REXX_DOPUS_NEW && !ok) {
        if (!add(out,cap,"ScanDir \"") || !add(out,cap,drawer) || !add(out,cap,"\"")) return -1;
        return REXX_DOPUS_SCAN;
    }
    if (stage==REXX_DOPUS_NEW && ok && *select && handle && *handle) next=REXX_DOPUS_WAIT;
    else if (stage==REXX_DOPUS_WAIT && ok) next=REXX_DOPUS_SELECT;
    else if (stage==REXX_DOPUS_SELECT && ok) next=REXX_DOPUS_REFRESH;
    if (!next) return 0;
    if (!beam_rexx_handle_valid(handle)) return -1;
    if (!add(out,cap,next==REXX_DOPUS_WAIT?"lister wait ":
            next==REXX_DOPUS_SELECT?"lister select ":"lister refresh ") ||
        !add(out,cap,handle)) return -1;
    if (next==REXX_DOPUS_SELECT &&
        (!add(out,cap," \"") || !add(out,cap,select) || !add(out,cap,"\" on"))) return -1;
    return next;
}
