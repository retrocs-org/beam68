#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

struct IntuitionBase *IntuitionBase;

static void append(char *out,unsigned int cap,const char *title)
{
    unsigned int n=(unsigned int)strlen(out),i=0;
    if (!title || !*title || n>94) return;
    if (n && n+2<cap) { out[n++]=',';out[n++]=' '; }
    while (title[i] && n+1<cap) out[n++]=title[i++];
    out[n]=0;
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *s,*needle;
    struct Screen *screen,*found_screen=NULL;
    struct Window *window,*found_window=NULL;
    ULONG lock;
    int screen_only=0;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (beam_ascii_prefix(s,"window:")) s=beam_plugin_trim(s+7);
    else if (beam_ascii_prefix(s,"screen:")) { s=beam_plugin_trim(s+7);screen_only=1; }
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if (!IntuitionBase) return beam_plugin_error(argc,argv,"intuition.library is unavailable");
    lock=LockIBase(0);out[0]=0;
    if (!*s || beam_ascii_equal(s,"windows") || beam_ascii_equal(s,"list")) {
        for (screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
            for (window=screen->FirstWindow;window;window=window->NextWindow)
                append(out,sizeof(out),window->Title);
        UnlockIBase(lock);CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
        return beam_plugin_text(argc,argv,*out?out:plugin_str(PLMSG_WIN_NONE));
    }
    needle=s;
    for (screen=IntuitionBase->FirstScreen;screen && !found_window && !found_screen;screen=screen->NextScreen) {
        if (screen->Title && beam_ascii_contains(screen->Title,needle)) found_screen=screen;
        if (!screen_only) for (window=screen->FirstWindow;window;window=window->NextWindow)
            if (window->Title && beam_ascii_contains(window->Title,needle)) { found_window=window;break; }
    }
    UnlockIBase(lock);
    /* Intuition functions must not be called with IBase locked, but the
     * window may close in between.  Under Forbid() no other task (including
     * input.device, which runs Intuition) can close it, so re-validate the
     * pointer there.  WindowToFront()/ActivateWindow()/ScreenToFront() are
     * deferred actions and do not block. */
    Forbid();
    if (found_window) {
        int alive=0;
        for (screen=IntuitionBase->FirstScreen;screen && !alive;screen=screen->NextScreen)
            for (window=screen->FirstWindow;window;window=window->NextWindow)
                if (window==found_window) { alive=1; break; }
        if (alive) { WindowToFront(found_window);ActivateWindow(found_window); }
        else found_window=NULL;
    } else if (found_screen) {
        int alive=0;
        for (screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
            if (screen==found_screen) { alive=1; break; }
        if (alive) ScreenToFront(found_screen);
        else found_screen=NULL;
    }
    Permit();
    CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
    if (!found_window && !found_screen) return beam_plugin_error(argc,argv,plugin_str(PLMSG_WIN_NOT_FOUND));
    sprintf(out,plugin_str(PLMSG_WIN_FRONT),found_window?"window":"screen");
    return beam_plugin_text(argc,argv,out);
}
