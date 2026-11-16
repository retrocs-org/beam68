/* Optional nonresident Shell tool. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "control.h"
#include "beam.h"
#include "locale_support.h"

/* Version string for the AmigaDOS Version command and the GUI installer
 * (getversion).  Non-static so the compiler keeps it. */
const char beam68ctl_version_tag[]="$VER: Beam68Ctl " BEAM68_VERSION " (" BEAM68_VERSION_DATE ")";

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
    char version[64];
    if (argc==2 && is(argv[1],"INSTALLINFO")) {
        static struct BeamConfig config;
        char shortcut[192];
        DeleteVar((STRPTR)"Beam68-InstallerShortcut",GVF_GLOBAL_ONLY);
        beam_config_defaults(&config);
        if(!beam_config_read_active(&config) ||
           !beam_hotkey_display(config.hotkey,shortcut,sizeof(shortcut)))return 20;
        return SetVar((STRPTR)"Beam68-InstallerShortcut",(STRPTR)shortcut,-1,GVF_GLOBAL_ONLY)?0:20;
    }
    if (argc==2 && is(argv[1],"WAITREADY")) {
        /* Poll only while absent; do not impose a startup pause on fast CPUs. */
        for(i=0;i<350 && !beam_control_running();i++)Delay(1);
        return beam_control_version(version,sizeof(version))==1?0:20;
    }
    if (argc==2) {
        if(is(argv[1],"VERSION")) {
            found=beam_control_version(version,sizeof(version));
            if(found==1) {PutStr(version);PutStr("\n");return 0;}
            PutStr((STRPTR)beam_str(MSG_CTL_NOT_RUNNING));return 5;
        }
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
