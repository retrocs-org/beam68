#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
static int writes=-1,renames=-1;
void qa_fail_writes(int n) { writes=n; }
void qa_fail_rename(int n) { renames=n; }
LONG qa_FPuts(BPTR file,CONST_STRPTR text)
{
    if (writes==0) { writes=-1; SetIoErr(ERROR_DISK_FULL); return -1; }
    if (writes>0) --writes;
    return FPuts(file,text);
}
LONG qa_Rename(CONST_STRPTR from,CONST_STRPTR to)
{
    if (renames==0) { renames=-1; SetIoErr(ERROR_DISK_FULL); return 0; }
    if (renames>0) --renames;
    return Rename(from,to);
}
