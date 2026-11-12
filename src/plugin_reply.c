/* One-file atomic reply publisher. The ARexx text transport never exposes
 * an incomplete reply. Native plugins use Exec messages instead.
 * Works on 68000 AmigaDOS. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "plugin_reply.h"
#define REPLY_PATH_MAX 240
#define REPLY_LINE_MAX 127
int beam_plugin_publish_reply(const char *destination, const char *line)
{
    char temporary[REPLY_PATH_MAX+8];
    unsigned int n,i;
    BPTR f, existing;
    int ok;
    if (!destination || !line || !*destination || !strchr(destination,':') ||
        strlen(destination)>=REPLY_PATH_MAX-5 ||
        strchr(destination,'\n') || strchr(destination,'\r') ||
        strchr(destination,'"') || strchr(destination,'*')) return 0;
    n=(unsigned int)strlen(line);
    if (n<6 || n>REPLY_LINE_MAX || line[n-1]!='\n' ||
        (strncmp(line,"TEXT=",5) && strncmp(line,"ERROR=",6))) return 0;
    for (i=0;i<n-1;i++)
        if ((unsigned char)line[i]<32 || (unsigned char)line[i]==127) return 0;
    existing=Lock((STRPTR)destination,ACCESS_READ);
    if (existing) { UnLock(existing); return 0; }
    sprintf(temporary,"%s.part",destination);
    f=Open((STRPTR)temporary,MODE_NEWFILE);
    if (!f) return 0;
    ok=FPuts(f,(STRPTR)line)>=0;
    if (!Close(f)) ok=0;
    if (!ok) { DeleteFile((STRPTR)temporary);return 0; }
    if (!Rename((STRPTR)temporary,(STRPTR)destination)) {
        DeleteFile((STRPTR)temporary);return 0;
    }
    return 1;
}
