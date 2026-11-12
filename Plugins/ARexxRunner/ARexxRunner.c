#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "dos_compat.h"
#include "plugin_locale.h"

#define MACRO_CAP 16
struct Macro { char name[25]; char path[201]; };

static int safe_name(const char *s)
{
    int n=0;
    while (*s) {
        char c=*s++;
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-')) return 0;
        if (++n>24) return 0;
    }
    return n>0;
}
static int safe_path(const char *s)
{
    return s && *s && strlen(s)<201 && strchr(s,':') && !strchr(s,'"') &&
           !strchr(s,'*') && !strchr(s,'\r') && !strchr(s,'\n');
}
static int load_macros(struct Macro *macros)
{
    static const char *files[]={"ENV:Beam68/Plugins/ARexxRunner.cfg",
        "ENVARC:Beam68/Plugins/ARexxRunner.cfg","PROGDIR:ARexxRunner.cfg",
        "PROGDIR:Plugins/ARexxRunner.cfg","Beam68:Plugins/ARexxRunner.cfg",NULL};
    BPTR f=0;char line[256];int i,count=0;
    for (i=0;files[i] && !f;i++) f=Open((STRPTR)files[i],MODE_OLDFILE);
    if (!f) return 0;
    while (count<MACRO_CAP && FGets(f,line,sizeof(line))) {
        char *s=beam_plugin_trim(line),*eq;
        if (!*s || *s=='#' || *s==';') continue;
        eq=strchr(s,'=');if (!eq) continue;*eq++=0;
        s=beam_plugin_trim(s);eq=beam_plugin_trim(eq);
        if (!safe_name(s) || !safe_path(eq)) continue;
        beam_plugin_copy(macros[count].name,s,sizeof(macros[count].name));
        beam_plugin_copy(macros[count].path,eq,sizeof(macros[count].path));
        count++;
    }
    Close(f);return count;
}
static int list_macros(int argc,char **argv,struct Macro *macros,int count)
{
    char out[BEAM_PLUGIN_MAX_RESULT+1];int i;
    if (!count) return beam_plugin_error(argc,argv,plugin_str(PLMSG_REXX_NONE));
    beam_plugin_copy(out,plugin_str(PLMSG_REXX_LIST),sizeof(out));
    for (i=0;i<count;i++) {
        if (strlen(out)+strlen(macros[i].name)+3>=sizeof(out)) break;
        if (i) strcat(out,", ");
        strcat(out,macros[i].name);
    }
    return beam_plugin_text(argc,argv,out);
}
int main(int argc,char **argv)
{
    static struct Macro macros[MACRO_CAP]; /* 3.6 KB: keep off the stack */
    char input[BEAM_PLUGIN_MAX_INPUT+1],command[240],temp[80],line[128],message[128];
    char *s;BPTR lock,f;LONG rc;int i,count;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);count=load_macros(macros);
    if (!*s || beam_ascii_equal(s,"list")) return list_macros(argc,argv,macros,count);
    if (!safe_name(s)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_REXX_NAME));
    for (i=0;i<count;i++) if (beam_ascii_equal(s,macros[i].name)) break;
    if (i==count) return beam_plugin_error(argc,argv,plugin_str(PLMSG_REXX_UNKNOWN));
    lock=Lock((STRPTR)macros[i].path,ACCESS_READ);
    if (!lock) return beam_plugin_error(argc,argv,plugin_str(PLMSG_REXX_MISSING));
    UnLock(lock);
    sprintf(temp,"T:Beam68-ARexx-%08lx",(unsigned long)FindTask(NULL));
    sprintf(command,"C:Rx \"%s\"",macros[i].path);
    /* SYS_Error is an OS4 (V50) tag that takes a handle, not a name; the
       portable wrapper opens NIL:/the temp file correctly on every 3.x. */
    rc=beam_system(command,NULL,temp,0);
    line[0]=0;f=Open((STRPTR)temp,MODE_OLDFILE);
    if (f) { FGets(f,line,sizeof(line));Close(f); } DeleteFile((STRPTR)temp);
    if (rc!=0) return beam_plugin_error(argc,argv,plugin_str(PLMSG_REXX_FAILED));
    s=beam_plugin_trim(line);
    if (*s) return beam_plugin_text(argc,argv,s);
    sprintf(message,plugin_str(PLMSG_REXX_DONE),macros[i].name);
    return beam_plugin_text(argc,argv,message);
}
