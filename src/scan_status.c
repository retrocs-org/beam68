#include <stdio.h>
#include <string.h>
#include "scan_status.h"
#include "locale_support.h"
int beam_scan_status_format(const struct BeamScanStatus *s, char *out,
                            unsigned int capacity)
{
    char line[256];
    unsigned int n;
    if (!out || !capacity) return 0;
    out[0]=0;
    if (!s) return 0;
    /* 7 bounded decimal unsigned longs (max 10 digits each) plus labels.
       The 256-byte temporary buffer is sufficient on 32-bit AmigaOS. */
    sprintf(line,"scanning=%d roots=%lu/%lu entries=%lu apps=%lu unreadable=%lu ioerrors=%lu overlong=%lu capped=%d cached=%d",
            !!s->scanning,s->roots_done,s->roots_total,s->entries,
            s->applications,s->unreadable,s->io_errors,s->overlong,
            !!s->capped,!!s->cached);
    n=(unsigned int)strlen(line);
    if (n+1>capacity) return 0;
    memcpy(out,line,n+1);
    return 1;
}

int beam_scan_progress_format(const struct BeamScanStatus *s,char *out,
                              unsigned int capacity)
{
    char line[160]; unsigned int n;
    if (!out || !capacity) return 0;
    out[0]=0; if (!s) return 0;
    if (s->cached)
        sprintf(line,beam_str(MSG_PROGRESS_CACHED),s->applications);
    else if (s->scanning)
        sprintf(line,beam_str(MSG_PROGRESS_SCANNING),
            s->roots_done,s->roots_total,s->entries,s->applications);
    else if (s->capped)
        sprintf(line,beam_str(MSG_PROGRESS_FULL),s->applications);
    else if (s->unreadable || s->io_errors || s->overlong)
        sprintf(line,beam_str(MSG_PROGRESS_PROBLEMS),
            s->applications,s->unreadable+s->overlong,s->io_errors);
    else
        sprintf(line,beam_str(MSG_PROGRESS_DONE),s->applications,s->roots_total);
    n=(unsigned int)strlen(line);
    if (n+1>capacity) return 0;
    memcpy(out,line,n+1); return 1;
}

/* Keep the cache decision portable and independently tested. */
int beam_scan_cacheable(const struct BeamScanStatus *s)
{
    return s && !s->scanning && !s->capped && !s->unreadable &&
           !s->io_errors && !s->overlong &&
           s->roots_done==s->roots_total;
}
