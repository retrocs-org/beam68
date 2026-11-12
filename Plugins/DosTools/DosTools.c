#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/wb.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

struct Library *WorkbenchBase;

static void append(char *out,unsigned int cap,const char *text)
{
    unsigned int n=(unsigned int)strlen(out),i=0;
    if (n && n+2<cap) { out[n++]=',';out[n++]=' ';out[n]=0; }
    while (text[i] && n+1<cap) out[n++]=text[i++];
    out[n]=0;
}

static void bstr_name(BPTR bstr,char *out,unsigned int cap)
{
    UBYTE *b=(UBYTE *)BADDR(bstr);
    unsigned int i,n;
    if (!b || !cap) { if (cap) out[0]=0;return; }
    n=b[0];if (n+1>cap) n=cap-1;
    for (i=0;i<n;i++) out[i]=(char)b[i+1];
    out[n]=0;
}

static int list_dos(int argc,char **argv,int volumes)
{
    struct DosList *entry;
    ULONG flags=LDF_READ|(volumes?LDF_VOLUMES:LDF_ASSIGNS);
    char out[121],name[80];
    int count=0;
    out[0]=0;
    entry=LockDosList(flags);
    if (!entry) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_LOCK_FAILED));
    while ((entry=NextDosEntry(entry,flags))!=NULL) {
        bstr_name(entry->dol_Name,name,sizeof(name));
        if (*name) { count++;if (strlen(out)<90) append(out,sizeof(out),name); }
    }
    UnLockDosList(flags);
    if (!count) strcpy(out,volumes?plugin_str(PLMSG_DOS_NO_VOLUMES):plugin_str(PLMSG_DOS_NO_ASSIGNS));
    return beam_plugin_text(argc,argv,out);
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *s,*path;
    BPTR lock;
    struct InfoData *info;
    unsigned long free_kib,total_kib;
    struct TagItem tags[1];
    int ok;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (beam_ascii_equal(s,"volumes")) return list_dos(argc,argv,1);
    if (beam_ascii_equal(s,"assigns")) return list_dos(argc,argv,0);
    if (beam_ascii_prefix(s,"diskfree")) {
        path=s+8;if (*path==':') path++;path=beam_plugin_trim(path);if (!*path) path="SYS:";
        lock=Lock(path,ACCESS_READ);if (!lock) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_NOT_FOUND));
        info=(struct InfoData *)AllocVec(sizeof(*info),MEMF_ANY|MEMF_CLEAR);
        if (!info) { UnLock(lock);return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_NO_MEMORY)); }
        ok=Info(lock,info);UnLock(lock);
        if (!ok) { FreeVec(info);return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_INFO_FAILED)); }
        total_kib=(unsigned long)info->id_NumBlocks*(unsigned long)info->id_BytesPerBlock/1024UL;
        free_kib=((unsigned long)info->id_NumBlocks-(unsigned long)info->id_NumBlocksUsed)*
            (unsigned long)info->id_BytesPerBlock/1024UL;
        sprintf(out,plugin_str(PLMSG_DOS_FREE),path,
            path[strlen(path)-1]==':'?"":":",free_kib,total_kib);
        FreeVec(info);return beam_plugin_text(argc,argv,out);
    }
    if (beam_ascii_prefix(s,"open:")) {
        path=beam_plugin_trim(s+5);if (!*path || !strchr(path,':')) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_OPEN_USAGE));
        WorkbenchBase=OpenLibrary("workbench.library",39);
        if (!WorkbenchBase) return beam_plugin_error(argc,argv,"workbench.library is unavailable");
        tags[0].ti_Tag=TAG_END;tags[0].ti_Data=0;
        ok=OpenWorkbenchObjectA(path,tags);CloseLibrary(WorkbenchBase);WorkbenchBase=NULL;
        return ok?beam_plugin_text(argc,argv,plugin_str(PLMSG_DOS_OPENED)):beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_OPEN_FAILED));
    }
    return beam_plugin_error(argc,argv,plugin_str(PLMSG_DOS_USAGE));
}
