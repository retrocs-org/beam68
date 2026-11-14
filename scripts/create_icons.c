/* Native OS3.2 installer helper -- uses system-default Workbench icons.
 * Generates a startup PROJECT icon pointing at the installed application.
 * No bundled copyright-protected system icon bitmaps are redistributed. */
#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <string.h>
#include <stdio.h>

struct Library *IconBase;
static const char *application="SYS:Utilities/Beam68/Beam68";
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
static int same_path(const char *a,const char *b)
{
    if(!a || !b)return 0;
    while(*a && *b){int x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}
    return !*a && !*b;
}
static int owned_startup_tool(const char *tool)
{
    return same_path(tool,"SYS:Utilities/Beam68") ||
        same_path(tool,"SYS:Utilities/Beam68/Beam68") ||
        same_path(tool,"SYS:Beam68/Beam68") || same_path(tool,application);
}
static int update_legacy_startup_script(void)
{
    BPTR f;char text[160],command[300];LONG n;
    f=Open((STRPTR)STARTUP,MODE_OLDFILE);if(!f)return 1;
    n=Read(f,text,sizeof(text)-1);Close(f);if(n<0)return 0;text[n]=0;
    text[strcspn(text,"\r\n")]=0;
    if(!same_path(text,"Run >NIL: \"SYS:Utilities/Beam68\""))return 1;
    f=Open((STRPTR)STARTUP,MODE_NEWFILE);if(!f)return 0;
    sprintf(command,"Run >NIL: \"%s\"\n",application);
    n=FPuts(f,command);Close(f);return n>=0;
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
    if(d && !owned_startup_tool(d->do_DefaultTool)) {
        FreeDiskObject(d);return 1; /* preserve a custom launcher */
    }
    if(!d)d=GetDefDiskObject(WBPROJECT);
    if (!d) return 0;
    original=d->do_DefaultTool;
    d->do_DefaultTool=(STRPTR)application;
    ok=PutDiskObject((STRPTR)STARTUP,d)!=0;
    d->do_DefaultTool=original;
    FreeDiskObject(d);
    return ok;
}
int main(int argc,char **argv)
{
    int ok=0;
    if(argc>1) {
        if(strlen(argv[1])>=240 || !strchr(argv[1],':') || strchr(argv[1],'"') || strchr(argv[1],'*') || strchr(argv[1],'\n') || strchr(argv[1],'\r'))return 20;
        application=argv[1];
    }
    IconBase=OpenLibrary("icon.library",39);
    if (!IconBase) goto cleanup;
    if (!create_tool_icon(application)) goto cleanup;
    if (!create_tool_icon(PREFS)) goto cleanup;
    if(!(argc>2 && same_path(argv[2],"NOSTARTUP"))) {
        if(!update_legacy_startup_script() || !create_startup_project())goto cleanup;
    }
    ok=1;
cleanup:
    if (IconBase) CloseLibrary(IconBase);
    return ok?0:20;
}
