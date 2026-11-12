/* Optional nonresident Shell tool. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "control.h"
#include "locale_support.h"

/* Version string for the AmigaDOS Version command and the GUI installer
 * (getversion).  Non-static so the compiler keeps it. */
const char beam68ctl_version_tag[]="$VER: Beam68Ctl 0.1 (27.10.2026)";

static int is(const char *arg,const char *upper)
{
    while (*arg && *upper) {
        int c=(unsigned char)*arg++;
        if (c>='a' && c<='z') c-=32;
        if (c!=(unsigned char)*upper++) return 0;
    }
    return !*arg && !*upper;
}

static int ctl_main(int argc,char **argv);

int main(int argc,char **argv)
{
    int rc;
    beam_locale_open();      /* PROGDIR:Catalogs, like the launcher */
    rc=ctl_main(argc,argv);
    beam_locale_close();
    return rc;
}

/* STATUS prints the fixed words running/stopped for scripts; the return
 * code is the reliable interface. */
static int ctl_main(int argc,char **argv)
{
    ULONG action=0;
    int found,i;
    if (argc==2) {
        /* Scripts and installers: RC 0 = running, 5 = not running. */
        if (is(argv[1],"STATUS")) {
            found=beam_control_running();
            PutStr((STRPTR)(found?"running\n":"stopped\n"));
            return found?0:5;
        }
        /* Quit and wait until the launcher has really gone (it saves its
           history first).  RC 0 = stopped (or was not running), 10 = still
           running after 15 seconds. */
        if (is(argv[1],"QUITWAIT")) {
            if (beam_control_running()) beam_control_send(BEAM_CTL_QUIT);
            for (i=0;i<75 && beam_control_running();i++) Delay(10);
            return beam_control_running()?10:0;
        }
        if (is(argv[1],"RELOAD")) action=BEAM_CTL_RELOAD;
        else if (is(argv[1],"RELOADPLUGINS")) action=BEAM_CTL_RELOAD_PLUGINS;
        else if (is(argv[1],"REINDEX")) action=BEAM_CTL_REINDEX;
        else if (is(argv[1],"SHOW")) action=BEAM_CTL_SHOW;
        else if (is(argv[1],"HIDE") || is(argv[1],"STOP")) action=BEAM_CTL_HIDE;
        else if (is(argv[1],"QUIT")) action=BEAM_CTL_QUIT;
    }
    if (!action) {
        PutStr((STRPTR)beam_str(MSG_CTL_USAGE));
        return 10;
    }
    found=beam_control_send(action);
    if (!found) PutStr((STRPTR)beam_str(MSG_CTL_NOT_RUNNING));
    return found?0:5;
}
