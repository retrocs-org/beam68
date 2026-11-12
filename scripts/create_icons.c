/* Native OS3.2 installer helper -- uses system-default Workbench icons.
 * Generates a startup PROJECT icon pointing at Beam68 in Utilities, while
 * the plugin files stay in their own application directory.
 * No bundled copyright-protected system icon bitmaps are redistributed. */
#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

struct Library *IconBase;
#define APP "SYS:Utilities/Beam68/Beam68"
#define PREFS "SYS:Prefs/Beam68Prefs"
#define STARTUP "SYS:WBStartup/Beam68Start"

static int create_tool_icon(const char *name)
{
    struct DiskObject *d=GetDiskObject((STRPTR)name);
    int ok;
    if (d) { FreeDiskObject(d); return 1; } /* do not overwrite custom icons */
    d=GetDefDiskObject(WBTOOL);
    if (!d) return 0;
    d->do_StackSize=16384;
    ok=PutDiskObject((STRPTR)name,d)!=0;
    FreeDiskObject(d);
    return ok;
}
static int create_startup_project(void)
{
    struct DiskObject *d;
    BPTR f, lock;
    STRPTR original;
    int ok;
    lock=Lock((STRPTR)STARTUP,ACCESS_READ);
    if (lock) UnLock(lock);
    else {
        f=Open((STRPTR)STARTUP,MODE_NEWFILE);
        if (!f) return 0;
        Close(f);
    }
    d=GetDiskObject((STRPTR)STARTUP);
    if (d) { FreeDiskObject(d); return 1; } /* preserve modified Default Tool */
    d=GetDefDiskObject(WBPROJECT);
    if (!d) return 0;
    original=d->do_DefaultTool;
    d->do_DefaultTool=(STRPTR)APP;
    ok=PutDiskObject((STRPTR)STARTUP,d)!=0;
    d->do_DefaultTool=original;
    FreeDiskObject(d);
    return ok;
}
int main(void)
{
    int ok=0;
    IconBase=OpenLibrary("icon.library",39);
    if (!IconBase) goto cleanup;
    if (!create_tool_icon(APP)) goto cleanup;
    if (!create_tool_icon(PREFS)) goto cleanup;
    if (!create_startup_project()) goto cleanup;
    ok=1;
cleanup:
    if (IconBase) CloseLibrary(IconBase);
    return ok?0:20;
}
