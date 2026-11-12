/* Beam68 - lightweight app launcher for AmigaOS 3.x / Motorola 68000.
 * Uses Intuition, Commodities, DOS, Workbench and icon.library only.
 * No MUI, Reaction, AHI, FPU or TCP stack required.
 *
 * OS support: 3.0 and later.  workbench.library V44+ (3.1.4, 3.5, 3.9, 3.2)
 * is used for launching when present; older systems get a built-in
 * WBStartup launcher.  dos.library V47 stream tags are used only on 3.2.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <devices/inputevent.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/notify.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/imageclass.h>
#include <graphics/gfxbase.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <libraries/commodities.h>
#include <libraries/keymap.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/rexxsyslib.h>
#include <proto/diskfont.h>
#include <proto/timer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "beam.h"
#include "startup_error.h"
#include "rexx_sequence.h"
#include "rexx_message.h"
#include "matcher.h"
#include "search_rank.h"
#include "calc_engine.h"
#include "history.h"
#include "query_history.h"
#include "favorites.h"
#include "aliases.h"
#include "index_cache.h"
#include "actions.h"
#include "plugin_protocol.h"
#include "plugin_job.h"
#include "control.h"
#include "launcher_ui.h"
#include "appearance.h"
#include "recovery.h"
#include "rexx_commands.h"
#include "plugin_diagnostics.h"
#include "plugin_health.h"
#include "scan_status.h"
#include "completion.h"
#include "search_scope.h"
#include "item_index.h"
#include "dos_compat.h"
#include "wb_launch.h"
#include "stack_guard.h"
#include "icon_info.h"
#include "whdload.h"
#include "clip.h"
#include "beam68plugin.h"
#include "hidden.h"
#include "sources.h"
#include "locale_support.h"

/* Version string for the AmigaDOS Version command and the GUI installer
 * (getversion).  Non-static so the compiler keeps it. */
const char beam68_version_tag[]="$VER: Beam68 0.1 (27.10.2026)";

#define HOTKEY_ID 1001L
#define BEAM_SCAN_MAX_CHUNK 256      /* upper bound per tick, any CPU */
#define BEAM_SCAN_BUDGET_MICROS 30000 /* work at most ~30ms of each 50ms tick */
#define BEAM_SCAN_FRAMES 17
#define BEAM_TICK_MICROS 50000
#define BEAM_NOTIFY_DEBOUNCE_TICKS 10
#define BEAM_SCAN_REDRAW_TICKS 10     /* refresh an open window ~2x/second */
/* The recovery marker only guards startup.  It is removed after this many
 * 50ms ticks of healthy running, so an ordinary power-off (the usual way an
 * Amiga is switched off) does not force safe mode on the next boot. */
#define BEAM_RECOVERY_GRACE_TICKS 160
#define BEAM_RECOVERY_MARKER "ENVARC:Beam68/Unclean.marker"
#define BEAM_STACK_MINIMUM 16384UL
#define RAW_UP 0x4c
#define RAW_DOWN 0x4d
#define RAW_ESCAPE 0x45
#define RAW_F1 0x50
#define RAW_F2 0x51
#define RAW_F3 0x52
#define RAW_F4 0x53
#define RAW_F5 0x54
#define RAW_F6 0x55
#define RAW_F7 0x56
#define RAW_F8 0x57
#define RAW_TAB 0x42
#define RAW_BACKSPACE 0x41
#define RAW_HELP 0x5f
#define RAW_LEFT 0x4f
#define RAW_RIGHT 0x4e
/* Mouse wheel as raw keys (NewMouse convention, also used by OS 3.2). */
#define RAW_WHEEL_UP 0x7a
#define RAW_WHEEL_DOWN 0x7b
#ifndef RAWKEY_PAGEUP
#define RAWKEY_PAGEUP 0x48
#define RAWKEY_PAGEDOWN 0x49
#define RAWKEY_HOME 0x70
#define RAWKEY_END 0x71
#endif

/* The names/types below match the public v39+ NDK prototypes. */
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CxBase;
struct Library *IconBase;
struct Library *WorkbenchBase;
/* Optional: missing rexxsyslib.library must never prevent Beam68 startup. */
struct RxsLib *RexxSysBase;
/* Optional: without diskfont.library only ROM/in-memory fonts can be used. */
struct Library *DiskfontBase;
struct Device *TimerBase;

/* The launcher keeps only the plugin fields it uses.  Settings-page
 * definitions are a Preferences concern (~1.6 KB per plugin). */
struct BeamLauncherPlugin {
    char name[BEAM_MAX_NAME];
    char command[BEAM_MAX_PATH];
    char description[96];
    char prefix[24];
    char category[32];
    char icon[BEAM_MAX_PATH];
    char example[96];
    char help[128];
    char capabilities[64];
    char trigger;
    unsigned char protocol;
    unsigned char preview;
};

static struct BeamConfig cfg;
static struct BeamFavorites favorites;
static struct BeamAliases aliases;
static struct BeamHistory usage; /* fixed size, no per-search allocations */
static struct BeamQueryHistory query_history;
static struct BeamPluginHealth plugin_health;
static struct BeamHidden hidden;
static struct BeamRecentDocs recent_docs;
static struct BeamIndex beam_index;
#define items (beam_index.items)
#define item_count (beam_index.count)
static struct BeamLauncherPlugin plugins[BEAM_MAX_PLUGINS];
static int plugin_count, plugin_load_errors, index_full, paths_truncated;
static int index_memory_reduced;
/* More results are kept than fit on screen; the list scrolls.  This lets
 * browsing, live lists and plugin rows go past the visible row count. */
#define BEAM_MAX_RESULTS 64   /* storage; RESULTS_MAX picks 8..64 */
static int results[BEAM_MAX_RESULTS], result_count, selected;
static int action_mode, result_scroll;
static struct MsgPort *cx_port;
static struct MsgPort *control_port;
static struct MsgPort *rexx_port;
static struct MsgPort *wb_reply_port; /* WBStartup replies (OS 3.0/3.1) */
static struct MsgPort *reply_port;    /* protocol-1 plugin replies */
static struct Beam68ReplyRow plugin_rows[BEAM68_MAX_ROWS];
static int plugin_row_count,rows_active;
static char rows_title[BEAM_MAX_NAME];
/* One small diagnostic entry per configured manifest, not per search. */
static UBYTE plugin_state[BEAM_MAX_PLUGINS];
static char plugin_trigger[BEAM_MAX_PLUGINS];
static char plugin_manifest[BEAM_MAX_PLUGINS][BEAM_MAX_PATH];
static CxObj *hotkey_filter;
#define QUICK_ID_BASE 2000L
static CxObj *quick_filters[9];
static void quick_filters_install(void);
static CxObj *broker;
static struct Screen *pubscreen;
static struct Window *window;
static struct DrawInfo *screen_drawinfo;
static struct BeamPalette palette;
static LONG theme_pens[BEAM_THEME_ROLE_COUNT];
static int theme_pen_count;
static struct TextFont *launcher_font;
static int compact_active;
/* Only ONE icon is cached while the search window is open. A preview is
 * opt-in, so floppy-based 68000 systems never incur selection-time I/O
 * unless the user enables it in Preferences or Beam68.cfg. */
static struct DiskObject *selected_icon;
static struct Rectangle selected_icon_rect;
static char selected_icon_path[BEAM_MAX_PATH];
static int selected_icon_usable;
static char query[BEAM_QUERY_LEN+1];
static int query_cursor, query_view_start;
static int restart_confirm;
static int last_width, last_height; /* size gadget changes last for this run */
static int last_clicked=-1;
static ULONG click_seconds, click_micros;
static char status[BEAM_PLUGIN_MAX_RESULT+32];
static int quitting;
static int search_pending;   /* typed-ahead keys: search once per batch */
/* Measurements for ARexx TIMING (microseconds), to decide on optimisations
 * with real numbers from real hardware. */
/* A single bounded outgoing ARexx job. Messages remain owned by the host
 * until replied: timed-out jobs block new requests until safely reaped. */
static struct {
    struct MsgPort *reply;
    struct RexxMsg *message;
    struct timeval started;
    int stage,timed_out;
    char port[80],drawer[BEAM_MAX_PATH],select[BEAM_MAX_PATH],handle[24];
} outgoing_rexx;
static unsigned long timing_search,timing_cache,timing_scan,timing_startup,timing_narrowed;
static struct timeval timing_scan_start;
static int recovery_grace_ticks;
static int scan_redraw_ticks;
static char index_cache_path[BEAM_MAX_PATH];

#define BEAM_WINDOW_STATE_CAP 8
#define BEAM_WINDOW_STATE_ENV "T:Beam68/WindowPositions.txt"
#define BEAM_WINDOW_STATE_ENVARC "ENVARC:Beam68/WindowPositions.txt"
struct BeamWindowState {
    char screen[64];
    int left,top,width,height;
};
static struct BeamWindowState window_states[BEAM_WINDOW_STATE_CAP];
static int window_state_count,window_states_dirty;

#define BEAM_HELP_CAP 64
#define BEAM_HELP_COMMAND 1
#define BEAM_HELP_PLUGIN 2
#define BEAM_HELP_ALIAS 3
#define BEAM_HELP_SHORTCUT 4
/* Help rows point at strings owned by the built-in table, the loaded
 * plugins or the alias catalog; help_build() runs after each reload. */
struct BeamHelpEntry {
    const char *title;
    const char *insert;
    const char *detail;
    unsigned char kind;
};
static struct BeamHelpEntry help_entries[BEAM_HELP_CAP];
static int help_count,help_mode;

/* An ExNext() iterator must retain both its lock and its own FileInfoBlock
 * across invocations, including while subdirectories are being examined. */
struct BeamScanFrame {
    BPTR lock;
    struct FileInfoBlock *fib;
    char path[BEAM_MAX_PATH];
    int depth;
    int root;
};
static struct BeamScanFrame scan_frames[BEAM_SCAN_FRAMES];
static int scan_active, scan_frames_used, scan_root, scan_count;
static unsigned long scan_roots;     /* roots (bit per DIR) being rescanned */
static unsigned long notify_pending_roots;
static int scan_full_rebuild;        /* volumes are part of this scan */
static unsigned long scan_unreadable, scan_io_errors;
static int scan_cached;
static struct MsgPort *notify_port;
static struct NotifyRequest notify_requests[BEAM_MAX_DIRS];
static unsigned char notify_active[BEAM_MAX_DIRS];
static int notify_refresh_ticks;
static struct BeamCompletions completions;

/* One timer only while background work exists: zero idle wakeups. */
static struct MsgPort *timer_port;
static struct timerequest *timer_io;
static int timer_open, timer_pending;
static struct BeamPluginJob plugin_job;
static char plugin_result_path[BEAM_MAX_PATH];
static char plugin_original_query[BEAM_QUERY_LEN+1];
/* Live previews (PREVIEW=YES): after a short typing pause the plugin runs
 * and its answer is shown under the query.  Previews never count towards
 * quarantine and are dropped as soon as the user presses Enter. */
#define BEAM_PREVIEW_TICKS 8   /* ~400 ms */
static int preview_ticks,preview_job,executing_preview;
static char preview_text[BEAM_PLUGIN_MAX_RESULT+1];
static char preview_for[BEAM_QUERY_LEN+1];
static char plugin_active_name[BEAM_MAX_NAME];
static char plugin_active_manifest[BEAM_MAX_PATH];
static char last_shell_command[BEAM_QUERY_LEN+1];
static char shell_notice[96];
static unsigned int plugin_serial;
static int safe_mode;
static int recovery_safe_mode,recovery_marker_owned;
static char plugin_last_result[BEAM_PLUGIN_MAX_RESULT+1];
static char plugin_last_name[BEAM_MAX_NAME];
static unsigned long plugin_last_ms;
static int plugin_confirm=-1;
static char plugin_confirm_query[BEAM_QUERY_LEN+1];
static void plugin_status(const char *message);

/* Window geometry is remembered per PUBLIC SCREEN NAME.  Screen->Title is
 * the current title-bar text, which changes with the active window (the
 * Workbench memory display, for one), so it is neither a stable key nor a
 * valid LockPubScreen() argument. */
static char pubscreen_key[MAXPUBSCREENNAME+1];

static int pubscreen_name(struct Screen *s,char *out,unsigned int cap)
{
    struct List *list;
    struct PubScreenNode *node;
    int found=0;
    if (!s || !out || !cap) return 0;
    list=LockPubScreenList();
    if (!list) return 0;
    for (node=(struct PubScreenNode *)list->lh_Head;
         node->psn_Node.ln_Succ;
         node=(struct PubScreenNode *)node->psn_Node.ln_Succ) {
        if (node->psn_Screen==s && node->psn_Node.ln_Name) {
            beam_copy(out,(const char *)node->psn_Node.ln_Name,cap);
            found=1;
            break;
        }
    }
    UnlockPubScreenList();
    return found;
}

static int window_state_find(const char *name)
{
    int i;for(i=0;i<window_state_count;i++)
        if(beam_equal(window_states[i].screen,name))return i;
    return -1;
}

static void window_states_load_file(const char *path)
{
    BPTR f=Open((STRPTR)path,MODE_OLDFILE);char line[192];
    if(!f)return;
    while(window_state_count<BEAM_WINDOW_STATE_CAP && FGets(f,line,sizeof(line))) {
        char *a,*b,*c,*d;struct BeamWindowState *s;
        a=strchr(line,'\t');if(!a)continue;*a++=0;
        b=strchr(a,'\t');if(!b)continue;*b++=0;
        c=strchr(b,'\t');if(!c)continue;*c++=0;
        d=strchr(c,'\t');if(!d)continue;*d++=0;
        s=&window_states[window_state_count];memset(s,0,sizeof(*s));
        beam_copy(s->screen,beam_trim(line),sizeof(s->screen));
        s->left=atoi(a);s->top=atoi(b);s->width=atoi(c);s->height=atoi(d);
        if(s->screen[0] && s->width>=230 && s->height>=90)++window_state_count;
    }
    Close(f);
}

static void window_states_load(void)
{
    window_state_count=0;window_states_load_file(BEAM_WINDOW_STATE_ENV);
    if(!window_state_count)window_states_load_file(BEAM_WINDOW_STATE_ENVARC);
}

static int window_states_save(const char *path)
{
    BPTR f,dir;char temp[BEAM_MAX_PATH],line[192];int i,ok=1;
    dir=CreateDir((STRPTR)(beam_equal(path,BEAM_WINDOW_STATE_ENV)?"T:Beam68":"ENVARC:Beam68"));
    if(dir)UnLock(dir);
    if(strlen(path)+5>=sizeof(temp))return 0;
    sprintf(temp,"%s.new",path);
    f=Open((STRPTR)temp,MODE_NEWFILE);if(!f)return 0;
    for(i=0;i<window_state_count;i++) {
        sprintf(line,"%.63s\t%ld\t%ld\t%ld\t%ld\n",window_states[i].screen,
            (long)window_states[i].left,(long)window_states[i].top,
            (long)window_states[i].width,(long)window_states[i].height);
        if(FPuts(f,(STRPTR)line)<0){ok=0;break;}
    }
    if(Close(f)==0)ok=0;
    if(!ok){DeleteFile((STRPTR)temp);return 0;}
    DeleteFile((STRPTR)path);
    if(!Rename((STRPTR)temp,(STRPTR)path)){DeleteFile((STRPTR)temp);return 0;}
    return 1;
}

static void window_state_capture(void)
{
    int i;if(!window || !pubscreen)return;
    i=window_state_find(pubscreen_key);
    if(i<0){
        if(window_state_count>=BEAM_WINDOW_STATE_CAP)return;
        i=window_state_count++;
        memset(&window_states[i],0,sizeof(window_states[i]));
        beam_copy(window_states[i].screen,pubscreen_key,sizeof(window_states[i].screen));
    }
    window_states[i].left=window->LeftEdge;window_states[i].top=window->TopEdge;
    window_states[i].width=window->Width;window_states[i].height=window->Height;
    window_states_dirty=1;window_states_save(BEAM_WINDOW_STATE_ENV);
}

static int launcher_lineheight(struct RastPort *rp)
{
    int spacing=compact_active?3:cfg.row_spacing;
    if (spacing<2) spacing=2;
    return (int)rp->TxHeight+spacing;
}

static void help_add(int kind,const char *title,const char *insert,
                     const char *detail)
{
    struct BeamHelpEntry *e;
    if (help_count>=BEAM_HELP_CAP || !title || !*title) return;
    e=&help_entries[help_count++];
    e->kind=(unsigned char)kind;
    e->title=title;
    e->insert=insert?insert:"";
    e->detail=detail?detail:"";
}

static void help_build(void)
{
    int i;
    help_count=0;
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_APP_TITLE),"app:",beam_str(MSG_HELP_APP));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_FILE_TITLE),"file:",beam_str(MSG_HELP_FILE));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_DRAWER_TITLE),"drawer:",beam_str(MSG_HELP_DRAWER));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_PLUGIN_TITLE),"plugin:",beam_str(MSG_HELP_PLUGIN));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_RECENT_TITLE),"recent:",beam_str(MSG_HELP_RECENT));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_VOLUME_TITLE),"volume:",beam_str(MSG_HELP_VOLUME));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_FAV_TITLE),"fav:",beam_str(MSG_HELP_FAV));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_GAME_TITLE),"game:",beam_str(MSG_HELP_GAME));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_RANDOM_TITLE),"game:random",beam_str(MSG_HELP_RANDOM));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_DOC_TITLE),"doc:",beam_str(MSG_HELP_DOC));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_MOD_TITLE),"mod:",beam_str(MSG_HELP_MOD));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_WEB_TITLE),"web:",beam_str(MSG_HELP_WEB));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_SNIP_TITLE),"snip:",beam_str(MSG_HELP_SNIP));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_SCRIPT_TITLE),"script:",beam_str(MSG_HELP_SCRIPT));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_PREFS_TITLE),"prefs:",beam_str(MSG_HELP_PREFS));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_STATS_TITLE),"stats:",beam_str(MSG_HELP_STATS));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_HIDDEN_TITLE),"hidden:",beam_str(MSG_HELP_HIDDEN));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_SHELL_TITLE),"shell:",beam_str(MSG_HELP_SHELL));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_RESTART_WB_TITLE),"restart workbench",beam_str(MSG_HELP_SECOND_ENTER));
    help_add(BEAM_HELP_COMMAND,beam_str(MSG_HELP_RESTART_AMIGA_TITLE),"restart amiga",beam_str(MSG_HELP_COLD_REBOOT));
    help_add(BEAM_HELP_SHORTCUT,"Up/Down","",beam_str(MSG_KEY_UPDOWN));
    help_add(BEAM_HELP_SHORTCUT,"Tab","",beam_str(MSG_KEY_TAB));
    help_add(BEAM_HELP_SHORTCUT,"F2","",beam_str(MSG_KEY_F2));
    help_add(BEAM_HELP_SHORTCUT,"F3 / Shift-F3","",beam_str(MSG_KEY_F3));
    help_add(BEAM_HELP_SHORTCUT,"F4 / F5 / F6","",beam_str(MSG_KEY_F4));
    help_add(BEAM_HELP_SHORTCUT,"F7 / F8","",beam_str(MSG_KEY_F7));
    for (i=0;i<plugin_count;i++)
        help_add(BEAM_HELP_PLUGIN,plugins[i].name,
            plugins[i].example[0]?plugins[i].example:plugins[i].prefix,
            plugins[i].help[0]?plugins[i].help:plugins[i].description);
    for (i=0;i<(int)aliases.count;i++)
        help_add(BEAM_HELP_ALIAS,aliases.entries[i].words,
                 aliases.entries[i].words,aliases.entries[i].path);
}

static int recovery_marker_exists(void)
{
    BPTR lock=Lock((STRPTR)BEAM_RECOVERY_MARKER,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);return 1;
}

static int recovery_marker_begin(void)
{
    BPTR dir,f;
    dir=CreateDir((STRPTR)"ENVARC:Beam68");if (dir) UnLock(dir);
    f=Open((STRPTR)BEAM_RECOVERY_MARKER,MODE_NEWFILE);
    if (!f) return 0;
    recovery_marker_owned=1;
    FPuts(f,(STRPTR)"Beam68 startup in progress; removed once running\n");
    if (!Close(f)) return 0;
    return 1;
}

/* Publish recovery/safe-mode state for Preferences.  The persistent ENVARC:
   marker cannot be used for this because it deliberately exists throughout
   every healthy launcher run. */
static void safe_mode_active_set(int active)
{
    BPTR dir,f;
    if (!active) {
        DeleteFile((STRPTR)BEAM_SAFE_MODE_ACTIVE);
        return;
    }
    dir=CreateDir((STRPTR)"T:Beam68");if (dir) UnLock(dir);
    f=Open((STRPTR)BEAM_SAFE_MODE_ACTIVE,MODE_NEWFILE);
    if (!f) return;
    FPuts(f,(STRPTR)"Beam68 0.1 safe mode active\n");
    Close(f);
}

static int allocate_index(struct BeamIndex *x,int requested,int *actual)
{
    int capacity=requested;
    while (capacity>=16) {
        if (beam_index_init(x,capacity)) {
            if (actual) *actual=capacity;
            return 1;
        }
        capacity=beam_index_retry_capacity(capacity);
    }
    if (actual) *actual=0;
    return 0;
}

static int root_bits(unsigned long roots);
static void scan_snapshot(struct BeamScanStatus *s)
{
    unsigned long done=scan_root>=32?0xffffffffUL:((1UL<<scan_root)-1UL);
    s->roots_done=(unsigned long)root_bits(scan_roots&done);
    s->roots_total=(unsigned long)root_bits(scan_roots);
    s->entries=(unsigned long)scan_count;
    s->applications=(unsigned long)item_count;
    s->unreadable=scan_unreadable;
    s->io_errors=scan_io_errors;
    s->overlong=(unsigned long)paths_truncated;
    s->scanning=scan_active;
    s->capped=index_full;
    s->cached=scan_cached;
}

/* PROGDIR: is process-local and can be unavailable to DOS file calls when
 * Beam68 is started from some Startup-Sequence/Run combinations. Resolve it
 * explicitly from this process' program directory before opening manifests
 * or handing executable paths to a child Shell. */
static int beam_runtime_path(const char *spec,char *out,unsigned int cap)
{
    BPTR dir;
    int progdir;
    if (!spec || !out || cap==0 || strlen(spec)+1>cap) return 0;
    progdir=(strlen(spec)>=8 &&
        (spec[0]=='P'||spec[0]=='p') && (spec[1]=='R'||spec[1]=='r') &&
        (spec[2]=='O'||spec[2]=='o') && (spec[3]=='G'||spec[3]=='g') &&
        (spec[4]=='D'||spec[4]=='d') && (spec[5]=='I'||spec[5]=='i') &&
        (spec[6]=='R'||spec[6]=='r') && spec[7]==':');
    if (!progdir) { beam_copy(out,spec,cap); return 1; }
    dir=GetProgramDir();
    if (!dir || !NameFromLock(dir,out,(LONG)cap)) return 0;
    return AddPart(out,(STRPTR)(spec+8),(LONG)cap)!=0;
}

static void arm_timer(void)
{
    if (!timer_io || timer_pending ||
        (!scan_active && !plugin_job.active &&
         !(outgoing_rexx.message && !outgoing_rexx.timed_out) && !notify_refresh_ticks &&
         !recovery_grace_ticks && !preview_ticks)) return;
    timer_io->tr_node.io_Command=TR_ADDREQUEST;
    timer_io->tr_time.tv_secs=0;
    timer_io->tr_time.tv_micro=BEAM_TICK_MICROS;
    SendIO((struct IORequest *)timer_io);
    timer_pending=1;
}

static void notify_stop(void)
{
    int i;struct Message *m;
    if (notify_port) while ((m=GetMsg(notify_port))) ReplyMsg(m);
    for (i=0;i<BEAM_MAX_DIRS;i++) if (notify_active[i]) {
        EndNotify(&notify_requests[i]);notify_active[i]=0;
    }
    if (notify_port) {
        while ((m=GetMsg(notify_port))) ReplyMsg(m);
        DeleteMsgPort(notify_port);notify_port=NULL;
    }
    notify_refresh_ticks=0;
    notify_pending_roots=0;
}

static void notify_start(void)
{
    int i;
    notify_stop();
    notify_port=CreateMsgPort();if (!notify_port) return;
    memset(notify_requests,0,sizeof(notify_requests));
    for (i=0;i<cfg.dir_count;i++) {
        notify_requests[i].nr_Name=(STRPTR)cfg.dirs[i];
        notify_requests[i].nr_UserData=(ULONG)i; /* which root changed */
        notify_requests[i].nr_Flags=NRF_SEND_MESSAGE;
        notify_requests[i].nr_stuff.nr_Msg.nr_Port=notify_port;
        if (StartNotify(&notify_requests[i])) notify_active[i]=1;
    }
}

static void notify_drain(void)
{
    struct NotifyMessage *m;int changed=0;
    if (!notify_port) return;
    while ((m=(struct NotifyMessage *)GetMsg(notify_port))) {
        if (m->nm_NReq && m->nm_NReq->nr_UserData<32UL)
            notify_pending_roots|=1UL<<m->nm_NReq->nr_UserData;
        ReplyMsg((struct Message *)m);changed=1;
    }
    if (changed) {
        notify_refresh_ticks=BEAM_NOTIFY_DEBOUNCE_TICKS;
        arm_timer();
    }
}

static int init_timer(void)
{
    timer_port=CreateMsgPort();
    if (!timer_port) return 0;
    timer_io=(struct timerequest *)CreateIORequest(timer_port,sizeof(struct timerequest));
    if (!timer_io) return 0;
    if (OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)timer_io,0)!=0) return 0;
    timer_open=1;
    TimerBase=timer_io->tr_node.io_Device; /* for GetSysTime() scan budgeting */
    return 1;
}

static void cleanup_timer(void)
{
    if (timer_io && timer_open) {
        if (timer_pending) {
            if (!CheckIO((struct IORequest *)timer_io)) AbortIO((struct IORequest *)timer_io);
            WaitIO((struct IORequest *)timer_io);
            timer_pending=0;
        }
        CloseDevice((struct IORequest *)timer_io);
    }
    if (timer_io) { DeleteIORequest((struct IORequest *)timer_io); timer_io=NULL; }
    if (timer_port) { DeleteMsgPort(timer_port); timer_port=NULL; }
    timer_open=0;
}


static void search_items(void);
static void render(void);

static void preview_clear(void)
{
    if (selected_icon) FreeDiskObject(selected_icon);
    selected_icon=NULL;
    selected_icon_path[0]=0;
    selected_icon_usable=0;
}

static void preview_prepare_path(const char *path)
{
    struct TagItem tags[3];
    int width,height;
    if (!path || !*path || !cfg.icon_preview || !window || IconBase->lib_Version<44) {
        preview_clear();return;
    }
    if (!strcmp(selected_icon_path,path)) return;
    preview_clear();
    beam_copy(selected_icon_path,path,sizeof(selected_icon_path));
    selected_icon=GetDiskObject((STRPTR)path);
    if (!selected_icon) return;
    tags[0].ti_Tag=ICONDRAWA_DrawInfo;
    tags[0].ti_Data=(ULONG)screen_drawinfo;
    tags[1].ti_Tag=ICONDRAWA_Borderless;
    tags[1].ti_Data=TRUE;
    tags[2].ti_Tag=TAG_DONE;
    tags[2].ti_Data=0;
    if (!GetIconRectangleA(window->RPort,selected_icon,NULL,
                           &selected_icon_rect,tags)) return;
    width=(int)selected_icon_rect.MaxX-(int)selected_icon_rect.MinX+1;
    height=(int)selected_icon_rect.MaxY-(int)selected_icon_rect.MinY+1;
    selected_icon_usable=beam_preview_fits(width,height);
}
static void preview_prepare(const struct BeamItem *item)
{
    if (!item || item->kind!=BEAM_ITEM_WORKBENCH) { preview_clear();return; }
    preview_prepare_path(item->path);
}

static void preview_draw(struct RastPort *rp, int x, int y)
{
    struct TagItem tags[3];
    int width,height;
    if (!selected_icon || !selected_icon_usable) return;
    width=(int)selected_icon_rect.MaxX-(int)selected_icon_rect.MinX+1;
    height=(int)selected_icon_rect.MaxY-(int)selected_icon_rect.MinY+1;
    tags[0].ti_Tag=ICONDRAWA_DrawInfo;
    tags[0].ti_Data=(ULONG)screen_drawinfo;
    tags[1].ti_Tag=ICONDRAWA_Borderless;
    tags[1].ti_Data=TRUE;
    tags[2].ti_Tag=TAG_DONE;
    tags[2].ti_Data=0;
    /* Account for negative rectangle origins to center without clipping. */
    DrawIconStateA(rp,selected_icon,NULL,
                   x+(BEAM_PREVIEW_BOX-width)/2-selected_icon_rect.MinX,
                   y+(BEAM_PREVIEW_BOX-height)/2-selected_icon_rect.MinY,
                   IDS_NORMAL,tags);
}


static int has_suffix_ci(const char *s, const char *suffix)
{
    unsigned int n=(unsigned int)strlen(s), m=(unsigned int)strlen(suffix);
    return n>=m && beam_equal(s+n-m,suffix);
}

/* Ranking metadata is computed once per item (on add, launch, pin and
 * reload) so the per-keystroke search performs no favorites/history/alias
 * string comparisons at all. */
static unsigned long index_generation;
static void item_bind_meta(struct BeamItem *it)
{
    ++index_generation;
    int pos;
    unsigned int bonus;
    const char *words;
    it->alias_index=(signed char)beam_aliases_find(&aliases,it->path);
    pos=beam_favorites_position(&favorites,it->path);
    it->favorite=(unsigned char)((pos>=0 && pos<BEAM_ITEM_META_NONE)?pos:BEAM_ITEM_META_NONE);
    pos=cfg.history?beam_history_recent_position(&usage,it->path):-1;
    it->recent=(unsigned char)((pos>=0 && pos<BEAM_ITEM_META_NONE)?pos:BEAM_ITEM_META_NONE);
    bonus=cfg.history?beam_history_bonus(&usage,it->path):0;
    it->bonus=(unsigned char)(bonus>144?144:bonus);
    if (beam_hidden_contains(&hidden,it->path)) it->flags|=BEAM_ITEM_FLAG_HIDDEN;
    else it->flags&=(unsigned char)~BEAM_ITEM_FLAG_HIDDEN;
    words=beam_aliases_words(&aliases,(int)it->alias_index);
    it->mask=beam_text_mask(it->name)|(words?beam_text_mask(words):0UL)|
             (it->title?beam_text_mask(it->title):0UL);
}

static void bind_all_meta(void)
{
    int i;
    for (i=0;i<item_count;i++) item_bind_meta(&items[i]);
}

/* Returns the index of a newly created item, otherwise -1. */
static int index_add(const char *path,int kind,int root)
{
    int index,created;
    if (!*path) return -1;
    /* Shell parsing of quoted strings uses an asterisk escape on AmigaDOS.
       Skip files with unsafe shell metacharacters in their path. */
    if (strchr(path,'"') || strchr(path,'*') || strchr(path,'\n')) return -1;
    index=beam_index_add(&beam_index,path,kind,root,&created);
    if (index==-1) { index_full=1; return -1; }
    if (index<0) { ++paths_truncated; return -1; }
    if (!created) return -1;
    item_bind_meta(&items[index]);
    return index;
}

static int is_native_executable(const char *path)
{
    /* HUNK_HEADER (0x000003F3). This intentionally skips scripts/data. */
    UBYTE hdr[4];
    BPTR f=Open((STRPTR)path,MODE_OLDFILE);
    LONG got;
    if (!f) return 0;
    got=Read(f,hdr,4);
    Close(f);
    return got==4 && hdr[0]==0 && hdr[1]==0 && hdr[2]==3 && hdr[3]==0xf3;
}

/* Classify an icon from its file instead of GetDiskObject(), which loads
 * and decodes the whole icon (including NewIcon/ColorIcon imagery).  Tools
 * need only the 50-byte header; projects are read a little further, up to
 * the Default Tool, to recognise WHDLoad games.  Returns BEAM_ITEM_WORKBENCH,
 * BEAM_ITEM_GAME, or 0 for icons that are not launchable. */
/* One 4 KB buffer serves the icon reader, the slave reader and the game
 * path lookup: they never run at the same time. */
static UBYTE io_buf[4096];

/* WHDLoad: the SLAVE tool type of the last game icon classified. */
static char icon_slave[64];

static int icon_file_kind(const char *info_path)
{
    UBYTE *buf=io_buf;
    char tool[64];
    BPTR f=Open((STRPTR)info_path,MODE_OLDFILE);
    LONG got,more;
    int type;
    if (!f) return 0;
    got=Read(f,buf,50);
    if (got<49 || buf[0]!=(UBYTE)(WB_DISKMAGIC>>8) ||
        buf[1]!=(UBYTE)(WB_DISKMAGIC&255)) { Close(f); return 0; }
    type=(int)buf[48];
    icon_slave[0]=0;
    if (type==WBPROJECT && cfg.index_games) {
        more=Read(f,buf+got,(LONG)sizeof(io_buf)-got);
        if (more>0) got+=more;
        Close(f);
        beam_icon_info(buf,(unsigned long)got,tool,sizeof(tool));
        if (!beam_icon_is_whdload_tool(tool)) return BEAM_ITEM_WORKBENCH;
        beam_icon_tooltype(buf,(unsigned long)got,"SLAVE",icon_slave,sizeof(icon_slave));
        return BEAM_ITEM_GAME;
    }
    Close(f);
    return (type==WBTOOL || type==WBPROJECT)?BEAM_ITEM_WORKBENCH:0;
}

static int read_slave(const char *slave_path,struct BeamSlaveInfo *info)
{
    UBYTE *buf=io_buf;
    BPTR f=Open((STRPTR)slave_path,MODE_OLDFILE);
    LONG got;
    if (!f) return 0;
    got=Read(f,buf,sizeof(io_buf));
    Close(f);
    return got>0 && beam_slave_info(buf,(unsigned long)got,info);
}

/* Slave of a game item: the item itself, or the icon's SLAVE tool type
 * relative to the icon's drawer. */
static int game_slave_path(const char *path,char *out,unsigned int cap)
{
    UBYTE *buf=io_buf;
    char info[BEAM_MAX_PATH+8],slave[64];
    BPTR f;
    LONG got;
    if (has_suffix_ci(path,".slave")) { beam_copy(out,path,cap); return strlen(path)<cap; }
    if (strlen(path)+6>sizeof(info)) return 0;
    sprintf(info,"%s.info",path);
    f=Open((STRPTR)info,MODE_OLDFILE);
    if (!f) return 0;
    got=Read(f,buf,sizeof(io_buf));
    Close(f);
    if (got<=0 || !beam_icon_tooltype(buf,(unsigned long)got,"SLAVE",slave,sizeof(slave)) ||
        !beam_parent_drawer(path,out,cap)) return 0;
    return AddPart((STRPTR)out,(STRPTR)slave,(LONG)cap)!=0;
}

/* Index the game's real title (from the slave) as display and search text. */
static void game_set_title(int index,const char *slave_path)
{
    struct BeamSlaveInfo s;
    char title[128];
    if (index<0 || !read_slave(slave_path,&s)) return;
    beam_slave_title(&s,title,sizeof(title));
    if (title[0] && beam_index_set_title(&beam_index,index,title))
        item_bind_meta(&items[index]);
}

/* An icon-less WHDLoad slave; skipped when the game already has an icon
 * of the same name, which is how WHDLoad installs normally launch. */
static int slave_without_icon(const char *slave_path)
{
    char info[BEAM_MAX_PATH+8];
    unsigned int n=(unsigned int)strlen(slave_path);
    BPTR lock;
    if (n<7 || n+5>=sizeof(info)) return 0;
    memcpy(info,slave_path,n-6); strcpy(info+n-6,".info");
    lock=Lock((STRPTR)info,ACCESS_READ);
    if (lock) { UnLock(lock); return 0; }
    return 1;
}

/* Directory processing is cooperative, not a second shared-memory task.
 * Every timer reply does as much as fits in a ~30ms budget (bounded by
 * BEAM_SCAN_MAX_CHUNK) and yields at once when keyboard or hotkey input is
 * pending.  Filesystem calls themselves can still block (on a floppy). */
static void scan_pop(void)
{
    struct BeamScanFrame *f;
    if (!scan_frames_used) return;
    f=&scan_frames[--scan_frames_used];
    if (f->fib) FreeDosObject(DOS_FIB,f->fib);
    if (f->lock) UnLock(f->lock);
    f->fib=NULL; f->lock=0;
}
/* Stop an unfinished scan.  Items not re-visited are kept (their stale mark
 * is simply cleared): an incomplete walk must never delete valid entries. */
static void scan_cancel(void)
{
    int i;
    while (scan_frames_used) scan_pop();
    if (scan_active)
        for (i=0;i<item_count;i++) items[i].flags&=(unsigned char)~BEAM_ITEM_FLAG_STALE;
    scan_active=0;
}
static int scan_push(const char *path,int depth,int root)
{
    struct BeamScanFrame *f;
    if (scan_frames_used==BEAM_SCAN_FRAMES) return 0;
    f=&scan_frames[scan_frames_used];
    f->lock=Lock((STRPTR)path,ACCESS_READ);
    if (!f->lock) return 0;
    f->fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (!f->fib) { UnLock(f->lock); f->lock=0; return 0; }
    if (!Examine(f->lock,f->fib)) {
        FreeDosObject(DOS_FIB,f->fib);
        UnLock(f->lock);
        f->fib=NULL;f->lock=0;
        return 0;
    }
    beam_copy(f->path,path,sizeof(f->path));
    f->depth=depth;
    f->root=root;
    ++scan_frames_used;
    return 1;
}
static unsigned long all_roots(void)
{
    return cfg.dir_count>=32?0xffffffffUL:((1UL<<cfg.dir_count)-1UL);
}
static int root_bits(unsigned long roots)
{
    int n=0;
    while (roots) { n+=(int)(roots&1UL); roots>>=1; }
    return n;
}
static int scan_stale_marked;
static void scan_complete(void);
/* Rescan the given roots.  Existing entries stay searchable throughout:
 * they are marked stale, refreshed when found again, and only the ones that
 * really vanished are removed once the walk completes. */
static void scan_begin(unsigned long roots)
{
    int i;
    scan_cancel();
    preview_clear();
    roots&=all_roots();
    scan_full_rebuild=(roots==all_roots());
    index_full=0; paths_truncated=0;
    beam_index_mark_stale(&beam_index,
        roots|(scan_full_rebuild?(1UL<<BEAM_ROOT_SOURCES):0UL),scan_full_rebuild);
    scan_stale_marked=0;
    for (i=0;i<item_count;i++) if (items[i].flags&BEAM_ITEM_FLAG_STALE) ++scan_stale_marked;
    if (scan_full_rebuild) {
        /* Volume scope is intentionally bounded to configured roots plus the
           standard RAM: volume; it performs no DosList traversal per query. */
        index_add("RAM:",BEAM_ITEM_VOLUME,BEAM_ROOT_NONE);
        for (i=0;i<cfg.dir_count;i++) {
            char volume[BEAM_MAX_NAME];const char *colon=strchr(cfg.dirs[i],':');
            unsigned int n=colon?(unsigned int)(colon-cfg.dirs[i]+1):0;
            if (n && n<sizeof(volume)) {
                memcpy(volume,cfg.dirs[i],n);volume[n]=0;
                index_add(volume,BEAM_ITEM_VOLUME,BEAM_ROOT_NONE);
            }
        }
    }
    scan_roots=roots;
    scan_root=0; scan_count=0; scan_unreadable=0; scan_io_errors=0; scan_cached=0;
    scan_redraw_ticks=0;
    if (TimerBase) GetSysTime(&timing_scan_start);
    scan_active=1;
    if (!roots) scan_complete(); /* nothing to walk: finish (and sweep) now */
    else arm_timer();
}

static long micros_since(const struct timeval *start)
{
    struct timeval now;
    GetSysTime(&now);
    return (long)(now.tv_secs-start->tv_secs)*1000000L+
           ((long)now.tv_micro-(long)start->tv_micro);
}

/* Index sources outside the drawer walk (only on full rebuilds): ARexx
 * scripts in REXX:, bookmarks from HTML hotlists, and text snippets. */
static void index_sources(void)
{
    static char line[512],url[BEAM_MAX_PATH],title[BEAM_MAX_PATH],path[BEAM_MAX_PATH];
    char name[48];
    BPTR f;
    int index;
    if (cfg.index_rexx) {
        BPTR lock=Lock((STRPTR)"REXX:",ACCESS_READ);
        struct FileInfoBlock *fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
        if (lock && fib && Examine(lock,fib)) {
            while (ExNext(lock,fib)) {
                const char *n=(const char *)fib->fib_FileName;
                if (fib->fib_DirEntryType>=0 || !has_suffix_ci(n,".rexx") ||
                    strlen(n)+6>=sizeof(path)) continue;
                sprintf(path,"REXX:%s",n);
                index_add(path,BEAM_ITEM_SCRIPT,BEAM_ROOT_SOURCES);
            }
        }
        if (fib) FreeDosObject(DOS_FIB,fib);
        if (lock) UnLock(lock);
    }
    if (cfg.bookmark_files[0]) {
        char files[BEAM_MAX_PATH],*one,*next;
        beam_copy(files,cfg.bookmark_files,sizeof(files));
        for (one=files;one && *one;one=next) {
            next=strchr(one,';');
            if (next) *next++=0;
            one=beam_trim(one);
            if (!*one || !(f=Open((STRPTR)one,MODE_OLDFILE))) continue;
            {
                /* Any hotlist layout: several anchors per line, anchors
                   spanning lines (carried over, bounded) and bare URLs. */
                static char carry[1024];
                unsigned int kept=0;
                while (FGets(f,line,sizeof(line))) {
                    const char *p,*next;
                    int partial=0;
                    unsigned int n=(unsigned int)strlen(line);
                    if (kept+n+1>sizeof(carry)) kept=0;   /* runaway anchor: drop */
                    memcpy(carry+kept,line,n+1); kept+=n;
                    for (p=carry;(next=beam_bookmark_next(p,url,sizeof(url),title,sizeof(title),&partial));p=next) {
                        if (strlen(beam_index_name_for(url))>=BEAM_MAX_NAME) continue;
                        index=index_add(url,BEAM_ITEM_BOOKMARK,BEAM_ROOT_SOURCES);
                        if (index>=0 && beam_index_set_title(&beam_index,index,title))
                            item_bind_meta(&items[index]);
                    }
                    if (partial) {
                        const char *open=p,*q;
                        for (q=p;*q;q++) if ((q[0]=='<') && (q[1]=='a' || q[1]=='A') && q[2]==' ') open=q;
                        kept=(unsigned int)strlen(open);
                        memmove(carry,open,kept+1);
                    } else kept=0;
                }
            }
            Close(f);
        }
    }
    if ((f=Open((STRPTR)BEAM_SNIPPETS_PATH,MODE_OLDFILE))) {
        while (FGets(f,line,sizeof(line)))
            if (beam_snippet_parse(line,name,sizeof(name),title,sizeof(title))) {
                sprintf(path,"SNIPPET:%s",name);
                index=index_add(path,BEAM_ITEM_SNIPPET,BEAM_ROOT_SOURCES);
                if (index>=0 && beam_index_set_title(&beam_index,index,title))
                    item_bind_meta(&items[index]);
            }
        Close(f);
    }
}

static void scan_complete(void)
{
    struct BeamScanStatus completed;
    while (scan_frames_used) scan_pop();
    scan_active=0;
    if (TimerBase) timing_scan=(unsigned long)micros_since(&timing_scan_start);
    if (scan_full_rebuild) index_sources();
    /* The walk finished: whatever is still stale has really disappeared.
       Sweeping renumbers items, so any shown result list must be rebuilt. */
    beam_index_sweep(&beam_index);
    ++index_generation;
    search_pending=1;
    scan_snapshot(&completed);
    if (cfg.index_cache && index_cache_path[0]) {
        if (!beam_scan_cacheable(&completed)) {
            /* A partial result must not appear as a complete cached index
               on the next Workbench startup. */
            DeleteFile((STRPTR)index_cache_path);
        } else {
            beam_cache_save(&cfg,&beam_index,index_cache_path);
        }
    }
    beam_scan_progress_format(&completed,status,sizeof(status));
    if (index_memory_reduced && strlen(status)+28<sizeof(status)) {
        char reduced[32];
        sprintf(reduced,beam_str(MSG_LOWMEM_CAP),(long)cfg.max_items);
        strcat(status,reduced);
    }
}

static void scan_step(void)
{
    int budget=BEAM_SCAN_MAX_CHUNK, previous_items=item_count, i;
    int previous_selection=(selected<result_count && selected>=0) ? results[selected] : -1;
    int previous_action=action_mode;
    ULONG input_signals=(cx_port?(1UL<<cx_port->mp_SigBit):0UL)|
                        (window?(1UL<<window->UserPort->mp_SigBit):0UL);
    struct timeval start;
    char path[BEAM_MAX_PATH];
    GetSysTime(&start);
    while (scan_active && budget-->0) {
        struct BeamScanFrame *f;
        const char *filename;
        if ((budget&7)==0 &&
            ((SetSignal(0,0)&input_signals) ||
             micros_since(&start)>BEAM_SCAN_BUDGET_MICROS)) break;
        if (index_full && !scan_stale_marked) {
            /* A first build has reached its capacity: nothing left to do. */
            scan_complete(); break;
        }
        if (!scan_frames_used) {
            while (scan_root<cfg.dir_count && !(scan_roots&(1UL<<scan_root))) ++scan_root;
            if (scan_root>=cfg.dir_count) { scan_complete(); break; }
            if (!scan_push(cfg.dirs[scan_root],0,scan_root)) ++scan_unreadable;
            ++scan_root;
            continue;
        }
        f=&scan_frames[scan_frames_used-1];
        if (!ExNext(f->lock,f->fib)) {
            /* End-of-drawer is normal; distinguish it from disk I/O errors. */
            if (IoErr()!=ERROR_NO_MORE_ENTRIES) ++scan_io_errors;
            scan_pop(); continue;
        }
        ++scan_count;
        filename=(const char *)f->fib->fib_FileName;
        if (filename[0]=='.' &&
            (!filename[1] || (filename[1]=='.' && !filename[2]))) continue;
        if (strlen(f->path)+strlen(filename)+2>=sizeof(path)) {
            ++paths_truncated;continue;
        }
        beam_copy(path,f->path,sizeof(path));
        if (!AddPart(path,(STRPTR)filename,sizeof(path))) {++paths_truncated;continue;}
        if (beam_path_is_excluded(path,cfg.excluded_dirs,cfg.excluded_dir_count))
            continue;
        if (f->fib->fib_DirEntryType==ST_USERDIR) {
            if (cfg.index_drawers)
                index_add(path,BEAM_ITEM_DRAWER,f->root);
            if (f->depth<cfg.max_depth && !scan_push(path,f->depth+1,f->root))
                ++scan_unreadable;
        } else if (f->fib->fib_DirEntryType<0) {
            if (cfg.scan_icons && has_suffix_ci(filename,".info")) {
                int kind=icon_file_kind(path);
                /* Workbench launches both tools and projects.  Shell.info
                 * is a WBPROJECT whose default tool is SYS:System/CLI, so
                 * restricting the index to WBTOOL hides a standard OS3.2
                 * application that is plainly visible in Workbench. */
                if (kind) {
                    int index;
                    path[strlen(path)-5]=0;
                    index=index_add(path,kind,f->root);
                    if (kind==BEAM_ITEM_GAME && index>=0 && icon_slave[0]) {
                        char slave[BEAM_MAX_PATH];
                        beam_copy(slave,f->path,sizeof(slave));
                        if (AddPart((STRPTR)slave,(STRPTR)icon_slave,sizeof(slave)))
                            game_set_title(index,slave);
                    }
                }
            } else if (cfg.index_modules && beam_module_name(filename)) {
                index_add(path,BEAM_ITEM_MODULE,f->root);
            } else if (cfg.index_games && has_suffix_ci(filename,".slave")) {
                if (slave_without_icon(path))
                    game_set_title(index_add(path,BEAM_ITEM_GAME,f->root),path);
            } else if (!has_suffix_ci(filename,".info")) {
                if (cfg.index_files &&
                    beam_filename_patterns_match(filename,cfg.file_patterns))
                    index_add(path,BEAM_ITEM_FILE,f->root);
                /* FIBF_EXECUTE is active-low: set means NOT executable.
                   Only open files that are marked executable at all. */
                if (cfg.scan_executables &&
                    !(f->fib->fib_Protection&FIBF_EXECUTE) &&
                    is_native_executable(path))
                    index_add(path,BEAM_ITEM_SHELL,f->root);
            }
        }
        /* Soft links and hard drawer links are neither indexed as drawers
           nor followed: they duplicate entries and can form cycles. */
    }
    if (scan_active && !plugin_job.active && (scan_count%24)<16) {
        struct BeamScanStatus progress;
        scan_snapshot(&progress);
        beam_scan_progress_format(&progress,status,sizeof(status));
    }
    /* Throttle visible refreshes while scanning: re-ranking and redrawing
     * on every tick would dominate a 68000.  Always refresh on completion,
     * since the sweep renumbers items. */
    if (scan_redraw_ticks>0) --scan_redraw_ticks;
    if (window && (!scan_active ||
                   (scan_redraw_ticks==0 && item_count!=previous_items))) {
        scan_redraw_ticks=BEAM_SCAN_REDRAW_TICKS;
        if (!scan_active) previous_selection=-1; /* indices may have moved */
        search_items();
        if (previous_selection>=0) {
            for (i=0;i<result_count;i++) if (results[i]==previous_selection) {
                selected=i;action_mode=previous_action;break;
            }
        }
        render();
    } else if (!window && !scan_active) {
        result_count=0; /* sweep renumbered items; never keep stale results */
    }
}

/* The cache is deliberately not trusted as a live filesystem view. F2 and
 * REINDEX force an incremental real scan. There is no idle filesystem poll. */
static void refresh_index(int force_scan)
{
    scan_cancel();
    preview_clear();
    struct timeval started;
    int loaded;
    if (TimerBase) GetSysTime(&started);
    loaded=!force_scan && cfg.index_cache && index_cache_path[0] &&
           beam_cache_load(&cfg,&beam_index,index_cache_path);
    ++index_generation;
    if (TimerBase) timing_cache=(unsigned long)micros_since(&started);
    if (loaded) {
        index_full=(item_count>=cfg.max_items);
        paths_truncated=0;
        scan_root=0; scan_count=0; scan_unreadable=0; scan_io_errors=0;
        scan_roots=all_roots();
        scan_cached=1;
        bind_all_meta();
    } else scan_begin(all_roots());
}

static int query_prefix(const char *text,const char *prefix)
{
    while (*prefix) {
        int a=(unsigned char)*text++,b=(unsigned char)*prefix++;
        if (a>='A' && a<='Z') a+=32;
        if (b>='A' && b<='Z') b+=32;
        if (a!=b) return 0;
    }
    return 1;
}

static int query_word(const char *text,const char *word)
{
    unsigned int n=(unsigned int)strlen(word);
    return query_prefix(text,word) &&
        (!text[n] || text[n]==' ' || text[n]=='\t' || text[n]==':');
}

/* Build a visible, bounded completion menu without filesystem work or Shell
 * execution on each keystroke. */
static void completion_refresh(void)
{
    static const char *builtins[]={
        "app:","file:","drawer:","plugin:","recent:","volume:","fav:","game:","doc:","hidden:","mod:","web:","snip:","script:","prefs:","stats:",
        "shell:dir ","shell:list ","shell:assign ","shell:avail",
        "shell:version","shell:status ","open:RAM:","open:SYS:",
        "open:T:","open:ENV:","open:ENVARC:","open:C:","open:S:",
        "open:DEVS:","open:FONTS:","volumes","assigns",
        "diskfree:RAM:","diskfree:SYS:",NULL
    };
    char candidate[BEAM_QUERY_LEN+1];
    int i;
    beam_completion_reset(&completions);
    if (!query[0]) return;
    for (i=0;builtins[i];i++)
        beam_completion_add(&completions,query,builtins[i]);
    for (i=0;i<cfg.dir_count;i++) {
        if (strlen(cfg.dirs[i])+5>=sizeof(candidate)) continue;
        sprintf(candidate,"open:%s",cfg.dirs[i]);
        beam_completion_add(&completions,query,candidate);
    }
    for (i=0;i<plugin_count;i++) {
        const char *value=plugins[i].example[0]?plugins[i].example:plugins[i].prefix;
        if (value[0]) beam_completion_add(&completions,query,value);
    }
}

static int accept_completion(void)
{
    const char *value=beam_completion_selected(&completions);
    if (!value) return 0;
    beam_copy(query,value,sizeof(query));
    query_cursor=(int)strlen(query);query_view_start=0;search_items();return 1;
}

/* Return the exact payload to encode for a matched plugin. Named aliases
 * keep the command word; PREFIX= forms strip their routing prefix. */
static const char *query_plugin_input(const struct BeamLauncherPlugin *p)
{
    const char *q;
    if (!p || !query[0]) return NULL;
    if (query[0]==p->trigger) {
        q=query+1; while (*q==' ' || *q=='\t') ++q; return q;
    }
    if (p->prefix[0] && query_prefix(query,p->prefix)) {
        q=query+strlen(p->prefix); while (*q==' ' || *q=='\t') ++q; return q;
    }
    if (beam_equal(p->name,"System Commands") &&
        (query_word(query,"reboot") || query_word(query,"shutdown") ||
         query_word(query,"flush") || query_word(query,"avail") ||
         query_word(query,"version"))) return query;
    if (beam_equal(p->name,"Assigns and Volumes") &&
        (query_word(query,"volumes") || query_word(query,"assigns") ||
         query_word(query,"diskfree") || query_word(query,"open"))) return query;
    if (beam_equal(p->name,"Task Manager") &&
        (query_word(query,"tasks") || query_word(query,"task"))) return query;
    if (beam_equal(p->name,"Date Utilities") && query_word(query,"today")) return query;
    if (beam_equal(p->name,"Workbench Windows") &&
        (query_word(query,"windows") || query_word(query,"window") ||
         query_word(query,"screen"))) return query;
    return NULL;
}

static struct BeamLauncherPlugin *query_plugin(void)
{
    int i;
    if (!query[0]) return NULL;
    for (i=0;i<plugin_count;i++)
        if (query_plugin_input(&plugins[i])) return &plugins[i];
    return NULL;
}

static int browse_active;
static int ext_active;
static int args_item=-1;          /* item waiting for typed arguments */
static int selected_alias_index(void)
{
    if (browse_active || args_item>=0 || rows_active || ext_active) return -1;
    if (selected<0 || selected>=result_count || results[selected]<0) return -1;
    return (int)items[results[selected]].alias_index;
}

static int selected_has_custom_action(void)
{
    return beam_alias_action_command(&aliases,selected_alias_index())!=NULL;
}

/* Query narrowing: while a single-word query only grows (scr -> scre ->
 * scree), an item can match the longer query only if it matched the
 * shorter one (fuzz-tested: tests/test_narrowing.c), so only the previous
 * matches are scored again.  A bitmap of the last matches is kept; any
 * index change bumps index_generation and invalidates it. */
static unsigned char *cand_bits;
static int cand_valid,cand_scope;
static char cand_query[BEAM_QUERY_LEN+1];
static unsigned long cand_generation;

static int kick_confirm_item=-1; /* game whose missing Kickstart was shown */

/* Drawer browsing: Right on a drawer result lists its contents (outside
 * the index), Left goes to the parent, typing filters.  The list is
 * allocated only while browsing. */
#define BROWSE_NAME 64
struct BrowseEntry { char name[BROWSE_NAME]; unsigned char kind; };
static struct BrowseEntry *browse;
static int browse_count;
static char browse_dir[BEAM_MAX_PATH];

static void browse_exit(void)
{
    if (browse) { FreeVec(browse); browse=NULL; }
    browse_active=0; browse_count=0; browse_dir[0]=0;
}

static int browse_less(const struct BrowseEntry *a,const struct BrowseEntry *b)
{
    int da=a->kind==BEAM_ITEM_DRAWER,db=b->kind==BEAM_ITEM_DRAWER;
    const char *x=a->name,*y=b->name;
    if (da!=db) return da;  /* drawers first */
    while (*x && *y) {
        int cx=(*x>='A' && *x<='Z')?*x+32:*x,cy=(*y>='A' && *y<='Z')?*y+32:*y;
        if (cx!=cy) return cx<cy;
        ++x; ++y;
    }
    return !*x && *y;
}

/* List `dir` into the browse table.  Files with an icon open through
 * Workbench (WORKBENCH), others as plain FILE items. */
static int browse_enter(const char *dir)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int i,j,infos=0;
    if (!browse) browse=(struct BrowseEntry *)AllocVec(sizeof(struct BrowseEntry)*(ULONG)cfg.browse_max,MEMF_ANY|MEMF_CLEAR);
    if (!browse) return 0;
    lock=Lock((STRPTR)dir,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (!fib || !Examine(lock,fib) || fib->fib_DirEntryType<=0) {
        if (fib) FreeDosObject(DOS_FIB,fib);
        UnLock(lock); return 0;
    }
    if (!NameFromLock(lock,browse_dir,sizeof(browse_dir))) beam_copy(browse_dir,dir,sizeof(browse_dir));
    browse_count=0;
    while (browse_count<cfg.browse_max && ExNext(lock,fib)) {
        const char *n=(const char *)fib->fib_FileName;
        unsigned int len=(unsigned int)strlen(n);
        if (len>=BROWSE_NAME) continue;
        if (has_suffix_ci(n,".info")) { ++infos; continue; }
        if (fib->fib_DirEntryType!=ST_USERDIR && fib->fib_DirEntryType>0) continue;
        beam_copy(browse[browse_count].name,n,BROWSE_NAME);
        browse[browse_count].kind=(unsigned char)(fib->fib_DirEntryType>0?BEAM_ITEM_DRAWER:BEAM_ITEM_FILE);
        ++browse_count;
    }
    FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
    /* Files that have an icon become Workbench items. */
    if (infos) for (i=0;i<browse_count;i++) if (browse[i].kind==BEAM_ITEM_FILE) {
        char info[BEAM_MAX_PATH+8];
        BPTR il;
        beam_copy(info,browse_dir,sizeof(info));
        if (!AddPart((STRPTR)info,(STRPTR)browse[i].name,sizeof(info)) ||
            strlen(info)+5>=sizeof(info)) continue;
        strcat(info,".info");
        il=Lock((STRPTR)info,ACCESS_READ);
        if (il) { UnLock(il); browse[i].kind=BEAM_ITEM_WORKBENCH; }
    }
    for (i=1;i<browse_count;i++) {               /* insertion sort */
        struct BrowseEntry e=browse[i];
        for (j=i;j>0 && browse_less(&e,&browse[j-1]);j--) browse[j]=browse[j-1];
        browse[j]=e;
    }
    browse_active=1;
    query[0]=0; query_cursor=0; query_view_start=0; args_item=-1;
    sprintf(status,beam_str(browse_count?MSG_BROWSE_STATUS:MSG_BROWSE_EMPTY),
            browse_dir,(long)browse_count);
    return 1;
}

/* Live lists that are not part of the index: volumes and assigns
 * (volume:), recent documents (doc:) and hidden results (hidden:).  The
 * table is allocated while such a list is shown. */
#define EXT_CAP 64
struct ExtEntry { char name[48]; char path[BEAM_MAX_PATH]; char extra[24]; unsigned char kind; };
static struct ExtEntry *ext;
static int ext_count,ext_scope;

static void ext_exit(void)
{
    if (ext) { FreeVec(ext); ext=NULL; }
    ext_active=0; ext_count=0;
}

static void ext_add(const char *name,const char *path,int kind)
{
    int i;
    if (ext_count>=EXT_CAP || !*path || strlen(path)>=BEAM_MAX_PATH) return;
    for (i=0;i<ext_count;i++) if (beam_equal(ext[i].path,path)) return;
    beam_copy(ext[ext_count].name,name,sizeof(ext[0].name));
    beam_copy(ext[ext_count].path,path,BEAM_MAX_PATH);
    ext[ext_count].kind=(unsigned char)kind;
    ++ext_count;
}

/* Volumes and assigns from the DOS list.  Only names are copied while the
 * list is locked: no DOS I/O may happen under LockDosList(). */
static void ext_fill_volumes(void)
{
    static char names[EXT_CAP][32];
    static unsigned char kinds[EXT_CAP];
    struct DosList *dl;
    int n=0,i;
    dl=LockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_READ);
    while (n<EXT_CAP && (dl=NextDosEntry(dl,LDF_VOLUMES|LDF_ASSIGNS))) {
        UBYTE *b=(UBYTE *)BADDR(dl->dol_Name);
        unsigned int len=b?b[0]:0;
        if (!len || len>=sizeof(names[0])-1) continue;
        memcpy(names[n],b+1,len); names[n][len]=0;
        kinds[n]=(unsigned char)(dl->dol_Type==DLT_VOLUME?BEAM_ITEM_VOLUME:BEAM_ITEM_ASSIGN);
        ++n;
    }
    UnLockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_READ);
    for (i=0;i<n;i++) {
        char path[40];
        sprintf(path,"%.37s:",names[i]);
        ext_add(path,path,kinds[i]);
    }
}

/* prefs: lists the preference programs live from SYS:Prefs, whether or
 * not SYS:Prefs is an indexed drawer: every program there with an icon. */
static void ext_fill_prefs(void)
{
    static char names[EXT_CAP][48];
    BPTR lock=Lock((STRPTR)"SYS:Prefs",ACCESS_READ);
    struct FileInfoBlock *fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    int n=0,i,j;
    if (lock && fib && Examine(lock,fib)) {
        while (n<EXT_CAP && ExNext(lock,fib)) {
            const char *f=(const char *)fib->fib_FileName;
            unsigned int len=(unsigned int)strlen(f);
            if (fib->fib_DirEntryType>=0 || len>=sizeof(names[0]) || has_suffix_ci(f,".info"))
                continue;
            beam_copy(names[n++],f,sizeof(names[0]));
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    if (lock) UnLock(lock);
    for (i=1;i<n;i++) {                              /* alphabetical */
        char t[48];
        beam_copy(t,names[i],sizeof(t));
        for (j=i;j>0 && strcmp(names[j-1],t)>0;j--) beam_copy(names[j],names[j-1],sizeof(t));
        beam_copy(names[j],t,sizeof(t));
    }
    for (i=0;i<n;i++) {
        char path[BEAM_MAX_PATH],info[BEAM_MAX_PATH+8];
        BPTR il;
        sprintf(path,"SYS:Prefs/%.47s",names[i]);
        sprintf(info,"%s.info",path);
        il=Lock((STRPTR)info,ACCESS_READ);
        if (!il) continue;                           /* only real tools */
        UnLock(il);
        ext_add(names[i],path,BEAM_ITEM_WORKBENCH);
    }
}

static int ext_fill(int scope)
{
    int i;
    if (!ext) ext=(struct ExtEntry *)AllocVec(sizeof(struct ExtEntry)*EXT_CAP,MEMF_ANY|MEMF_CLEAR);
    if (!ext) return 0;
    ext_count=0; ext_scope=scope;
    if (scope==BEAM_SCOPE_VOLUME) ext_fill_volumes();
    else if (scope==BEAM_SCOPE_PREFS) ext_fill_prefs();
    else if (scope==BEAM_SCOPE_STATS) {
        /* Most-launched first; the counts come from the launch history. */
        int order[BEAM_HISTORY_CAP],n=0,j,k;
        for (i=0;i<(int)usage.count;i++) {
            for (j=n;j>0 && usage.entries[order[j-1]].launches<usage.entries[i].launches;j--)
                order[j]=order[j-1];
            order[j]=i; ++n;
        }
        for (k=0;k<n;k++) {
            const struct BeamHistoryEntry *h=&usage.entries[order[k]];
            int index=beam_index_find(&beam_index,h->path);
            int before=ext_count;
            ext_add(index>=0?items[index].name:beam_index_name_for(h->path),h->path,
                    index>=0?items[index].kind:BEAM_ITEM_FILE);
            if (ext_count>before) sprintf(ext[ext_count-1].extra,beam_str(MSG_STATS_LAUNCHES),h->launches);
        }
    }
    else if (scope==BEAM_SCOPE_DOC) {
        for (i=0;i<recent_docs.count;i++)
            ext_add(beam_index_name_for(recent_docs.paths[i]),recent_docs.paths[i],BEAM_ITEM_FILE);
    } else {
        for (i=0;i<hidden.count;i++) {
            int index=beam_index_find(&beam_index,hidden.paths[i]);
            ext_add(beam_index_name_for(hidden.paths[i]),hidden.paths[i],
                    index>=0?items[index].kind:BEAM_ITEM_FILE);
        }
    }
    ext_active=1;
    return 1;
}

/* Remember documents (files and Workbench projects, not tools). */
static void record_document(const char *path,int kind)
{
    if (kind==BEAM_ITEM_WORKBENCH) {
        char info[BEAM_MAX_PATH+8];
        UBYTE hdr[50];
        BPTR f;
        LONG got=0;
        if (strlen(path)+6>sizeof(info)) return;
        sprintf(info,"%s.info",path);
        f=Open((STRPTR)info,MODE_OLDFILE);
        if (f) { got=Read(f,hdr,sizeof(hdr)); Close(f); }
        if (got<49 || hdr[48]!=WBPROJECT) return;
    } else if (kind!=BEAM_ITEM_FILE) return;
    beam_recent_docs_record(&recent_docs,path);
    beam_recent_docs_save(&recent_docs,BEAM_RECENT_DOCS_PATH);
}

static int browse_parent(void)
{
    char parent[BEAM_MAX_PATH];
    unsigned int n=(unsigned int)strlen(browse_dir);
    if (!n || browse_dir[n-1]==':') { browse_exit(); return 0; } /* at a root */
    if (!beam_parent_drawer(browse_dir,parent,sizeof(parent))) { browse_exit(); return 0; }
    return browse_enter(parent);
}
static int random_pick;
static void search_items(void)
{
    int i,k,score,oldscore,scope;
    const char *search_query;
    char favorite_group[BEAM_FAVORITE_GROUP_LEN];
    int restart_kind=beam_restart_query(query);
    int previous_item=(selected>=0 && selected<result_count)?results[selected]:-1;
    int scores[BEAM_MAX_RESULTS];
    const int cap=cfg.results_max<=BEAM_MAX_RESULTS?cfg.results_max:BEAM_MAX_RESULTS;
    struct BeamQueryMasks query_masks;
    search_pending=0;
    if (args_item>=0) {
        /* The query field holds arguments, not a search. */
        result_count=0; beam_completion_reset(&completions);
        return;
    }
    if (rows_active) {
        /* Results are plugin rows, in the plugin's order. */
        beam_completion_reset(&completions);
        selected=0; result_scroll=0; action_mode=BEAM_ACTION_LAUNCH;
        result_count=plugin_row_count<BEAM_MAX_RESULTS?plugin_row_count:BEAM_MAX_RESULTS;
        for (i=0;i<result_count;i++) results[i]=i;
        return;
    }
    if (browse_active) {
        /* Results are browse-table indices, filtered by the query. */
        beam_completion_reset(&completions);
        result_count=0; selected=0; result_scroll=0; action_mode=BEAM_ACTION_LAUNCH;
        for (i=0;i<cap;i++) { results[i]=-1; scores[i]=0; }
        for (i=0;i<browse_count;i++) {
            score=query[0]?beam_match_score(query,browse[i].name):9000-i;
            if (!score) continue;
            for (k=0;k<cap;k++) if (score>scores[k]) {
                int j;
                for (j=cap-1;j>k;j--) { scores[j]=scores[j-1]; results[j]=results[j-1]; }
                scores[k]=score; results[k]=i; break;
            }
        }
        for (k=0;k<cap;k++) if (results[k]>=0) ++result_count;
        return;
    }
    result_count=0; selected=0; result_scroll=0; action_mode=BEAM_ACTION_LAUNCH;
    scope=beam_search_scope_parse(query,&search_query);
    favorite_group[0]=0;
    if (scope==BEAM_SCOPE_FAVORITE && *search_query) {
        unsigned int n=0;
        while (search_query[n] && search_query[n]!=' ' && search_query[n]!='\t') {
            if (n+1<sizeof(favorite_group)) favorite_group[n]=search_query[n];
            ++n;
        }
        if (n>=sizeof(favorite_group)) favorite_group[0]=0;
        else favorite_group[n]=0;
        search_query+=n;while (*search_query==' ' || *search_query=='\t') ++search_query;
    }
    if (!help_mode && (scope==BEAM_SCOPE_VOLUME || scope==BEAM_SCOPE_DOC ||
                       scope==BEAM_SCOPE_HIDDEN || scope==BEAM_SCOPE_STATS ||
                       scope==BEAM_SCOPE_PREFS)) {
        beam_completion_reset(&completions);
        for (i=0;i<cap;i++) { results[i]=-1; scores[i]=0; }
        if (!ext_fill(scope)) return;
        for (i=0;i<ext_count;i++) {
            score=*search_query?beam_match_score(search_query,ext[i].name):9000-i;
            if (!score) continue;
            for (k=0;k<cap;k++) if (score>scores[k]) {
                int j;
                for (j=cap-1;j>k;j--) { scores[j]=scores[j-1]; results[j]=results[j-1]; }
                scores[k]=score; results[k]=i; break;
            }
        }
        for (k=0;k<cap;k++) if (results[k]>=0) ++result_count;
        return;
    }
    ext_active=0;
    if (!help_mode && scope!=BEAM_SCOPE_PLUGIN) completion_refresh();
    else beam_completion_reset(&completions);
    for (i=0;i<cap;i++) { results[i]=-1; scores[i]=0; }
    if (restart_confirm && restart_confirm!=restart_kind) restart_confirm=0;
    if (help_mode || scope==BEAM_SCOPE_PLUGIN) {
        for (i=0;i<help_count;i++) {
            const struct BeamHelpEntry *entry=&help_entries[i];
            if (scope==BEAM_SCOPE_PLUGIN && entry->kind!=BEAM_HELP_PLUGIN) continue;
            if (!*search_query) score=9000-i;
            else {
                score=beam_match_score(search_query,entry->title);
                oldscore=beam_match_score(search_query,entry->insert);
                if (oldscore>score) score=oldscore;
                oldscore=beam_match_score(search_query,entry->detail);
                if (oldscore>score) score=oldscore;
            }
            if (!score) continue;
            for (k=0;k<cap;k++) if (score>scores[k]) {
                int j;for(j=cap-1;j>k;j--){scores[j]=scores[j-1];results[j]=results[j-1];}
                scores[k]=score;results[k]=i;break;
            }
        }
        for(k=0;k<cap;k++)if(results[k]>=0)++result_count;
        return;
    }
    if (query_plugin() || beam_shell_query(query,NULL) || restart_kind) return;
    kick_confirm_item=-1;
    random_pick=0;
    if (scope==BEAM_SCOPE_GAME && beam_equal(search_query,"random")) {
        /* One game, picked from the clock; Enter starts it. */
        struct DateStamp now;
        int games=0,pick;
        for (i=0;i<item_count;i++) if (items[i].kind==BEAM_ITEM_GAME) ++games;
        if (!games) return;
        DateStamp(&now);
        pick=(int)(((unsigned long)now.ds_Tick+(unsigned long)now.ds_Minute*3001UL)%
                   (unsigned long)games);
        for (i=0;i<item_count;i++)
            if (items[i].kind==BEAM_ITEM_GAME && pick--==0) {
                results[0]=i; result_count=1; random_pick=1; return;
            }
        return;
    }
    beam_query_masks_build(search_query,&query_masks);
    {
        struct timeval started;
        unsigned int qlen=(unsigned int)strlen(search_query),clen=(unsigned int)strlen(cand_query);
        int single=*search_query && !strpbrk(search_query," \t,;") &&
                   scope!=BEAM_SCOPE_RECENT && scope!=BEAM_SCOPE_FAVORITE;
        int narrow=single && cand_valid && cand_bits && cand_scope==scope &&
                   cand_generation==index_generation && clen>=3 && qlen>clen &&
                   !strncmp(search_query,cand_query,clen);
        if (single && !cand_bits)
            cand_bits=(unsigned char *)AllocVec((ULONG)beam_index.capacity/8UL+1UL,MEMF_ANY|MEMF_CLEAR);
        if (!cand_bits) single=narrow=0;
        if (TimerBase) GetSysTime(&started);
    for (i=0;i<item_count;i++) {
        const struct BeamItem *it=&items[i];
        if (narrow && !(cand_bits[i>>3]&(1<<(i&7)))) continue;
        if (single) cand_bits[i>>3]&=(unsigned char)~(1<<(i&7));
        int pinned=it->favorite==BEAM_ITEM_META_NONE?-1:(int)it->favorite;
        int recent=it->recent==BEAM_ITEM_META_NONE?-1:(int)it->recent;
        if (it->flags&BEAM_ITEM_FLAG_HIDDEN) continue;
        if (!beam_search_scope_kind_allowed(scope,it->kind)) continue;
        if (scope==BEAM_SCOPE_RECENT && recent<0) continue;
        if (scope==BEAM_SCOPE_FAVORITE &&
            (pinned<0 || (favorite_group[0] &&
             !beam_favorites_in_group(&favorites,it->path,favorite_group)))) continue;
        if (!*search_query && scope!=BEAM_SCOPE_ALL)
            score=9000-(pinned>=0?pinned:(recent>=0?recent:i));
        else if (!query[0] && pinned<0) {
            score=recent<0?0:9000-recent;
        } else {
            /* Cheap, sound early reject before any string matching. */
            if (beam_query_masks_reject(&query_masks,it->mask)) continue;
            score=beam_search_rank_alias(search_query,it->name,
                 beam_aliases_words(&aliases,(int)it->alias_index)?
                 beam_aliases_words(&aliases,(int)it->alias_index):it->title,pinned,
                 (unsigned int)it->bonus);
        }
        if (!score) continue;
        if (single) cand_bits[i>>3]|=(unsigned char)(1<<(i&7));
        for (k=0;k<cap;k++) {
            oldscore=scores[k];
            if (score>oldscore) {
                int j;
                for (j=cap-1;j>k;j--) {
                    scores[j]=scores[j-1]; results[j]=results[j-1];
                }
                scores[k]=score; results[k]=i;
                break;
            }
        }
    }
        /* The bitmap now holds exactly the matches of this query. */
        cand_valid=single;
        if (single) {
            beam_copy(cand_query,search_query,sizeof(cand_query));
            cand_scope=scope; cand_generation=index_generation;
        }
        if (narrow) ++timing_narrowed;
        if (TimerBase) timing_search=(unsigned long)micros_since(&started);
    }
    for (k=0;k<cap;k++) if (results[k]>=0) ++result_count;
    if (previous_item>=0) {
        for (k=0;k<result_count;k++) if (results[k]==previous_item) {
            selected=k;
            break;
        }
    }
}

/* One TextFit() call (graphics V36) replaces shortening the text a
 * character at a time with a TextLength() per step: for a long path that
 * was ~200 font measurements per cell on every redraw. */
/* Like draw_text, but underlines the characters whose bit is set in marks
 * (bit 0 = text[offset]).  Underlining works on every font and depth. */
static void draw_marked(struct RastPort *rp,int x,int y,const char *text,
                        unsigned long marks,int offset,int maxwidth)
{
    struct TextExtent extent;
    ULONG len=(ULONG)strlen(text),fit,i=0,j;
    if (!len || maxwidth<8) return;
    fit=TextFit(rp,(STRPTR)text,len,&extent,NULL,1,(ULONG)maxwidth,32767);
    if (!fit) return;
    Move(rp,x,y);
    while (i<fit) {
        int on=(int)i>=offset && (int)i-offset<32 && ((marks>>(i-(ULONG)offset))&1UL);
        for (j=i+1;j<fit;j++) {
            int next=(int)j>=offset && (int)j-offset<32 && ((marks>>(j-(ULONG)offset))&1UL);
            if (next!=on) break;
        }
        SetSoftStyle(rp,on?FSF_UNDERLINED:FS_NORMAL,FSF_UNDERLINED);
        Text(rp,(STRPTR)(text+i),j-i);
        i=j;
    }
    SetSoftStyle(rp,FS_NORMAL,FSF_UNDERLINED);
}

static void draw_text(struct RastPort *rp, int x, int y, const char *text, int maxwidth)
{
    struct TextExtent extent;
    ULONG len=(ULONG)strlen(text),fit;
    if (len==0 || maxwidth<8) return;
    fit=TextFit(rp,(STRPTR)text,len,&extent,NULL,1,(ULONG)maxwidth,32767);
    if (fit<len && maxwidth>=(int)TextLength(rp,(STRPTR)"...",3)) {
        fit=TextFit(rp,(STRPTR)text,len,&extent,NULL,1,
            (ULONG)(maxwidth-TextLength(rp,(STRPTR)"...",3)),32767);
        Move(rp,x,y); if (fit) Text(rp,(STRPTR)text,fit);
        Text(rp,(STRPTR)"...",3);
    } else if (fit>0) { Move(rp,x,y); Text(rp,(STRPTR)text,fit); }
}

/* The selected help description gets two full-width lines on small screens. */
static void draw_help_detail(struct RastPort *rp,int x,int y,const char *text,int width,int height)
{
    struct TextExtent extent;
    unsigned int next,n;
    ULONG fit;
    if (!text || width<8) return;
    fit=TextFit(rp,(STRPTR)text,strlen(text),&extent,NULL,1,(ULONG)width,32767);
    n=beam_text_line(text,(unsigned int)fit,&next);
    if (n) { Move(rp,x,y); Text(rp,(STRPTR)text,n); }
    if (next && text[next]) draw_text(rp,x,y+height,text+next,width);
}

/* Keep the text insertion cursor visible without allocating an edit buffer.
 * Long queries scroll horizontally; the search still uses the full string. */
static void draw_query_input(struct RastPort *rp, int left, int top, int width)
{
    int n=(int)strlen(query), x, i;
    if (width<10) return;
    if (query_cursor<0) query_cursor=0;
    if (query_cursor>n) query_cursor=n;
    if (query_view_start>query_cursor) query_view_start=query_cursor;
    if (query_view_start<0) query_view_start=0;
    while (query_view_start<query_cursor &&
           TextLength(rp,(STRPTR)(query+query_view_start),
                      query_cursor-query_view_start)>width-6)
        ++query_view_start;
    /* If the caret fits, expose some text to its left when moving backwards. */
    while (query_view_start>0 &&
           TextLength(rp,(STRPTR)(query+query_view_start-1),
                      query_cursor-query_view_start+1)<=width-6)
        --query_view_start;
    draw_text(rp,left,top,query+query_view_start,width-3);
    i=query_cursor-query_view_start;
    x=left+(i>0 ? (int)TextLength(rp,(STRPTR)(query+query_view_start),i) : 0);
    if (x<left+width-2) {
        SetAPen(rp,palette.text);
        Move(rp,x,top-(int)rp->TxBaseline);
        Draw(rp,x,top-(int)rp->TxBaseline+(int)rp->TxHeight);
    }
}

#define BEAM_RENDER_HEADER 1
#define BEAM_RENDER_BODY 2
#define BEAM_RENDER_ALL 3
static int render_pending;
/* Redraw only what changed: cursor movement touches the query line, list
 * navigation only the result area.  Typing redraws both. */
static void render_parts(int parts)
{
    struct RastPort *rp;
    struct BeamLauncherPlugin *plugin;
    int i, base, row, y, width, lineheight, reserved=0, result_width;
    int shell_query,restart_kind,scope,browse_mode;
    const char *shell_command=NULL;
    char label[256], aux[256];
    if (!window) return;
    if (search_pending) { search_items(); parts=BEAM_RENDER_ALL; }
    render_pending&=~parts;
    rp=window->RPort;
    width=window->Width-window->BorderLeft-window->BorderRight;
    base=window->BorderTop;
    lineheight=launcher_lineheight(rp);
    SetDrMd(rp,JAM1);
    if (parts&BEAM_RENDER_HEADER) {
        SetAPen(rp,palette.background);
        RectFill(rp,window->BorderLeft,base,
                 window->Width-window->BorderRight-1,base+lineheight+2);
        SetAPen(rp,palette.text);
        draw_text(rp,window->BorderLeft+8,base+(int)rp->TxBaseline+6,
                  "BEAM68  >",width-16);
        draw_query_input(rp,window->BorderLeft+94,base+(int)rp->TxBaseline+6,width-105);
        SetAPen(rp,palette.shadow);
        Move(rp,window->BorderLeft+6,base+lineheight+3);
        Draw(rp,window->Width-window->BorderRight-7,base+lineheight+3);
    }
    if (!(parts&BEAM_RENDER_BODY)) return;
    SetAPen(rp,palette.background);
    RectFill(rp,window->BorderLeft,base+lineheight+4,
             window->Width-window->BorderRight-1,
             window->Height-window->BorderBottom-1);
    plugin=query_plugin();
    scope=beam_search_scope_parse(query,NULL);
    browse_mode=help_mode || scope==BEAM_SCOPE_PLUGIN;
    shell_query=beam_shell_query(query,&shell_command);
    restart_kind=beam_restart_query(query);
    row=base+lineheight+9;
    SetAPen(rp,palette.text);
    if (completions.count) preview_clear();
    else if (plugin && plugin->icon[0]) {
        char icon_path[BEAM_MAX_PATH];
        if (beam_runtime_path(plugin->icon,icon_path,sizeof(icon_path)))
            preview_prepare_path(icon_path);
        else preview_clear();
    } else if (browse_mode || plugin || shell_query || restart_kind || !result_count) preview_clear();
    if (ext_active) {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-lineheight-8,result_count);
        result_scroll=beam_result_scroll(selected,result_scroll,visible,result_count);
        for (i=result_scroll;i<result_count && i<result_scroll+visible;i++) {
            const struct ExtEntry *e=&ext[results[i]];
            y=row+(i-result_scroll)*lineheight;
            if (i==selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,window->Width-window->BorderRight-5,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            beam_copy(label,beam_str(ext_scope==BEAM_SCOPE_STATS || ext_scope==BEAM_SCOPE_PREFS?MSG_BADGE_APP:
                      ext_scope==BEAM_SCOPE_HIDDEN?MSG_BADGE_HIDDEN:
                      ext_scope==BEAM_SCOPE_DOC?MSG_BADGE_DOC:
                      e->kind==BEAM_ITEM_ASSIGN?MSG_BADGE_ASSIGN:MSG_BADGE_VOLUME),sizeof(label));
            beam_copy(label+strlen(label),e->name,sizeof(label)-(unsigned int)strlen(label));
            draw_text(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,label,width/2-12);
            draw_text(rp,window->BorderLeft+width/2,y+(int)rp->TxBaseline,
                      e->extra[0]?e->extra:e->path,width/2-16);
        }
    } else if (rows_active) {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-lineheight-8,result_count);
        result_scroll=beam_result_scroll(selected,result_scroll,visible,result_count);
        for (i=result_scroll;i<result_count && i<result_scroll+visible;i++) {
            const struct Beam68ReplyRow *r=&plugin_rows[results[i]];
            y=row+(i-result_scroll)*lineheight;
            if (i==selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,window->Width-window->BorderRight-5,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            beam_copy(label,beam_str(MSG_BADGE_ROW),sizeof(label));
            beam_copy(label+strlen(label),r->text,sizeof(label)-(unsigned int)strlen(label));
            draw_text(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,label,
                      r->value[0]?width/2-12:width-18);
            if (r->value[0])
                draw_text(rp,window->BorderLeft+width/2,y+(int)rp->TxBaseline,r->value,width/2-16);
        }
    } else if (browse_active) {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-lineheight-8,result_count);
        result_scroll=beam_result_scroll(selected,result_scroll,visible,result_count);
        for (i=result_scroll;i<result_count && i<result_scroll+visible;i++) {
            const struct BrowseEntry *e=&browse[results[i]];
            y=row+(i-result_scroll)*lineheight;
            if (i==selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,window->Width-window->BorderRight-5,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            beam_copy(label,beam_str(e->kind==BEAM_ITEM_DRAWER?MSG_BADGE_DRAWER:
                      e->kind==BEAM_ITEM_WORKBENCH?MSG_BADGE_APP:MSG_BADGE_FILE),sizeof(label));
            beam_copy(label+strlen(label),e->name,sizeof(label)-(unsigned int)strlen(label));
            draw_marked(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,label,
                        query[0]?beam_match_marks(query,e->name):0UL,
                        (int)(strlen(label)-strlen(e->name)),width/2-12);
            draw_text(rp,window->BorderLeft+width/2,y+(int)rp->TxBaseline,browse_dir,width/2-16);
        }
    } else if (args_item>=0 && args_item<item_count) {
        beam_copy(label,beam_str(MSG_BADGE_ARGS),sizeof(label));
        beam_copy(label+strlen(label),items[args_item].name,sizeof(label)-(unsigned int)strlen(label));
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,label,width-18);
        sprintf(aux,"\"%.150s\" %.90s",items[args_item].path,query);
        draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,aux,width-18);
    } else if (completions.count) {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-lineheight-8,
                    (int)completions.count);
        if (visible>(int)completions.count) visible=(int)completions.count;
        for (i=0;i<visible;i++) {
            y=row+i*lineheight;
            if (i==(int)completions.selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,
                         window->Width-window->BorderRight-5,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            beam_copy(label,beam_str(MSG_BADGE_COMPLETE),sizeof(label));
            beam_copy(label+strlen(label),completions.values[i],
                      sizeof(label)-(unsigned int)strlen(label));
            draw_text(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,
                      label,width-18);
        }
    } else if (browse_mode && result_count) {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-2*lineheight-8,result_count);
        result_scroll=beam_result_scroll(selected,result_scroll,visible,result_count);
        for (i=result_scroll;i<result_count && i<result_scroll+visible;i++) {
            const struct BeamHelpEntry *entry=&help_entries[results[i]];
            const char *badge=entry->kind==BEAM_HELP_PLUGIN?beam_str(MSG_BADGE_PLUGIN):
                entry->kind==BEAM_HELP_ALIAS?beam_str(MSG_BADGE_ALIAS):
                entry->kind==BEAM_HELP_SHORTCUT?beam_str(MSG_BADGE_KEY):beam_str(MSG_BADGE_COMMAND);
            y=row+(i-result_scroll)*lineheight;
            if (i==selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,
                    window->Width-window->BorderRight-5,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            beam_copy(label,badge,sizeof(label));
            beam_copy(label+strlen(label),entry->title,
                      sizeof(label)-(unsigned int)strlen(label));
            draw_text(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,
                      label,width/2-12);
            draw_text(rp,window->BorderLeft+width/2,y+(int)rp->TxBaseline,
                      entry->detail,width/2-16);
        }
    } else if (plugin) {
        sprintf(label,beam_str(MSG_PLUGIN_LINE),plugin->name,
                plugin->category[0]?"  [":"",
                plugin->category[0]?plugin->category:"");
        if (plugin->category[0] && strlen(label)+2<sizeof(label)) strcat(label,"]");
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,label,width-18);
        if (plugin->trigger=='=' && beam_equal(plugin->name,"Calculator")) {
            /* Built-in *preview* for the default calculator prefix.  The
               external calculator plugin remains the Enter action and the
               calculation stays local. No child per keystroke. */
            const char *expr=query_plugin_input(plugin);
            long value;
            while (*expr==' ' || *expr=='\t') ++expr;
            if (!*expr) beam_copy(aux,beam_str(MSG_CALC_HINT),sizeof(aux));
            else if (beam_calculate(expr,&value))
            { aux[0]='='; aux[1]=' '; beam_calc_format(value,aux+2,sizeof(aux)-2); }
            else beam_copy(aux,beam_str(MSG_CALC_INVALID),sizeof(aux));
            draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                      aux,width-18);
        } else {
            int live=preview_text[0] && !strcmp(preview_for,query);
            draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                      live?preview_text:plugin->example[0]?plugin->example:plugin->description,
                      width-18);
        }
        if (selected_icon_usable)
            preview_draw(rp,window->Width-window->BorderRight-BEAM_PREVIEW_BOX-5,row+2);
    } else if (shell_query) {
        beam_copy(label,beam_str(MSG_BADGE_SHELL),sizeof(label));
        if (strlen(label)+strlen(cfg.shell_commands?beam_str(MSG_SHELL_COMMAND):beam_str(MSG_SHELL_DISABLED_LABEL))<sizeof(label))
            strcat(label,cfg.shell_commands?beam_str(MSG_SHELL_COMMAND):beam_str(MSG_SHELL_DISABLED_LABEL));
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,
                  label,width-18);
        draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                  *shell_command?shell_command:beam_str(MSG_SHELL_HINT),
                  width-18);
    } else if (restart_kind) {
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,
                  beam_str(MSG_RESTART_ACTION),width-18);
        draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                  restart_kind==BEAM_RESTART_AMIGA?
                  beam_str(MSG_RESTART_AMIGA_DESC):
                  beam_str(MSG_RESTART_WB_DESC),width-18);
    } else if (!query[0] && !result_count) {
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,
                  beam_str(MSG_EMPTY_LINE1),width-18);
        draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                  beam_str(MSG_EMPTY_LINE2),width-18);
    } else if (!result_count) {
        char first[80],second[80];
        beam_no_results_assistance(scan_active,item_count,
            first,sizeof(first),second,sizeof(second));
        draw_text(rp,window->BorderLeft+9,row+(int)rp->TxBaseline,
                  first,width-18);
        draw_text(rp,window->BorderLeft+9,row+lineheight+(int)rp->TxBaseline,
                  second,width-18);
    } else {
        int visible=beam_result_rows(row,lineheight,
                    window->Height-window->BorderBottom-lineheight-8,result_count);
        reserved=beam_preview_reserved(cfg.icon_preview,width,
                    window->Height-window->BorderBottom-lineheight-8-row);
        result_width=width-reserved;
        if (!reserved) preview_clear();
        result_scroll=beam_result_scroll(selected,result_scroll,visible,result_count);
        for (i=result_scroll;i<result_count && i<result_scroll+visible;i++) {
            const struct BeamItem *it=&items[results[i]];
            y=row+(i-result_scroll)*lineheight;
            if (i==selected) {
                SetAPen(rp,palette.fill);
                RectFill(rp,window->BorderLeft+4,y-2,
                         window->BorderLeft+result_width-4,y+lineheight-2);
                SetAPen(rp,palette.fill_text);
            } else SetAPen(rp,palette.text);
            if (action_mode!=BEAM_ACTION_LAUNCH) {
                beam_copy(label,beam_str(MSG_BADGE_ACTION),sizeof(label));
                beam_copy(label+strlen(label),it->name,sizeof(label)-(unsigned int)strlen(label));
            } else if (random_pick) {
                beam_copy(label,beam_str(MSG_BADGE_RANDOM),sizeof(label));
                beam_copy(label+strlen(label),it->name,sizeof(label)-(unsigned int)strlen(label));
            } else if (!query[0] && it->recent!=BEAM_ITEM_META_NONE) {
                beam_copy(label,beam_str(MSG_BADGE_RECENT),sizeof(label));
                beam_copy(label+strlen(label),it->name,sizeof(label)-(unsigned int)strlen(label));
            } else if (it->favorite!=BEAM_ITEM_META_NONE) {
                /* A plain ASCII marker works on low-depth Topaz displays. */
                beam_copy(label,beam_str(MSG_BADGE_FAVORITE),sizeof(label));
                beam_copy(label+strlen(label),it->name,sizeof(label)-(unsigned int)strlen(label));
            } else {
                beam_copy(label,it->kind==BEAM_ITEM_MODULE?beam_str(MSG_BADGE_MODULE):
                    it->kind==BEAM_ITEM_SCRIPT?beam_str(MSG_BADGE_SCRIPT):
                    it->kind==BEAM_ITEM_BOOKMARK?beam_str(MSG_BADGE_WEB):
                    it->kind==BEAM_ITEM_SNIPPET?beam_str(MSG_BADGE_SNIPPET):
                    it->kind==BEAM_ITEM_GAME?beam_str(MSG_BADGE_GAME):
                    it->kind==BEAM_ITEM_FILE?beam_str(MSG_BADGE_FILE):
                    it->kind==BEAM_ITEM_DRAWER?beam_str(MSG_BADGE_DRAWER):
                    it->kind==BEAM_ITEM_VOLUME?beam_str(MSG_BADGE_VOLUME):beam_str(MSG_BADGE_APP),sizeof(label));
                beam_copy(label+strlen(label),it->name,sizeof(label)-(unsigned int)strlen(label));
            }
            {
                /* Underline the letters that matched the query. */
                const char *typed;
                unsigned long marks=0;
                int scope_used=beam_search_scope_parse(query,&typed);
                if (*typed && !random_pick && scope_used!=BEAM_SCOPE_FAVORITE)
                    marks=beam_query_marks(typed,it->name);
                draw_marked(rp,window->BorderLeft+9,y+(int)rp->TxBaseline,label,marks,
                            (int)(strlen(label)-strlen(it->name)),result_width/2-12);
            }
            draw_text(rp,window->BorderLeft+result_width/2,y+(int)rp->TxBaseline,
                      it->kind==BEAM_ITEM_BOOKMARK?it->path:it->title?it->title:it->path,
                      result_width/2-16);
        }
        if (reserved && selected>=0 && selected<result_count) {
            const struct BeamItem *it=&items[results[selected]];
            preview_prepare(it);
            preview_draw(rp,window->BorderLeft+result_width,
                         row+2);
        }
    }
    SetAPen(rp,palette.text);
    if (!status[0] && browse_mode && result_count) {
        const struct BeamHelpEntry *entry=&help_entries[results[selected]];
        draw_help_detail(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-lineheight-8-(int)(rp->TxHeight-rp->TxBaseline),
                  entry->detail,width-18,lineheight);
    } else if (!status[0] && completions.count) {
        sprintf(aux,beam_str(MSG_FOOT_COMPLETE),
                (unsigned)completions.selected+1,(unsigned)completions.count);
        draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  aux,width-18);
    } else if (!status[0] && restart_kind) {
        draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  restart_confirm==restart_kind?
                  beam_str(MSG_FOOT_RESTART_CONFIRM):
                  beam_str(MSG_FOOT_RESTART_ARM),
                  width-18);
    } else if (!status[0] && shell_query) {
        draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  cfg.shell_commands && *shell_command?
                  beam_str(MSG_FOOT_SHELL):beam_str(MSG_FOOT_SHELL_OFF),
                  width-18);
    } else if (!status[0] && result_count && !query_plugin()) {
        sprintf(aux,beam_str(MSG_FOOT_RESULTS),
                selected+1,result_count,
                beam_action_name(action_mode,
                    beam_alias_action_label(&aliases,selected_alias_index())));
        draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  aux,width-18);
    } else if (status[0]) {
        beam_copy(label,beam_str(MSG_FOOT_RESULT),sizeof(label));
        beam_copy(aux,status,sizeof(aux));
        if (strlen(label)+strlen(aux)<sizeof(label)) strcat(label,aux);
        draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  label,width-18);
    } else {
        if (plugin_last_result[0]) {
            sprintf(label,beam_str(MSG_FOOT_LAST),plugin_last_name,
                plugin_last_result,plugin_last_ms/1000UL,(plugin_last_ms%1000UL)/100UL);
            draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  label,width-18);
        } else draw_text(rp,window->BorderLeft+9,
                  window->Height-window->BorderBottom-8-(int)(rp->TxHeight-rp->TxBaseline),
                  beam_str(MSG_FOOT_DEFAULT),width-18);
    }
}

static void render(void)
{
    render_parts(BEAM_RENDER_ALL);
}

static void palette_release(void)
{
    int i;
    if (pubscreen && pubscreen->ViewPort.ColorMap)
        for (i=0;i<theme_pen_count;i++)
            ReleasePen(pubscreen->ViewPort.ColorMap,(ULONG)theme_pens[i]);
    theme_pen_count=0;
}

static void palette_apply(void)
{
    int role;
    unsigned long red,green,blue;
    struct TagItem tags[2];
    beam_palette_init(&palette,
                      screen_drawinfo?screen_drawinfo->dri_Pens:NULL,
                      screen_drawinfo?screen_drawinfo->dri_NumPens:0);
    palette_release();
    if (!pubscreen || !pubscreen->ViewPort.ColorMap || cfg.reduced_colors ||
        cfg.theme==BEAM_THEME_WORKBENCH) {
        if (cfg.high_contrast && screen_drawinfo && screen_drawinfo->dri_NumPens>BEAM_PEN_FILLTEXT) {
            palette.fill=screen_drawinfo->dri_Pens[BEAM_PEN_FILL];
            palette.fill_text=screen_drawinfo->dri_Pens[BEAM_PEN_FILLTEXT];
        }
        return;
    }
    tags[0].ti_Tag=OBP_Precision; tags[0].ti_Data=PRECISION_GUI;
    tags[1].ti_Tag=TAG_END; tags[1].ti_Data=0;
    for (role=0;role<BEAM_THEME_ROLE_COUNT;role++) {
        LONG pen;
        if (!beam_theme_color_custom(cfg.theme,role,cfg.custom_colors,
                                     &red,&green,&blue)) break;
        pen=ObtainBestPenA(pubscreen->ViewPort.ColorMap,
                           red,green,blue,tags);
        if (pen<0) break;
        theme_pens[theme_pen_count++]=pen;
    }
    if (theme_pen_count!=BEAM_THEME_ROLE_COUNT) {
        palette_release();
        return; /* low-depth screen could not supply a complete palette */
    }
    palette.text=(unsigned short)theme_pens[BEAM_THEME_ROLE_TEXT];
    palette.shadow=(unsigned short)theme_pens[BEAM_THEME_ROLE_SHADOW];
    palette.fill=(unsigned short)theme_pens[BEAM_THEME_ROLE_FILL];
    palette.fill_text=(unsigned short)theme_pens[BEAM_THEME_ROLE_FILL_TEXT];
    palette.background=(unsigned short)theme_pens[BEAM_THEME_ROLE_BACKGROUND];
    palette.highlight=palette.fill_text;
    if (cfg.high_contrast && screen_drawinfo &&
        screen_drawinfo->dri_NumPens>BEAM_PEN_FILLTEXT) {
        palette.fill=screen_drawinfo->dri_Pens[BEAM_PEN_FILL];
        palette.fill_text=screen_drawinfo->dri_Pens[BEAM_PEN_FILLTEXT];
    }
}

static void hide_window(void)
{
    preview_clear();
    ext_exit();
    args_item=-1;
    rows_active=0;
    browse_exit();
    if (window) {
        struct IntuiMessage *pending;
        window_state_capture();
        /* Return queued Intuition messages before disposing of its port. */
        while ((pending=(struct IntuiMessage *)GetMsg(window->UserPort)))
            ReplyMsg((struct Message *)pending);
        CloseWindow(window); window=NULL;
    }
    if (launcher_font) { CloseFont(launcher_font);launcher_font=NULL; }
    last_clicked=-1;
    palette_release();
    if (screen_drawinfo && pubscreen) {
        FreeScreenDrawInfo(pubscreen,screen_drawinfo);
        screen_drawinfo=NULL;
    }
    if (pubscreen) { UnlockPubScreen(NULL,pubscreen); pubscreen=NULL; }
}

static int plugins_loaded;
static void load_plugins(void);
static void ensure_plugins(void)
{
    if (!plugins_loaded) load_plugins();
}

static void show_window(void)
{
    int w,h,maxheight,top,left,rows,font_height,state_index;
    struct TextAttr font_attr;
    struct Screen *active;
    if (window) { WindowToFront(window); ActivateWindow(window); return; }
    ensure_plugins();
    active=IntuitionBase?IntuitionBase->ActiveScreen:NULL;
    pubscreen=NULL;
    if (pubscreen_name(active,pubscreen_key,sizeof(pubscreen_key)))
        pubscreen=LockPubScreen((STRPTR)pubscreen_key);
    if (!pubscreen) {
        pubscreen=LockPubScreen(NULL);
        if (!pubscreen) return;
        if (!pubscreen_name(pubscreen,pubscreen_key,sizeof(pubscreen_key)))
            beam_copy(pubscreen_key,"Workbench",sizeof(pubscreen_key));
    }
    compact_active=cfg.compact_mode==BEAM_COMPACT_ALWAYS ||
        (cfg.compact_mode==BEAM_COMPACT_AUTO &&
         (pubscreen->Width<640 || pubscreen->Height<300));
    memset(&font_attr,0,sizeof(font_attr));
    font_attr.ta_Name=(STRPTR)cfg.font_name;font_attr.ta_YSize=(UWORD)cfg.font_size;
    font_attr.ta_Style=FS_NORMAL;font_attr.ta_Flags=0;
    /* OpenFont() only finds fonts already in memory (ROM topaz or fonts
       another program loaded); OpenDiskFont() loads any font from FONTS:. */
    launcher_font=DiskfontBase?OpenDiskFont(&font_attr):OpenFont(&font_attr);
    if (!launcher_font && DiskfontBase) launcher_font=OpenFont(&font_attr);
    font_height=launcher_font?(int)launcher_font->tf_YSize:(int)pubscreen->RastPort.TxHeight;
    state_index=window_state_find(pubscreen_key);
    w=last_width?last_width:(state_index>=0?window_states[state_index].width:cfg.window_width);
    if (w>(int)pubscreen->Width-8) w=(int)pubscreen->Width-8;
    rows=compact_active && cfg.visible>4?4:cfg.visible;
    h=last_height?last_height:(state_index>=0?window_states[state_index].height:
      (cfg.window_height?cfg.window_height:
        45+(rows+3)*(font_height+(compact_active?3:cfg.row_spacing))));
    maxheight=(int)pubscreen->Height-(int)pubscreen->BarHeight-5;
    if (h>maxheight) h=maxheight;
    if (w<230 || h<90) { hide_window(); return; }
    left=state_index>=0?window_states[state_index].left:(int)(pubscreen->Width-w)/2;
    top=state_index>=0?window_states[state_index].top:
        (int)pubscreen->BarHeight+1+(maxheight-h)/2;
    if(left<0)left=0;
    if(left+w>(int)pubscreen->Width)left=(int)pubscreen->Width-w;
    if(top<(int)pubscreen->BarHeight+1)top=(int)pubscreen->BarHeight+1;
    if(top+h>(int)pubscreen->Height)top=(int)pubscreen->Height-h;
    window=OpenWindowTags(NULL,
        WA_PubScreen,(ULONG)pubscreen,
        WA_Title,(ULONG)beam_str(MSG_WINDOW_TITLE),
        WA_Left,(ULONG)left,
        WA_Top,(ULONG)top,
        WA_Width,(ULONG)w,
        WA_Height,(ULONG)h,
        WA_SizeGadget,TRUE,
        WA_MinWidth,230UL,WA_MaxWidth,(ULONG)(pubscreen->Width-8),
        WA_MinHeight,90UL,WA_MaxHeight,(ULONG)maxheight,
        WA_DragBar,TRUE,
        WA_CloseGadget,TRUE,
        WA_DepthGadget,FALSE,
        WA_SmartRefresh,TRUE,
        WA_Activate,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_RAWKEY|
                  IDCMP_REFRESHWINDOW|IDCMP_NEWSIZE|IDCMP_MOUSEBUTTONS,
        TAG_DONE);
    if (!window) { hide_window(); return; }
    if (launcher_font) SetFont(window->RPort,launcher_font);
    screen_drawinfo=GetScreenDrawInfo(pubscreen);
    palette_apply();
    query[0]=0; query_cursor=0; query_view_start=0;
    if (plugin_job.active) plugin_status(beam_str(MSG_PLUGIN_RUNNING));
    else if (scan_active) {
        struct BeamScanStatus progress;
        scan_snapshot(&progress);
        beam_scan_progress_format(&progress,status,sizeof(status));
    } else if (shell_notice[0]) {
        beam_copy(status,shell_notice,sizeof(status)); shell_notice[0]=0;
    } else status[0]=0;
    search_items(); render();
}

static void plugin_status(const char *message)
{
    beam_copy(status,plugin_active_name,sizeof(status));
    if (*status && strlen(status)+1<sizeof(status)) strcat(status," ");
    if (strlen(status)+strlen(message)<sizeof(status)) strcat(status,message);
    else beam_copy(status,message,sizeof(status));
}

/* Native protocol-1 plugins reply through the public message port. ARexx
 * scripts use the atomic text transport. Neither blocks the event loop. */
static void plugin_finish(const char *reply,int is_error)
{
    char part_path[BEAM_MAX_PATH+6];
    char timing[24];
    unsigned long elapsed=beam_job_elapsed_ms(&plugin_job);
    if (preview_job) {
        preview_job=0;
        if (!strcmp(query,plugin_original_query)) {
            beam_copy(preview_text,is_error?"":reply,sizeof(preview_text));
            beam_copy(preview_for,query,sizeof(preview_for));
        }
        beam_job_end(&plugin_job);
        DeleteFile((STRPTR)plugin_result_path);
        if (strlen(plugin_result_path)+5<sizeof(part_path)) {
            sprintf(part_path,"%s.part",plugin_result_path);
            DeleteFile((STRPTR)part_path);
        }
        if (window) render();
        return;
    }
    if (!strcmp(query,plugin_original_query)) {
        if (is_error) {
            plugin_status(beam_str(MSG_PLUGIN_ERROR));
            if (strlen(status)+1+strlen(reply)<sizeof(status)) {
                strcat(status," "); strcat(status,reply);
            }
        } else {
            beam_copy(status,reply,sizeof(status));
            beam_copy(plugin_last_result,reply,sizeof(plugin_last_result));
            beam_copy(plugin_last_name,plugin_active_name,sizeof(plugin_last_name));
            plugin_last_ms=elapsed;
        }
        sprintf(timing," (%lu.%lus)",elapsed/1000UL,(elapsed%1000UL)/100UL);
        if (strlen(status)+strlen(timing)<sizeof(status)) strcat(status,timing);
    }
    beam_plugin_health_record(&plugin_health,plugin_active_manifest,
        is_error?BEAM_PLUGIN_HEALTH_ERROR:BEAM_PLUGIN_HEALTH_SUCCESS);
    if (is_error && beam_plugin_health_is_quarantined(&plugin_health,
                                                       plugin_active_manifest))
        plugin_status(beam_str(MSG_PLUGIN_QUARANTINED_NOW));
    beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    beam_job_end(&plugin_job);
    /* The finished result files are ours. Timeout does NOT delete its path:
     * the unsupervised child may still be writing it. */
    DeleteFile((STRPTR)plugin_result_path);
    if (strlen(plugin_result_path)+5<sizeof(part_path)) {
        sprintf(part_path,"%s.part",plugin_result_path);
        DeleteFile((STRPTR)part_path);
    }
    if (window) render();
}
static void plugin_cancel(void)
{
    char part_path[BEAM_MAX_PATH+6];
    unsigned long elapsed;
    if (!plugin_job.active) return;
    elapsed=beam_job_elapsed_ms(&plugin_job);
    beam_job_end(&plugin_job);
    DeleteFile((STRPTR)plugin_result_path);
    if (strlen(plugin_result_path)+5<sizeof(part_path)) {
        sprintf(part_path,"%s.part",plugin_result_path);
        DeleteFile((STRPTR)part_path);
    }
    sprintf(status,beam_str(MSG_PLUGIN_CANCELED),
        plugin_active_name,elapsed/1000UL,(elapsed%1000UL)/100UL);
    if (window) render();
}
static void plugin_poll(void)
{
    BPTR f;
    char line[BEAM_PLUGIN_MAX_RESULT+16],out[BEAM_PLUGIN_MAX_RESULT+1];
    int parsed,error;
    if (!plugin_job.active) return;
    f=Open((STRPTR)plugin_result_path,MODE_OLDFILE);
    if (f) {
        if (FGets(f,line,sizeof(line))) {
            parsed=beam_plugin_parse_reply(line,out,sizeof(out),&error);
            Close(f);
            if (parsed==1) { plugin_finish(out,error);return; }
            if (parsed<0) { plugin_finish(beam_str(MSG_PLUGIN_INVALID_RESPONSE),1);return; }
        } else Close(f);
    }
    if (beam_job_tick(&plugin_job)) {
        beam_job_end(&plugin_job);
        if (preview_job) { preview_job=0; return; } /* a slow preview is no failure */
        beam_plugin_health_record(&plugin_health,plugin_active_manifest,
                                  BEAM_PLUGIN_HEALTH_TIMEOUT);
        beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
        /* No forced child termination: classic Shell provides no portable,
         * safe way to kill a potentially hung third-party process. */
        if (!strcmp(query,plugin_original_query)) {
            if (beam_plugin_health_is_quarantined(&plugin_health,
                                                   plugin_active_manifest))
                plugin_status(beam_str(MSG_PLUGIN_TIMEOUT_QUARANTINED));
            else plugin_status(beam_str(MSG_PLUGIN_TIMEOUT));
        }
        if (window) render();
    }
}
/* "Resident" plugins, done safely.  AmigaDOS can only keep *pure*
 * (re-entrant) programs resident; ordinary C programs keep static data
 * between runs, so re-running one loaded segment is unsafe for arbitrary
 * plugins.  Instead PLUGIN_CACHE=YES copies a plugin's package drawer to
 * T:Beam68/PluginCache/<Drawer>/ (RAM) on first use and runs it from there:
 * each run still loads a fresh copy, but from RAM instead of floppy/CF.
 * The copy is refreshed whenever the original executable changes. */
static int same_file(const char *a,const char *b)
{
    struct FileInfoBlock *fa,*fb;
    BPTR la,lb;
    int same=0;
    la=Lock((STRPTR)a,ACCESS_READ);
    lb=Lock((STRPTR)b,ACCESS_READ);
    fa=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    fb=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (la && lb && fa && fb && Examine(la,fa) && Examine(lb,fb))
        same=fa->fib_Size==fb->fib_Size &&
             fa->fib_Date.ds_Days==fb->fib_Date.ds_Days &&
             fa->fib_Date.ds_Minute==fb->fib_Date.ds_Minute &&
             fa->fib_Date.ds_Tick==fb->fib_Date.ds_Tick;
    if (fa) FreeDosObject(DOS_FIB,fa);
    if (fb) FreeDosObject(DOS_FIB,fb);
    if (la) UnLock(la);
    if (lb) UnLock(lb);
    return same;
}

static int plugin_cached_command(const char *command,char *out,unsigned int cap)
{
    char drawer[BEAM_MAX_PATH],cache[BEAM_MAX_PATH],copy[BEAM_MAX_PATH*2+48];
    const char *name=(const char *)FilePart((STRPTR)command);
    BPTR lock;
    if (!cfg.plugin_cache || !*name || !beam_parent_drawer(command,drawer,sizeof(drawer)) ||
        strchr(drawer,'"')) return 0;
    beam_copy(cache,"T:Beam68/PluginCache",sizeof(cache));
    if (!AddPart((STRPTR)cache,FilePart((STRPTR)drawer),sizeof(cache))) return 0;
    beam_copy(out,cache,cap);
    if (!AddPart((STRPTR)out,(STRPTR)name,(LONG)cap)) return 0;
    if (same_file(command,out)) return 1;
    lock=CreateDir((STRPTR)"T:Beam68"); if (lock) UnLock(lock);
    lock=CreateDir((STRPTR)"T:Beam68/PluginCache"); if (lock) UnLock(lock);
    lock=CreateDir((STRPTR)cache); if (lock) UnLock(lock);
    if (strlen(drawer)+strlen(cache)+40>=sizeof(copy)) return 0;
    sprintf(copy,"C:Copy \"%s/#?\" TO \"%s\" ALL CLONE QUIET",drawer,cache);
    if (beam_system(copy,NULL,NULL,0)!=0) return 0;
    return same_file(command,out);
}

static void execute_plugin(struct BeamLauncherPlugin *p);
static void arm_timer(void);
static void schedule_preview(void)
{
    struct BeamLauncherPlugin *p=query_plugin();
    preview_ticks=0;
    if (!p || !p->preview || !*query_plugin_input(p)) return;
    preview_ticks=BEAM_PREVIEW_TICKS;
    arm_timer();
}

static void run_preview(void)
{
    struct BeamLauncherPlugin *p=query_plugin();
    if (!window || !p || !p->preview || rows_active || !*query_plugin_input(p)) return;
    if (plugin_job.active) { preview_ticks=BEAM_PREVIEW_TICKS/2; arm_timer(); return; }
    executing_preview=1;
    execute_plugin(p);
    executing_preview=0;
    preview_job=plugin_job.active;
}

static void execute_plugin(struct BeamLauncherPlugin *p)
{
    const char *q=query_plugin_input(p);
    char hex[BEAM_QUERY_LEN*2+2],command[512];
    struct DateStamp stamp;
    LONG rc;
    int plugin_index=(p>=plugins && p<plugins+plugin_count)?(int)(p-plugins):-1;
    if (plugin_job.active && preview_job && !executing_preview) {
        /* Enter wins over a running preview; its late reply is ignored. */
        beam_job_end(&plugin_job); preview_job=0;
    }
    if (plugin_job.active) {
        if (!executing_preview) { plugin_status(beam_str(MSG_PLUGIN_BUSY)); render(); }
        return;
    }
    if (p>=plugins && p<plugins+plugin_count &&
        beam_plugin_health_is_quarantined(&plugin_health,
                                           plugin_manifest[p-plugins])) {
        beam_copy(plugin_active_name,p->name,sizeof(plugin_active_name));
        plugin_status(beam_str(MSG_PLUGIN_IS_QUARANTINED));render();return;
    }
    if (beam_capability_list_has(p->capabilities,"SYSTEM") &&
        (plugin_confirm!=plugin_index || !beam_equal(plugin_confirm_query,query))) {
        plugin_confirm=plugin_index;
        beam_copy(plugin_confirm_query,query,sizeof(plugin_confirm_query));
        beam_copy(status,beam_str(MSG_PLUGIN_SYSTEM_CONFIRM),sizeof(status));
        render();return;
    }
    plugin_confirm=-1;plugin_confirm_query[0]=0;
    beam_copy(plugin_active_name,p->name,sizeof(plugin_active_name));
    if (p>=plugins && p<plugins+plugin_count)
        beam_copy(plugin_active_manifest,plugin_manifest[p-plugins],sizeof(plugin_active_manifest));
    else plugin_active_manifest[0]=0;
    if (!q) { plugin_status(beam_str(MSG_PLUGIN_ERR_QUERY));render();return; }
    if (!beam_plugin_hex_argument(q,hex,sizeof(hex))) {
        plugin_status(beam_str(MSG_PLUGIN_ERR_LONG));render();return;
    }
    /* A unique path prevents late results from a timed-out process from
     * being mistaken for those of the next invocation. */
    DateStamp(&stamp);
    ++plugin_serial;
    sprintf(plugin_result_path,"T:Beam68-%08lx-%08lx-%04x",
            (unsigned long)FindTask(NULL),
            ((unsigned long)stamp.ds_Minute*3000UL+(unsigned long)stamp.ds_Tick),
            (unsigned int)(plugin_serial&65535U));
    {
        char cached[BEAM_MAX_PATH];
        const char *run=plugin_cached_command(p->command,cached,sizeof(cached))?cached:p->command;
        int rexx_script=strlen(run)>5 && beam_equal(run+strlen(run)-5,".rexx");
        if (rexx_script) {
            int rexx;
            Forbid(); rexx=FindPort((STRPTR)"REXX")!=NULL; Permit();
            if (!rexx) {
                if (!executing_preview) { plugin_status(beam_str(MSG_REXX_NOT_RUNNING)); render(); }
                return;
            }
        }
        if (!rexx_script && !reply_port) {
            if (!executing_preview) { plugin_status(beam_str(MSG_PLUGIN_INVALID_RESPONSE)); render(); }
            return;
        }
        if (!beam_plugin_command(run,hex,plugin_result_path,rexx_script,
                !rexx_script && reply_port?BEAM68_REPLY_PORT:NULL,
                (unsigned long)plugin_serial,command,sizeof(command))) {
            plugin_status(beam_str(MSG_PLUGIN_ERR_PATH));render();return;
        }
    }
    /* The child owns separate NIL: handles (see dos_compat.c), so it can
     * never close the launcher's inherited streams on any OS 3.x. */
    rc=beam_system(command,NULL,NULL,1);
    if (rc==-1) {
        beam_plugin_health_record(&plugin_health,plugin_active_manifest,
                                  BEAM_PLUGIN_HEALTH_ERROR);
        beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
        plugin_status(beam_str(MSG_PLUGIN_ERR_START)); render();return;
    }
    beam_copy(plugin_original_query,query,sizeof(plugin_original_query));
    beam_query_history_record(&query_history,query);
    if (query_history.pending>=4)
        beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
    beam_job_begin_timeout(&plugin_job,cfg.plugin_timeout);
    plugin_status(beam_str(MSG_PLUGIN_RUNNING));
    arm_timer(); render();
}

static void bind_history_meta(void)
{
    int i;
    for (i=0;i<item_count;i++) {
        struct BeamItem *it=&items[i];
        int pos=cfg.history?beam_history_recent_position(&usage,it->path):-1;
        unsigned int bonus=cfg.history?beam_history_bonus(&usage,it->path):0;
        it->recent=(unsigned char)((pos>=0 && pos<BEAM_ITEM_META_NONE)?pos:BEAM_ITEM_META_NONE);
        it->bonus=(unsigned char)(bonus>144?144:bonus);
    }
}

static void bind_favorite_meta(void)
{
    int i;
    for (i=0;i<item_count;i++) {
        int pos=beam_favorites_position(&favorites,items[i].path);
        items[i].favorite=(unsigned char)((pos>=0 && pos<BEAM_ITEM_META_NONE)?pos:BEAM_ITEM_META_NONE);
    }
}

/* ------------------------------------------------------- Directory Opus
 * Talks to Directory Opus over ARexx (port DOPUS_PORT, "DOPUS.1"):
 * Opus 5/Magellan understands "lister new <path>", Opus 4 "ScanDir <path>". */
static int dopus_running(void)
{
    int found;
    Forbid();
    found=FindPort((STRPTR)cfg.dopus_port)!=NULL;
    Permit();
    return found;
}

static void outgoing_rexx_free_message(void)
{
    /* On an error rm_Result2 is a numeric error code, not an argstring. */
    if (!outgoing_rexx.message->rm_Result1 && outgoing_rexx.message->rm_Result2)
        DeleteArgstring((UBYTE *)outgoing_rexx.message->rm_Result2);
    beam_rexx_message_delete(outgoing_rexx.message);
    outgoing_rexx.message=NULL;
}
static void outgoing_rexx_close(void)
{
    if (outgoing_rexx.message) {
        /* The external host still owns it. Never free a message under it.
         * At most one silent orphan is left when Beam68 itself exits. */
        Forbid(); outgoing_rexx.reply->mp_Flags=PA_IGNORE; Permit();
    } else if (outgoing_rexx.reply) DeleteMsgPort(outgoing_rexx.reply);
    outgoing_rexx.reply=NULL;
}
static int outgoing_rexx_send(const char *command,int stage)
{
    struct MsgPort *host;
    struct RexxMsg *rm;
    if (!RexxSysBase || !command || outgoing_rexx.message) return 0;
    if (!outgoing_rexx.reply) outgoing_rexx.reply=CreateMsgPort();
    if (!outgoing_rexx.reply) return 0;
    rm=beam_rexx_message_create(outgoing_rexx.reply,command,1);
    if (!rm) { outgoing_rexx_close(); return 0; }
    Forbid();
    host=FindPort((STRPTR)outgoing_rexx.port);
    if (host) PutMsg(host,(struct Message *)rm);
    Permit();
    if (!host) {
        beam_rexx_message_delete(rm);
        outgoing_rexx_close(); return 0;
    }
    outgoing_rexx.message=rm; outgoing_rexx.stage=stage;
    outgoing_rexx.timed_out=0;
    GetSysTime(&outgoing_rexx.started); arm_timer();
    return 1; /* queued, completion is handled by the normal event loop */
}
static void outgoing_rexx_failure(int stage)
{
    const char *text=beam_str(stage==REXX_MODULE?MSG_SOURCE_LAUNCH_FAILED:MSG_DOPUS_FAILED);
    beam_copy(shell_notice,text,sizeof(shell_notice));
    if (window) { beam_copy(status,text,sizeof(status)); render(); }
}
static void outgoing_rexx_poll(void)
{
    char command[BEAM_MAX_PATH+80];
    struct Message *reply;
    int ok,stage,next=0;
    if (!outgoing_rexx.message) return;
    reply=GetMsg(outgoing_rexx.reply);
    if (!reply) {
        if (!outgoing_rexx.timed_out &&
            micros_since(&outgoing_rexx.started)>=3000000L) {
            outgoing_rexx.timed_out=1;
            outgoing_rexx_failure(outgoing_rexx.stage);
        }
        return;
    }
    stage=outgoing_rexx.stage;
    ok=outgoing_rexx.message->rm_Result1==0;
    if (stage==REXX_DOPUS_NEW) outgoing_rexx.handle[0]=0;
    if (stage==REXX_DOPUS_NEW && ok && outgoing_rexx.message->rm_Result2) {
        const char *handle=(const char *)outgoing_rexx.message->rm_Result2;
        if (beam_rexx_handle_valid(handle))
            beam_copy(outgoing_rexx.handle,handle,sizeof(outgoing_rexx.handle));
    }
    outgoing_rexx_free_message();
    if (outgoing_rexx.timed_out) { outgoing_rexx_close(); return; }
    next=beam_rexx_next(stage,ok,outgoing_rexx.handle,outgoing_rexx.drawer,
        outgoing_rexx.select,command,sizeof(command));
    if (next>0) {
        if (outgoing_rexx_send(command,next)) return;
        ok=0;
    } else if (next<0) ok=0;
    if (!ok) outgoing_rexx_failure(stage);
    outgoing_rexx_close();
}
static int arexx_send(const char *port_name,const char *command,char *result,unsigned int cap)
{
    (void)result; (void)cap; /* Module actions need completion status, not text. */
    if (!port_name || strlen(port_name)>=sizeof(outgoing_rexx.port) || outgoing_rexx.message) return 0;
    beam_copy(outgoing_rexx.port,port_name,sizeof(outgoing_rexx.port));
    return outgoing_rexx_send(command,REXX_MODULE);
}
static int dopus_show_select(const char *drawer,const char *select)
{
    char command[BEAM_MAX_PATH+48];
    if (!dopus_running() || !drawer || outgoing_rexx.message ||
        strlen(cfg.dopus_port)>=sizeof(outgoing_rexx.port) ||
        strlen(drawer)>=sizeof(outgoing_rexx.drawer) ||
        strpbrk(drawer,"\"*\r\n") ||
        (select && (strlen(select)>=sizeof(outgoing_rexx.select) || strpbrk(select,"\"*\r\n")))) return 0;
    beam_copy(outgoing_rexx.port,cfg.dopus_port,sizeof(outgoing_rexx.port));
    beam_copy(outgoing_rexx.drawer,drawer,sizeof(outgoing_rexx.drawer));
    beam_copy(outgoing_rexx.select,select?select:"",sizeof(outgoing_rexx.select));
    sprintf(command,"lister new \"%s\"",drawer);
    return outgoing_rexx_send(command,REXX_DOPUS_NEW);
}

static int dopus_show(const char *drawer)
{
    return dopus_show_select(drawer,NULL);
}

/* Open a path the way Workbench would.  workbench.library V44+ does it all;
 * on OS 3.0/3.1 tools and projects are started with a WBStartup message,
 * while drawers/volumes cannot be opened without the V44 API. */
static int workbench_open(const char *path,int kind)
{
    if (WorkbenchBase && WorkbenchBase->lib_Version>=44)
        return OpenWorkbenchObjectA((STRPTR)path,NULL)!=0;
    if (kind==BEAM_ITEM_DRAWER || kind==BEAM_ITEM_VOLUME || !wb_reply_port)
        return 0;
    return beam_wb_launch(wb_reply_port,path);
}

static void launch_selected(void)
{
    struct BeamItem *it;
    static char command[BEAM_ALIAS_ACTION_COMMAND_LEN+BEAM_MAX_PATH*2+16];
    char shell_text[BEAM_QUERY_LEN+1];
    LONG rc;
    const char *shell_command=NULL;
    int restart_kind=beam_restart_query(query);
    int scope=beam_search_scope_parse(query,NULL);
    if ((help_mode || scope==BEAM_SCOPE_PLUGIN) && result_count &&
        selected>=0 && selected<result_count) {
        const struct BeamHelpEntry *entry=&help_entries[results[selected]];
        if (!entry->insert[0]) {
            beam_copy(status,entry->detail,sizeof(status));render();return;
        }
        beam_copy(query,entry->insert,sizeof(query));
        query_cursor=(int)strlen(query);query_view_start=0;help_mode=0;
        status[0]=0;search_items();render();return;
    }
    if (ext_active) {
        const struct ExtEntry *e;
        int kind;
        if (!result_count || selected>=result_count) return;
        e=&ext[results[selected]];
        if (ext_scope==BEAM_SCOPE_STATS) {
            int index=beam_index_find(&beam_index,e->path);
            if (index>=0) {
                /* Launch exactly as from a normal search result. */
                ext_exit();
                results[0]=index; result_count=1; selected=0; action_mode=BEAM_ACTION_LAUNCH;
                launch_selected();
                return;
            }
        }
        kind=(e->kind==BEAM_ITEM_VOLUME || e->kind==BEAM_ITEM_ASSIGN)?BEAM_ITEM_DRAWER:e->kind;
        if (kind==BEAM_ITEM_DRAWER && cfg.dopus_drawers && dopus_show(e->path)) {
            hide_window(); return;
        }
        if (!workbench_open(e->path,kind)) {
            beam_copy(status,beam_str(kind==BEAM_ITEM_DRAWER?MSG_DRAWER_NEEDS_V44:MSG_WB_LAUNCH_FAILED),
                      sizeof(status));
            render(); return;
        }
        if (ext_scope==BEAM_SCOPE_DOC) record_document(e->path,BEAM_ITEM_FILE);
        hide_window(); return;
    }
    if (rows_active) {
        const struct Beam68ReplyRow *r;
        if (!result_count || selected>=result_count) return;
        r=&plugin_rows[results[selected]];
        if (r->kind==BEAM68_ROW_OPEN && r->value[0]) {
            record_document(r->value,BEAM_ITEM_FILE);
            if (strchr(r->value,'"') || !workbench_open(r->value,BEAM_ITEM_FILE)) {
                beam_copy(status,beam_str(MSG_WB_LAUNCH_FAILED),sizeof(status));
                render(); return;
            }
            hide_window(); return;
        }
        if (r->kind==BEAM68_ROW_QUERY && r->value[0]) {
            rows_active=0;
            beam_copy(query,r->value,sizeof(query));
            query_cursor=(int)strlen(query); query_view_start=0; status[0]=0;
            search_items(); render(); return;
        }
        beam_copy(status,beam_str(beam_clip_write(r->text)?MSG_COPIED:MSG_COPY_FAILED),sizeof(status));
        render(); return;
    }
    if (browse_active) {
        char path[BEAM_MAX_PATH];
        const struct BrowseEntry *e;
        if (!result_count || selected>=result_count) return;
        e=&browse[results[selected]];
        beam_copy(path,browse_dir,sizeof(path));
        if (!AddPart((STRPTR)path,(STRPTR)e->name,sizeof(path))) return;
        if (e->kind==BEAM_ITEM_DRAWER) { browse_enter(path); search_items(); render(); return; }
        record_document(path,e->kind);
        if (!workbench_open(path,e->kind)) {
            beam_copy(status,beam_str(MSG_WB_LAUNCH_FAILED),sizeof(status));
            render(); return;
        }
        hide_window();
        return;
    }
    if (args_item>=0) {
        /* Second step of "Run with arguments": run it in a console. */
        if (args_item>=item_count || strchr(query,'\n') ||
            strlen(items[args_item].path)+strlen(query)+4>=sizeof(command)) {
            args_item=-1; search_items(); render(); return;
        }
        sprintf(command,"\"%s\" %s",items[args_item].path,query);
        args_item=-1;
        if (beam_system(command,NULL,"CON:20/20/600/150/Beam68/AUTO/CLOSE/WAIT",1)==-1) {
            beam_copy(status,beam_str(MSG_SHELL_LAUNCH_FAILED),sizeof(status));
            query[0]=0; query_cursor=0; search_items(); render(); return;
        }
        hide_window();
        return;
    }
    if (completions.count) { accept_completion();render();return; }
    if (restart_kind) {
        if (restart_confirm!=restart_kind) {
            restart_confirm=restart_kind;
            status[0]=0;
            render();
            return;
        }
        restart_confirm=0;
        if (restart_kind==BEAM_RESTART_WORKBENCH) {
            hide_window(); /* release the public-screen lock before closing it */
            if (!CloseWorkBench()) {
                show_window();
                beam_copy(status,
                    beam_str(MSG_WB_CLOSE_FIRST),
                    sizeof(status));
                render();
                return;
            }
            Delay(10);
            if (!OpenWorkBench()) {
                /* A second attempt covers a Workbench task still finishing. */
                Delay(25);
                OpenWorkBench();
            }
            beam_query_history_record(&query_history,"restart workbench");
            beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
            beam_copy(shell_notice,beam_str(MSG_WB_RESTARTED),sizeof(shell_notice));
            return;
        }
        /* Persist bounded learned state before an immediate cold reboot. */
        beam_query_history_record(&query_history,"restart amiga");
        beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
        if (usage.pending) beam_history_save(&usage,BEAM_HISTORY_PATH);
        if (plugin_health.pending)
            beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
        if (window_states_dirty) window_states_save(BEAM_WINDOW_STATE_ENVARC);
        /* A deliberate reboot is not a crash. */
        DeleteFile((STRPTR)BEAM_RECOVERY_MARKER);
        recovery_marker_owned=0;
        hide_window();
        /* FFS writes bitmap and header blocks with a short delay; rebooting
           at once can leave SYS: needing validation. */
        Delay(100);
        ColdReboot();
        return; /* defensive: ColdReboot normally never returns */
    }
    if (query_plugin()) { execute_plugin(query_plugin()); return; }
    if (beam_shell_query(query,&shell_command)) {
        if (!cfg.shell_commands) {
            beam_copy(status,beam_str(MSG_SHELL_DISABLED),sizeof(status));
            render(); return;
        }
        if (!beam_shell_command_copy(query,shell_text,sizeof(shell_text)) ||
            !*shell_text) {
            beam_copy(status,beam_str(MSG_SHELL_EMPTY),sizeof(status));
            render(); return;
        }
        if (beam_system(shell_text,NULL,
                "CON:20/20/600/150/Beam68 Shell/AUTO/CLOSE/WAIT",1)==-1) {
            beam_copy(status,beam_str(MSG_SHELL_FAILED),sizeof(status));
            render(); return;
        }
        beam_copy(last_shell_command,shell_text,sizeof(last_shell_command));
        beam_query_history_record(&query_history,query);
        if (query_history.pending>=4)
            beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
        beam_copy(shell_notice,
            beam_str(MSG_SHELL_STARTED),sizeof(shell_notice));
        hide_window(); return;
    }
    if (!result_count || selected>=result_count) return;
    it=&items[results[selected]];
    if (action_mode==BEAM_ACTION_DRAWER) {
        char parent[BEAM_MAX_PATH];
        if (!beam_parent_drawer(it->path,parent,sizeof(parent)) ||
            (!(cfg.dopus_drawers && dopus_show(parent)) &&
             !workbench_open(parent,BEAM_ITEM_DRAWER))) {
            beam_copy(status,WorkbenchBase && WorkbenchBase->lib_Version>=44?
                beam_str(MSG_DRAWER_OPEN_FAILED):
                beam_str(MSG_DRAWER_NEEDS_V44),sizeof(status));
            render(); return;
        }
        hide_window();
        if (query[0]) beam_query_history_record(&query_history,query);
        return; /* opening a drawer does not count as an application launch */
    }
    if (action_mode==BEAM_ACTION_ARGS) {
        if (it->kind!=BEAM_ITEM_SHELL && it->kind!=BEAM_ITEM_WORKBENCH &&
            it->kind!=BEAM_ITEM_FILE) {
            beam_copy(status,beam_str(MSG_ARGS_NEEDS_PROGRAM),sizeof(status));
            render(); return;
        }
        if (!cfg.shell_commands) {
            beam_copy(status,beam_str(MSG_SHELL_DISABLED),sizeof(status));
            render(); return;
        }
        args_item=results[selected];
        query[0]=0; query_cursor=0; query_view_start=0;
        sprintf(status,beam_str(MSG_ARGS_PROMPT),it->name);
        search_items(); render(); return;
    }
    if (action_mode==BEAM_ACTION_COPY) {
        beam_copy(status,beam_str(beam_clip_write(it->path)?MSG_COPIED:MSG_COPY_FAILED),
                  sizeof(status));
        render(); return;
    }
    if (action_mode==BEAM_ACTION_DOPUS) {
        char parent[BEAM_MAX_PATH];
        const char *drawer=it->path;
        if (it->kind!=BEAM_ITEM_DRAWER && it->kind!=BEAM_ITEM_VOLUME) {
            if (!beam_parent_drawer(it->path,parent,sizeof(parent))) return;
            drawer=parent;
        }
        if (!dopus_show_select(drawer,drawer==it->path?NULL:
                               (const char *)FilePart((STRPTR)it->path))) {
            beam_copy(status,beam_str(MSG_DOPUS_FAILED),sizeof(status));
            render(); return;
        }
        hide_window(); return;
    }
    if (action_mode==BEAM_ACTION_INFO) {
        /* Workbench's own Information window (workbench.library V39+). */
        char parent[BEAM_MAX_PATH];
        const char *name="";
        BPTR lock=0;
        struct Screen *wb;
        int ok=0;
        if (WorkbenchBase && WorkbenchBase->lib_Version>=39) {
            if (it->kind==BEAM_ITEM_VOLUME) lock=Lock((STRPTR)it->path,ACCESS_READ);
            else if (beam_parent_drawer(it->path,parent,sizeof(parent))) {
                lock=Lock((STRPTR)parent,ACCESS_READ);
                name=(const char *)FilePart((STRPTR)it->path);
            }
        }
        if (lock) {
            hide_window();
            wb=LockPubScreen((STRPTR)"Workbench");
            ok=WBInfo(lock,(STRPTR)name,wb)!=0;
            if (wb) UnlockPubScreen(NULL,wb);
            UnLock(lock);
        }
        if (!ok) {
            if (!window) show_window();
            beam_copy(status,beam_str(MSG_INFO_FAILED),sizeof(status));
            render();
        }
        return;
    }
    if (action_mode==BEAM_ACTION_CUSTOM) {
        const char *template_text=beam_alias_action_command(&aliases,(int)it->alias_index);
        if (!template_text || !beam_action_expand(template_text,it->path,
                                                   command,sizeof(command))) {
            beam_copy(status,beam_str(MSG_ACTION_INVALID),sizeof(status));
            render(); return;
        }
        rc=beam_system(command,NULL,NULL,1);
        if (rc==-1) {
            beam_copy(status,beam_str(MSG_ACTION_FAILED),sizeof(status));
            render(); return;
        }
        hide_window();
        if (query[0]) beam_query_history_record(&query_history,query);
        return;
    }
    if (it->kind==BEAM_ITEM_SNIPPET) {
        beam_copy(status,beam_str(it->title && beam_clip_write(it->title)?MSG_SNIPPET_COPIED:
                  MSG_COPY_FAILED),sizeof(status));
        render(); return;
    }
    if (it->kind==BEAM_ITEM_SCRIPT || it->kind==BEAM_ITEM_MODULE || it->kind==BEAM_ITEM_BOOKMARK) {
        int ok=0;
        if (it->kind==BEAM_ITEM_SCRIPT) {
            int rexx;
            Forbid(); rexx=FindPort((STRPTR)"REXX")!=NULL; Permit();
            if (!rexx) { beam_copy(status,beam_str(MSG_REXX_NOT_RUNNING),sizeof(status)); render(); return; }
            ok=strlen(it->path)+8<sizeof(command) && !strchr(it->path,'"');
            if (ok) {
                sprintf(command,"RX \"%s\"",it->path);
                ok=beam_system(command,NULL,"CON:20/20/600/150/Beam68 ARexx/AUTO/CLOSE/WAIT",1)!=-1;
            }
        } else if (it->kind==BEAM_ITEM_MODULE) {
            /* A running player gets the module over ARexx (no second copy);
               otherwise MODULE_COMMAND starts one. */
            int running=0;
            if (cfg.module_port[0] && cfg.module_arexx[0]) {
                Forbid(); running=FindPort((STRPTR)cfg.module_port)!=NULL; Permit();
            }
            if (running)
                ok=beam_template_fill(cfg.module_arexx,"{file}",it->path,command,sizeof(command)) &&
                   arexx_send(cfg.module_port,command,NULL,0);
            if (!running)
                ok=beam_template_fill(cfg.module_command,"{file}",it->path,command,sizeof(command)) &&
                   beam_system(command,NULL,NULL,1)!=-1;
        }
        else
            ok=beam_template_fill(cfg.url_command,"{url}",it->path,command,sizeof(command)) &&
               beam_system(command,NULL,NULL,1)!=-1;
        if (!ok) {
            beam_copy(status,beam_str(MSG_SOURCE_LAUNCH_FAILED),sizeof(status));
            render(); return;
        }
        if (cfg.history) {
            beam_history_record(&usage,it->path);
            if (usage.pending>=4) beam_history_save(&usage,BEAM_HISTORY_PATH);
            bind_history_meta();
        }
        hide_window();
        return;
    }
    if (it->kind==BEAM_ITEM_GAME && kick_confirm_item!=results[selected]) {
        /* Warn before WHDLoad fails on a missing Kickstart image; a second
           Enter starts the game anyway (the image may live elsewhere). */
        char slave[BEAM_MAX_PATH],kick[BEAM_MAX_PATH];
        struct BeamSlaveInfo s;
        if (game_slave_path(it->path,slave,sizeof(slave)) && read_slave(slave,&s) &&
            s.kick[0] && strlen(s.kick)+20<sizeof(kick)) {
            BPTR lock;
            sprintf(kick,"DEVS:Kickstarts/%s",s.kick);
            lock=Lock((STRPTR)kick,ACCESS_READ);
            if (lock) UnLock(lock);
            else {
                kick_confirm_item=results[selected];
                sprintf(status,beam_str(MSG_KICK_MISSING),s.kick);
                render(); return;
            }
        }
    }
    kick_confirm_item=-1;
    if (it->kind==BEAM_ITEM_GAME && has_suffix_ci(it->path,".slave")) {
        /* Icon-less slave: run WHDLoad with the game drawer as data path. */
        char drawer[BEAM_MAX_PATH];
        if (!beam_parent_drawer(it->path,drawer,sizeof(drawer)) ||
            strchr(cfg.whdload_command,'"') ||
            strlen(cfg.whdload_command)+strlen(it->path)+strlen(drawer)+32>=sizeof(command)) {
            beam_copy(status,beam_str(MSG_UNSAFE_PATH),sizeof(status));
            render(); return;
        }
        sprintf(command,"\"%s\" SLAVE=\"%s\" DATA=\"%s\" PRELOAD",
                cfg.whdload_command,it->path,drawer);
        if (beam_system(command,NULL,NULL,1)==-1) {
            beam_copy(status,beam_str(MSG_SHELL_LAUNCH_FAILED),sizeof(status)); render(); return;
        }
    } else if (it->kind!=BEAM_ITEM_SHELL) {
        if ((it->kind==BEAM_ITEM_DRAWER || it->kind==BEAM_ITEM_VOLUME) &&
            cfg.dopus_drawers && dopus_show(it->path)) {
            hide_window(); return;
        }
        record_document(it->path,it->kind);
        if (!workbench_open(it->path,it->kind)) {
            beam_copy(status,
                (WorkbenchBase && WorkbenchBase->lib_Version>=44) ||
                it->kind==BEAM_ITEM_WORKBENCH || it->kind==BEAM_ITEM_GAME?
                beam_str(MSG_WB_LAUNCH_FAILED):
                beam_str(MSG_OPEN_NEEDS_V44),sizeof(status));
            render(); return;
        }
    } else {
        if (strchr(it->path,'"') || strchr(it->path,'*')) {
            beam_copy(status,beam_str(MSG_UNSAFE_PATH),sizeof(status));
            render(); return;
        }
        sprintf(command,"\"%s\"",it->path);
        rc=beam_system(command,NULL,NULL,1);
        if (rc==-1) { beam_copy(status,beam_str(MSG_SHELL_LAUNCH_FAILED),sizeof(status)); render(); return; }
    }
    /* Only successful launches are learned. Writes are batched to keep
       floppy/flash storage quiet; pending history is also saved at exit. */
    if (cfg.history) {
        beam_history_record(&usage,it->path);
        if (usage.pending>=4) beam_history_save(&usage,BEAM_HISTORY_PATH);
        bind_history_meta();
    }
    if (query[0]) {
        beam_query_history_record(&query_history,query);
        if (query_history.pending>=4)
            beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
    }
    hide_window();
}

static void launch_selected(void);
/* Start favorite number n (1..9, the pin order) without a search. */
static void quick_launch(int n)
{
    int i;
    for (i=0;i<item_count;i++) if (items[i].favorite==(unsigned char)(n-1)) break;
    if (i==item_count) {
        if (window) { beam_copy(status,beam_str(MSG_QUICK_NONE),sizeof(status)); render(); }
        else DisplayBeep(NULL);
        return;
    }
    args_item=-1;
    query[0]=0; query_cursor=0;
    results[0]=i; result_count=1; selected=0; action_mode=BEAM_ACTION_LAUNCH;
    launch_selected();
}

/* Toggle a favorite without requiring Preferences. Persistent changes
 * go to ENVARC and the current ENV copy, both human-editable text. */
static void toggle_favorite(void)
{
    struct BeamItem *item;
    int added,active_ok;
    if (help_mode || browse_active || args_item>=0 || rows_active || ext_active ||
        beam_search_scope_parse(query,NULL)==BEAM_SCOPE_PLUGIN ||
        query_plugin() || selected<0 || selected>=result_count) return;
    item=&items[results[selected]];
    added=beam_favorites_toggle(&favorites,item->path);
    if (added<0) {
        beam_copy(status,beam_str(MSG_FAV_FULL),sizeof(status));render();return;
    }
    if (!beam_favorites_save(&favorites,BEAM_FAVS_ENVARC)) {
        beam_favorites_toggle(&favorites,item->path); /* undo */
        beam_copy(status,beam_str(MSG_FAV_SAVE_FAILED),sizeof(status));
        render(); return;
    }
    active_ok=beam_favorites_save(&favorites,BEAM_FAVS_ENV);
    bind_favorite_meta();
    search_items();
    if (!active_ok) beam_copy(status,beam_str(MSG_FAV_ENV_FAILED),sizeof(status));
    else beam_copy(status,added?beam_str(MSG_FAV_PINNED):beam_str(MSG_FAV_UNPINNED),sizeof(status));
    render();
}

static void plugin_to_launcher(struct BeamLauncherPlugin *d,
                               const struct BeamPlugin *p)
{
    beam_copy(d->name,p->name,sizeof(d->name));
    beam_copy(d->command,p->command,sizeof(d->command));
    beam_copy(d->description,p->description,sizeof(d->description));
    beam_copy(d->prefix,p->prefix,sizeof(d->prefix));
    beam_copy(d->category,p->category,sizeof(d->category));
    beam_copy(d->icon,p->icon,sizeof(d->icon));
    beam_copy(d->example,p->example,sizeof(d->example));
    beam_copy(d->help,p->help,sizeof(d->help));
    beam_copy(d->capabilities,p->capabilities,sizeof(d->capabilities));
    d->trigger=p->trigger;
    d->protocol=p->protocol;
    /* SYSTEM plugins never run without an explicit Enter. */
    d->preview=(unsigned char)(p->preview && !beam_plugin_has_capability(p,"SYSTEM"));
}

/* Plugin manifests are read on first use (first popup or ARexx query),
 * not at boot: 13+ manifest reads are noticeable on floppy and slow CF. */
static void load_plugins(void)
{
    plugins_loaded=1;
    /* ~2.9 KB manifest buffer: static, so a 4 KB Shell stack cannot overflow. */
    static struct BeamPlugin p;
    int i;
    plugin_count=0;
    plugin_load_errors=0;
    memset(plugin_state,0,sizeof(plugin_state));
    memset(plugin_trigger,0,sizeof(plugin_trigger));
    memset(plugin_manifest,0,sizeof(plugin_manifest));
    if (safe_mode) { help_build(); return; }
    for (i=0;i<cfg.plugin_file_count;i++) {
        char manifest_path[BEAM_MAX_PATH];
        int j, collision=0;
        if (!beam_plugin_enabled(&cfg,cfg.plugin_files[i])) {
            plugin_state[i]=BEAM_PLUGIN_DISABLED;
            continue;
        }
        if (beam_plugin_health_is_quarantined(&plugin_health,cfg.plugin_files[i])) {
            plugin_state[i]=BEAM_PLUGIN_QUARANTINED;
            ++plugin_load_errors;
            continue;
        }
        if (!beam_runtime_path(cfg.plugin_files[i],manifest_path,
                               sizeof(manifest_path)) ||
            !beam_plugin_read(&p,manifest_path)) {
            plugin_state[i]=BEAM_PLUGIN_MANIFEST_ERROR;
            ++plugin_load_errors;continue;
        }
        plugin_trigger[i]=p.trigger;
        /* Resolve PROGDIR: in Beam68's process, not in the child Shell.
         * Keep an exact per-manifest error code for script diagnostics. */
        {
            BPTR programlock;
            char absolute[BEAM_MAX_PATH],spec[BEAM_MAX_PATH];
            /* PLUGINDIR: is the manifest's own drawer (plugin packages);
               PROGDIR: is the launcher's drawer (legacy/bundled forms). */
            if (!beam_plugindir_resolve(manifest_path,p.command,spec,sizeof(spec)) ||
                !beam_runtime_path(spec,absolute,sizeof(absolute))) {
                plugin_state[i]=BEAM_PLUGIN_PATH_ERROR;
                ++plugin_load_errors;continue;
            }
            programlock=Lock((STRPTR)absolute,ACCESS_READ);
            if (!programlock) {
                plugin_state[i]=BEAM_PLUGIN_EXECUTABLE_MISSING;
                ++plugin_load_errors;continue;
            }
            if (!NameFromLock(programlock,absolute,sizeof(absolute))) {
                UnLock(programlock);
                plugin_state[i]=BEAM_PLUGIN_PATH_ERROR;
                ++plugin_load_errors;continue;
            }
            UnLock(programlock);
            if (strchr(absolute,'"') || strchr(absolute,'*') ||
                strchr(absolute,'\n') || strchr(absolute,'\r')) {
                plugin_state[i]=BEAM_PLUGIN_PATH_ERROR;
                ++plugin_load_errors;continue;
            }
            beam_copy(p.command,absolute,sizeof(p.command));
            if (p.icon[0]) {
                if (beam_plugindir_resolve(manifest_path,p.icon,spec,sizeof(spec)))
                    beam_copy(p.icon,spec,sizeof(p.icon));
                else p.icon[0]=0;
            }
        }
        for (j=0;j<plugin_count;j++)
            if (plugins[j].trigger==p.trigger ||
                (p.prefix[0] && plugins[j].prefix[0] &&
                 beam_equal(plugins[j].prefix,p.prefix))) collision=1;
        if (collision) {
            plugin_state[i]=BEAM_PLUGIN_CONFLICT;
            ++plugin_load_errors;
        } else {
            plugin_state[i]=BEAM_PLUGIN_OK;
            plugin_to_launcher(&plugins[plugin_count],&p);
            beam_copy(plugin_manifest[plugin_count],cfg.plugin_files[i],BEAM_MAX_PATH);
            ++plugin_count;
        }
    }
    help_build();
}

static void reload_plugins_only(void)
{
    if (plugin_job.active) {
        beam_copy(status,beam_str(MSG_PLUGINS_CANCEL_FIRST),sizeof(status));
        return;
    }
    load_plugins();
    if (safe_mode) beam_copy(status,beam_str(MSG_SAFE_PLUGINS_OFF),sizeof(status));
    else if (plugin_load_errors)
        sprintf(status,beam_str(MSG_PLUGINS_RELOADED_ERRORS),(long)plugin_load_errors);
    else sprintf(status,beam_str(MSG_PLUGINS_RELOADED),(long)plugin_count);
}

/* Preferences is its own executable, installed in SYS:Prefs or next to Beam68.
 * Start it asynchronously so the commodity keeps responding to hotkeys. */
static void open_preferences(void)
{
    static const char *candidates[]={"SYS:Prefs/Beam68Prefs", "PROGDIR:Beam68Prefs"};
    char path[BEAM_MAX_PATH], command[BEAM_MAX_PATH+4];
    BPTR lock=0;
    int i;
    for (i=0;i<2;i++) {
        lock=Lock((STRPTR)candidates[i],ACCESS_READ);
        if (lock) break;
    }
    if (!lock) { beam_copy(status,beam_str(MSG_PREFS_MISSING),sizeof(status)); render(); return; }
    if (!NameFromLock(lock,path,sizeof(path))) {
        UnLock(lock); beam_copy(status,beam_str(MSG_PREFS_PATH_LONG),sizeof(status)); render(); return;
    }
    UnLock(lock);
    if (strchr(path,'"') || strchr(path,'*') || strlen(path)+3>sizeof(command)) {
        beam_copy(status,beam_str(MSG_PREFS_PATH_INVALID),sizeof(status)); render(); return;
    }
    sprintf(command,"\"%s\"",path);
    if (beam_system(command,NULL,NULL,1)==-1) {
        beam_copy(status,beam_str(MSG_PREFS_START_FAILED),sizeof(status)); render(); return;
    }
    hide_window();
}

static int index_settings_equal(const struct BeamConfig *a,
                                const struct BeamConfig *b)
{
    int i;
    if (a->dir_count!=b->dir_count || a->excluded_dir_count!=b->excluded_dir_count ||
        a->max_items!=b->max_items || a->max_depth!=b->max_depth ||
        a->scan_icons!=b->scan_icons || a->scan_executables!=b->scan_executables ||
        a->index_files!=b->index_files || a->index_drawers!=b->index_drawers ||
        a->index_games!=b->index_games ||
        !beam_equal(a->file_patterns,b->file_patterns)) return 0;
    for (i=0;i<a->dir_count;i++) if (!beam_equal(a->dirs[i],b->dirs[i])) return 0;
    for (i=0;i<a->excluded_dir_count;i++)
        if (!beam_equal(a->excluded_dirs[i],b->excluded_dirs[i])) return 0;
    return 1;
}

/* Validate first, allocate before committing, then update the existing Cx
 * filter in place. No commodity restart required for hotkey or index size.
 * The index is kept (and only re-bound) when no indexing setting changed. */
static int reload_config(void)
{
    struct BeamConfig *next;
    struct BeamIndex new_index;
    int actual_items=0,next_memory_reduced=0,reopen_window=0,new_alloc=0,rescan;
    IX ix,previous_ix;
    /* 14 KB, allocated only for the duration of a reload. */
    next=(struct BeamConfig *)AllocVec(sizeof(*next),MEMF_ANY|MEMF_CLEAR);
    if (!next) { beam_copy(status,beam_str(MSG_RELOAD_NO_MEMORY),sizeof(status)); return 0; }
    beam_config_defaults(next);
    if (!beam_config_read_active(next) || !beam_config_valid(next)) {
        FreeVec(next);
        beam_copy(status,beam_str(MSG_CONFIG_INVALID),sizeof(status)); return 0;
    }
    if (ParseIX((STRPTR)next->hotkey,&ix)!=0) {
        FreeVec(next);
        beam_copy(status,beam_str(MSG_HOTKEY_INVALID),sizeof(status)); return 0;
    }
    memset(&new_index,0,sizeof(new_index));
    if (next->max_items!=cfg.max_items) {
        if (!allocate_index(&new_index,next->max_items,&actual_items)) {
            FreeVec(next);
            beam_copy(status,beam_str(MSG_INDEX_NO_MEMORY),sizeof(status)); return 0;
        }
        new_alloc=1;
        if (actual_items<next->max_items) {
            next->max_items=actual_items;
            next_memory_reduced=1;
        }
    }
    if (hotkey_filter) {
        SetFilterIX(hotkey_filter,&ix);
        if (CxObjError(hotkey_filter)) {
            if (ParseIX((STRPTR)cfg.hotkey,&previous_ix)==0)
                SetFilterIX(hotkey_filter,&previous_ix);
            if (new_alloc) beam_index_free(&new_index);
            FreeVec(next);
            beam_copy(status,beam_str(MSG_HOTKEY_FAILED),sizeof(status)); return 0;
        }
    }
    rescan=new_alloc || !index_settings_equal(&cfg,next);
    if (cfg.window_width!=next->window_width ||
        cfg.window_height!=next->window_height) {
        last_width=0; last_height=0; /* apply preferred size next popup */
    }
    notify_stop();
    reopen_window=window!=NULL;
    if (reopen_window) hide_window();
    scan_cancel();
    result_count=0;
    if (new_alloc) {
        if (cand_bits) { FreeVec(cand_bits); cand_bits=NULL; } /* sized by capacity */
        cand_valid=0;
        beam_index_free(&beam_index);
        beam_index=new_index;
        index_memory_reduced=next_memory_reduced;
    }
    cfg=*next;
    FreeVec(next);
    if (safe_mode) cfg.theme=BEAM_THEME_WORKBENCH;
    beam_favorites_load_active(&favorites);
    beam_aliases_load_active(&aliases);
    browse_exit();  /* browse table is sized by BROWSE_MAX */
    beam_hidden_size(&hidden,cfg.hidden_max);
    beam_recent_docs_size(&recent_docs,cfg.recent_docs_max);
    beam_hidden_load(&hidden,BEAM_HIDDEN_PATH);
    quick_filters_install();
    load_plugins();
    if (rescan) { beam_index_clear(&beam_index); ++index_generation; refresh_index(0); }
    else bind_all_meta();
    notify_start(); search_items();
    if (reopen_window) show_window();
    if (plugin_load_errors)
        sprintf(status,beam_str(MSG_SETTINGS_PLUGIN_ERRORS),
                (long)plugin_load_errors);
    else beam_copy(status,scan_active ? beam_str(MSG_SETTINGS_SCANNING) :
                   beam_str(MSG_SETTINGS_UPDATED),sizeof(status));
    return 1;
}

static int install_control_port(void)
{
    control_port=CreateMsgPort();
    if (!control_port) return 0;
    control_port->mp_Node.ln_Name=(STRPTR)BEAM_PORT_NAME;
    Forbid();
    if (FindPort((STRPTR)BEAM_PORT_NAME)) {
        Permit(); DeleteMsgPort(control_port); control_port=NULL;
        return 0; /* already running */
    }
    AddPort(control_port);
    Permit();
    return 1;
}
static void drain_control(void)
{
    struct Message *raw;
    struct BeamControlMessage *msg;
    if (!control_port) return;
    while ((raw=GetMsg(control_port))) {
        ULONG command=0;
        ULONG theme=BEAM_THEME_WORKBENCH;
        ULONG custom_colors[BEAM_THEME_COLOR_COUNT];
        struct BeamThemeControlMessage *theme_msg;
        int i;
        msg=(struct BeamControlMessage *)raw;
        if (raw->mn_Length>=sizeof(*msg) &&
            msg->magic==BEAM_CONTROL_MAGIC && msg->version==BEAM_CONTROL_VERSION) {
            command=msg->command;
            if (command==BEAM_CTL_PREVIEW_THEME &&
                raw->mn_Length>=sizeof(*theme_msg)) {
                theme_msg=(struct BeamThemeControlMessage *)raw;
                theme=theme_msg->theme;
                for (i=0;i<BEAM_THEME_COLOR_COUNT;i++)
                    custom_colors[i]=theme_msg->custom_colors[i];
            } else if (command==BEAM_CTL_PREVIEW_THEME) command=0;
            FreeVec(raw); /* validated one-way protocol: allocated by control.c */
        } else if (raw->mn_ReplyPort) {
            ReplyMsg(raw); /* don't free another sender's allocation */
        } else if (raw->mn_Length>=sizeof(*msg) && msg->magic==BEAM_CONTROL_MAGIC) {
            FreeVec(raw); /* our protocol, other version: still control.c's */
        }
        /* Anything else is neither ours to free nor repliable: leave it. */
        if (command==BEAM_CTL_RELOAD) { reload_config(); render(); }
        else if (command==BEAM_CTL_RELOAD_PLUGINS) { reload_plugins_only(); render(); }
        else if (command==BEAM_CTL_REINDEX) { scan_begin(all_roots()); beam_copy(status,beam_str(MSG_SCANNING),sizeof(status)); search_items(); render(); }
        else if (command==BEAM_CTL_SHOW) show_window();
        else if (command==BEAM_CTL_HIDE) hide_window();
        else if (command==BEAM_CTL_QUIT) quitting=1;
        else if (command==BEAM_CTL_PREVIEW_THEME && !safe_mode &&
                 theme<BEAM_THEME_COUNT) {
            cfg.theme=(int)theme;
            for (i=0;i<BEAM_THEME_COLOR_COUNT;i++)
                cfg.custom_colors[i]=custom_colors[i];
            if (window) { palette_apply(); render(); }
        }
    }
}


/* ARexx interface, independent from the older one-way control port.
 * Receive only RexxMsg packets; reply before leaving this event-loop turn.
 * No filesystem calls or blocking waits under Forbid(). */
/* Protocol-1 plugin replies arrive on the public port BEAM68_REPLY.  The
 * sender allocates with AllocVec(MEMF_PUBLIC) and never waits; we free. */
static void install_reply_port(void)
{
    reply_port=CreateMsgPort();
    if (!reply_port) return;
    reply_port->mp_Node.ln_Name=(STRPTR)BEAM68_REPLY_PORT;
    Forbid();
    if (FindPort((STRPTR)BEAM68_REPLY_PORT)) {
        Permit(); DeleteMsgPort(reply_port); reply_port=NULL; return;
    }
    AddPort(reply_port);
    Permit();
}

static void free_reply(struct Message *raw)
{
    struct Beam68ReplyMessage *m=(struct Beam68ReplyMessage *)raw;
    if (raw->mn_ReplyPort) ReplyMsg(raw);
    else if (raw->mn_Length>=sizeof(*m) && m->magic==BEAM68_REPLY_MAGIC) FreeVec(raw);
    /* anything else is not ours to free */
}

static void remove_reply_port(void)
{
    struct Message *raw;
    if (!reply_port) return;
    Forbid(); RemPort(reply_port); Permit();
    while ((raw=GetMsg(reply_port))) free_reply(raw);
    DeleteMsgPort(reply_port);
    reply_port=NULL;
}

static void drain_replies(void)
{
    struct Message *raw;
    while ((raw=GetMsg(reply_port))) {
        struct Beam68ReplyMessage *m=(struct Beam68ReplyMessage *)raw;
        if (!raw->mn_ReplyPort && raw->mn_Length>=sizeof(*m) &&
            m->magic==BEAM68_REPLY_MAGIC && m->version==BEAM68_REPLY_VERSION &&
            plugin_job.active &&
            m->serial==(unsigned long)plugin_serial && m->count>0) {
            int i,k,n=m->count>BEAM68_MAX_ROWS?BEAM68_MAX_ROWS:(int)m->count;
            int same_query=!strcmp(query,plugin_original_query);
            for (i=0;i<n;i++) {
                plugin_rows[i]=m->rows[i];
                plugin_rows[i].text[BEAM68_PLUGIN_MAX_RESULT]=0;
                plugin_rows[i].value[sizeof(plugin_rows[i].value)-1]=0;
                for (k=0;plugin_rows[i].text[k];k++)
                    if ((unsigned char)plugin_rows[i].text[k]<32) plugin_rows[i].text[k]=' ';
            }
            int was_preview=preview_job;
            beam_copy(rows_title,plugin_active_name,sizeof(rows_title));
            plugin_finish(plugin_rows[0].text,m->is_error);
            if (!was_preview && !m->is_error && same_query &&
                (n>1 || plugin_rows[0].kind!=BEAM68_ROW_TEXT)) {
                plugin_row_count=n; rows_active=1;
                sprintf(status,beam_str(MSG_ROWS_STATUS),rows_title,(long)n);
                search_items();
                if (window) render();
            }
        }
        free_reply(raw);
    }
}

static void install_rexx_port(void)
{
    if (!RexxSysBase) return;
    rexx_port=CreateMsgPort();
    if (!rexx_port) return;
    rexx_port->mp_Node.ln_Name=(STRPTR)BEAM_REXX_PORT;
    Forbid();
    if (FindPort((STRPTR)BEAM_REXX_PORT)) {
        Permit(); DeleteMsgPort(rexx_port); rexx_port=NULL; return;
    }
    AddPort(rexx_port);
    Permit();
}

/* Keep result text well below one ARexx message and avoid heap allocations
 * in normal launcher operation. Rexx CreateArgstring is used only on demand. */
static void drain_rexx(void)
{
    struct RexxMsg *rmsg;
    if (!rexx_port) return;
    while ((rmsg=(struct RexxMsg *)GetMsg(rexx_port))) {
        struct BeamRexxCommand cmd;
        char output[350];
        int rc=0;
        output[0]=0;
        if (!IsRexxMsg(rmsg)) {
            ReplyMsg((struct Message *)rmsg);
            continue;
        }
        if ((rmsg->rm_Action & RXCODEMASK)!=RXCOMM ||
            !beam_rexx_parse((const char *)rmsg->rm_Args[0],&cmd)) {
            rc=5;
            beam_copy(output,"Unknown command (HELP lists commands)",sizeof(output));
        } else {
            switch (cmd.action) {
            case BEAM_REXX_SHOW: show_window(); break;
            case BEAM_REXX_HIDE: hide_window(); break;
            case BEAM_REXX_RELOAD:
                if (!reload_config()) {
                    rc=10;beam_copy(output,status,sizeof(output));
                } else render();
                break;
            case BEAM_REXX_REINDEX:
                scan_begin(all_roots());search_items();render();
                break;
            case BEAM_REXX_RELOAD_PLUGINS:
                reload_plugins_only();render();break;
            case BEAM_REXX_QUIT: quitting=1;break;
            case BEAM_REXX_STATUS:
                sprintf(output,"version=0.1 items=%ld scanning=%ld plugins=%ld errors=%ld busy=%ld",
                    (long)item_count,(long)(scan_active?1:0),
                    (long)plugin_count,(long)plugin_load_errors,
                    (long)(plugin_job.active?1:0));
                break;
            case BEAM_REXX_INDEX:
                {
                    struct BeamScanStatus info;
                    scan_snapshot(&info);
                    if (!beam_scan_status_format(&info,output,sizeof(output))) {
                        rc=10;
                        beam_copy(output,"Could not format scan status",sizeof(output));
                    }
                }
                break;
            case BEAM_REXX_VERSION:
                beam_copy(output,"0.1",sizeof(output));break;
            case BEAM_REXX_HELP:
                beam_copy(output,
                    "SHOW HIDE RELOAD RELOADPLUGINS REINDEX QUIT STATUS INDEX VERSION PLUGINS PLUGIN <1..16> TIMING HELP",
                    sizeof(output));break;
            case BEAM_REXX_TIMING:
                sprintf(output,"search=%luus cacheload=%luus scan=%luus startup=%luus items=%ld narrowed=%lu",
                        timing_search,timing_cache,timing_scan,timing_startup,
                        (long)item_count,timing_narrowed);
                break;
            case BEAM_REXX_PLUGINS:
                ensure_plugins();
                sprintf(output,"configured=%ld loaded=%ld errors=%ld",
                    (long)cfg.plugin_file_count,(long)plugin_count,
                    (long)plugin_load_errors);
                break;
            case BEAM_REXX_PLUGIN:
                ensure_plugins();
                if (cmd.plugin_index>=cfg.plugin_file_count) {
                    rc=10;
                    beam_copy(output,"Plugin index out of range",sizeof(output));
                } else if (!beam_plugin_diagnostic_line(cmd.plugin_index+1,
                    plugin_state[cmd.plugin_index],plugin_trigger[cmd.plugin_index],
                    cfg.plugin_files[cmd.plugin_index],output,sizeof(output))) {
                    rc=10;
                    beam_copy(output,"Could not format plugin status",sizeof(output));
                }
                break;
            default: rc=5; break;
            }
        }
        rmsg->rm_Result1=(LONG)rc;
        rmsg->rm_Result2=0;
        if (output[0] && (rmsg->rm_Action & RXFF_RESULT)) {
            STRPTR str=CreateArgstring((STRPTR)output,(LONG)strlen(output));
            if (str) rmsg->rm_Result2=(LONG)str;
            else rmsg->rm_Result1=10;
        }
        ReplyMsg((struct Message *)rmsg);
    }
}

static void remove_rexx_port(void)
{
    if (rexx_port) {
        struct RexxMsg *rmsg;
        Forbid();RemPort(rexx_port);Permit();
        while ((rmsg=(struct RexxMsg *)GetMsg(rexx_port))) {
            if (IsRexxMsg(rmsg)) {
                rmsg->rm_Result1=20;
                rmsg->rm_Result2=0;
            }
            ReplyMsg((struct Message *)rmsg);
        }
        DeleteMsgPort(rexx_port);
        rexx_port=NULL;
    }
}

static void handle_window_messages(void)
{
    struct IntuiMessage *m;
    ULONG cls;
    UWORD code, qualifier;
    while (window && (m=(struct IntuiMessage *)GetMsg(window->UserPort))) {
        WORD mouse_y=m->MouseY;
        ULONG seconds=m->Seconds, micros=m->Micros;
        cls=m->Class; code=m->Code; qualifier=m->Qualifier;
        ReplyMsg((struct Message *)m);
        /* Typed-ahead text keys only queue a search; anything else (Enter,
           navigation, mouse) first needs results for the current query. */
        if (search_pending &&
            !(cls==IDCMP_VANILLAKEY && code!=13 && code!=10 && code!=27 && code!=9)) {
            search_items(); render_pending|=BEAM_RENDER_ALL;
        }
        if (cls==IDCMP_CLOSEWINDOW) { hide_window(); return; }
        if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(window); render(); EndRefresh(window,TRUE);
        } else if (cls==IDCMP_NEWSIZE) {
            last_width=window->Width; last_height=window->Height;
            render();
        } else if (cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN &&
                   (completions.count || (result_count && !query_plugin()))) {
            struct RastPort *rp=window->RPort;
            int lineheight=launcher_lineheight(rp);
            int top=window->BorderTop+lineheight+9;
            int bottom=window->Height-window->BorderBottom-lineheight-8;
            int count=completions.count?(int)completions.count:result_count;
            int visible,index;
            if (!completions.count && (help_mode || beam_search_scope_parse(query,NULL)==BEAM_SCOPE_PLUGIN))
                bottom-=lineheight;
            visible=beam_result_rows(top,lineheight,bottom,count);
            index=beam_hit_result(mouse_y,top,lineheight,bottom,visible);
            if (index>=0) {
                int dbl;
                if (!completions.count) index+=result_scroll;
                if (index>=count) continue;
                dbl=(last_clicked==index &&
                     DoubleClick(click_seconds,click_micros,seconds,micros));
                if (completions.count) completions.selected=(unsigned char)index;
                else { selected=index;if (!dbl) action_mode=BEAM_ACTION_LAUNCH; }
                last_clicked=index; click_seconds=seconds;click_micros=micros;
                if (dbl) {
                    if (completions.count) accept_completion();
                    else { launch_selected(); if (!window) return; }
                }
                render();
            }
        } else if (cls==IDCMP_RAWKEY) {
            int changed=0;
            /* Key-up events have bit 7 set. They must not repeat actions. */
            if (code&0x80) continue;
            if (code==RAW_UP || code==RAW_DOWN ||
                code==RAWKEY_PAGEUP || code==RAWKEY_PAGEDOWN ||
                code==RAWKEY_HOME || code==RAWKEY_END ||
                code==RAW_WHEEL_UP || code==RAW_WHEEL_DOWN) {
                int direction=(code==RAW_UP || code==RAWKEY_PAGEUP ||
                               code==RAWKEY_HOME || code==RAW_WHEEL_UP)?-1:1;
                if (completions.count) {
                    beam_completion_move(&completions,direction);changed=BEAM_RENDER_BODY;
                } else {
                int lineheight=launcher_lineheight(window->RPort);
                int page=beam_result_rows(window->BorderTop+lineheight+9,
                    lineheight,window->Height-window->BorderBottom-lineheight-8-
                    ((help_mode || beam_search_scope_parse(query,NULL)==BEAM_SCOPE_PLUGIN)?lineheight:0),
                    result_count);
                int next=beam_result_navigate(selected,result_count,direction,
                    code==RAWKEY_PAGEUP || code==RAWKEY_PAGEDOWN ||
                    (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))!=0,
                    code==RAWKEY_HOME || code==RAWKEY_END ||
                    (qualifier&IEQUALIFIER_CONTROL)!=0,page);
                if (next>=0 && next!=selected) {
                    selected=next; action_mode=BEAM_ACTION_LAUNCH; changed=BEAM_RENDER_BODY;
                }
                }
            } else if (code==RAW_BACKSPACE && (qualifier&IEQUALIFIER_CONTROL)) {
                if (beam_query_edit(query,sizeof(query),&query_cursor,BEAM_EDIT_CLEAR,0)) {
                    beam_query_history_end(&query_history);
                    status[0]=0;last_clicked=-1;search_items(); changed=BEAM_RENDER_ALL;
                }
            } else if (code==RAW_RIGHT && query_cursor>=(int)strlen(query) &&
                       selected>=0 && selected<result_count && args_item<0 && !rows_active &&
                       (ext_active?(ext[results[selected]].kind==BEAM_ITEM_VOLUME ||
                                    ext[results[selected]].kind==BEAM_ITEM_ASSIGN):
                        browse_active?browse[results[selected]].kind==BEAM_ITEM_DRAWER:
                        (items[results[selected]].kind==BEAM_ITEM_DRAWER ||
                         items[results[selected]].kind==BEAM_ITEM_VOLUME)) &&
                       !help_mode && !completions.count) {
                /* Right at the end of the query opens the drawer in place. */
                char path[BEAM_MAX_PATH];
                if (ext_active) {
                    beam_copy(path,ext[results[selected]].path,sizeof(path));
                    ext_exit();
                } else if (browse_active) {
                    beam_copy(path,browse_dir,sizeof(path));
                    AddPart((STRPTR)path,(STRPTR)browse[results[selected]].name,sizeof(path));
                } else beam_copy(path,items[results[selected]].path,sizeof(path));
                if (!browse_enter(path)) {
                    beam_copy(status,beam_str(MSG_BROWSE_EMPTY),sizeof(status));
                }
                search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_LEFT && browse_active && query_cursor==0) {
                browse_parent();
                if (!browse_active) { query[0]=0; status[0]=0; }
                search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_LEFT || code==RAW_RIGHT) {
                int op;
                if (code==RAW_LEFT)
                    op=(qualifier&IEQUALIFIER_CONTROL)?BEAM_EDIT_HOME:BEAM_EDIT_LEFT;
                else op=(qualifier&IEQUALIFIER_CONTROL)?BEAM_EDIT_END:BEAM_EDIT_RIGHT;
                changed=beam_query_edit(query,sizeof(query),&query_cursor,op,0)?
                        BEAM_RENDER_HEADER:0;
            } else if (code==RAW_TAB) {
                if (completions.count) accept_completion();
                else if (help_mode || beam_search_scope_parse(query,NULL)==BEAM_SCOPE_PLUGIN)
                    launch_selected();
                else if (result_count && !query_plugin()) {
                    action_mode=beam_next_action(action_mode,selected_has_custom_action());
                    if (action_mode==BEAM_ACTION_DOPUS && !dopus_running())
                        action_mode=beam_next_action(action_mode,selected_has_custom_action());
                } else beam_copy(status,beam_str(MSG_NO_COMPLETION),sizeof(status));
                changed=BEAM_RENDER_ALL;
            } else if (code==RAW_ESCAPE && rows_active) {
                rows_active=0; status[0]=0; search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_ESCAPE && browse_active) {
                browse_exit(); query[0]=0; query_cursor=0; status[0]=0;
                search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_ESCAPE && args_item>=0) {
                args_item=-1; query[0]=0; query_cursor=0; status[0]=0;
                search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_ESCAPE) {
                if (plugin_job.active) { plugin_cancel(); return; }
                if (help_mode) {
                    help_mode=0;query[0]=0;query_cursor=0;status[0]=0;
                    search_items();render();continue;
                }
                hide_window(); return;
            }
            else if (code==RAW_F1 || code==RAW_HELP) {
                help_mode=!help_mode;query[0]=0;query_cursor=0;query_view_start=0;
                status[0]=0;search_items();
                changed=BEAM_RENDER_ALL;
            } else if (code==RAW_F2) {
                /* F2 updates incrementally (results stay usable);
                   Shift+F2 rebuilds from scratch, refreshing everything
                   (e.g. changed WHDLoad titles). */
                if (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) {
                    beam_index_clear(&beam_index); ++index_generation; result_count=0;
                }
                scan_begin(all_roots());
                beam_copy(status,beam_str(MSG_SCANNING),sizeof(status));
                search_items(); changed=BEAM_RENDER_ALL;
            } else if (code==RAW_F3) {
                if (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
                    reload_plugins_only();
                else reload_config();
                changed=BEAM_RENDER_ALL;
            } else if (code==RAW_F4) { quitting=1; hide_window(); return; }
            else if (code==RAW_F5) { open_preferences(); return; }
            else if (code==RAW_F6) { toggle_favorite(); }
            else if (code==RAW_F7) {
                const char *saved=(qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))?
                    beam_query_history_newer(&query_history):
                    beam_query_history_older(&query_history);
                if (saved) {
                    beam_copy(query,saved,sizeof(query));
                    query_cursor=(int)strlen(query); query_view_start=0;
                    status[0]=0; last_clicked=-1; search_items(); changed=BEAM_RENDER_ALL;
                } else {
                    beam_copy(status,beam_str(MSG_NO_HISTORY),sizeof(status));
                    changed=BEAM_RENDER_ALL;
                }
            }
            else if (code==RAW_F8) {
                if (last_shell_command[0]) {
                    query[0]='>';
                    beam_copy(query+1,last_shell_command,BEAM_QUERY_LEN);
                    query_cursor=(int)strlen(query);
                    query_view_start=0; status[0]=0; last_clicked=-1;
                    search_items(); changed=BEAM_RENDER_ALL;
                } else {
                    beam_copy(status,beam_str(MSG_NO_SHELL_RECALL),sizeof(status));
                    changed=BEAM_RENDER_ALL;
                }
            }
            /* Backspace/Delete are normally IDCMP_VANILLAKEY when a text
               mapping exists; do not process them twice in RAWKEY. */
            if (changed && window) render_pending|=changed;
        } else if (cls==IDCMP_VANILLAKEY) {
            int op=0, edited;
            if ((code=='v' || code=='V') &&
                (qualifier&(IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))) {
                /* Amiga+V: paste the first clipboard line at the cursor. */
                char clip[BEAM_QUERY_LEN+1];
                int k;
                if (beam_clip_read(clip,sizeof(clip))) {
                    for (k=0;clip[k] && clip[k]!='\n' && clip[k]!='\r';k++)
                        if ((unsigned char)clip[k]>=32 && (unsigned char)clip[k]<127)
                            beam_query_edit(query,sizeof(query),&query_cursor,
                                            BEAM_EDIT_INSERT,clip[k]);
                    beam_query_history_end(&query_history);
                    status[0]=0; last_clicked=-1;
                    search_pending=1; render_pending|=BEAM_RENDER_ALL;
                } else DisplayBeep(pubscreen);
                continue;
            }
            if (code==127 && (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) &&
                selected>=0 && selected<result_count && !browse_active && !rows_active &&
                args_item<0 && !help_mode && !completions.count &&
                (ext_active?ext_scope==BEAM_SCOPE_HIDDEN:1)) {
                /* Shift+Del: hide this result (or, in hidden:, show it again). */
                const char *path=ext_active?ext[results[selected]].path:items[results[selected]].path;
                char copy[BEAM_MAX_PATH];
                int state,index;
                beam_copy(copy,path,sizeof(copy));
                state=beam_hidden_toggle(&hidden,copy);
                if (state>=0) beam_hidden_save(&hidden,BEAM_HIDDEN_PATH);
                index=beam_index_find(&beam_index,copy);
                if (index>=0) item_bind_meta(&items[index]);
                search_items();
                beam_copy(status,beam_str(state<0?MSG_HIDDEN_FULL:state?MSG_HIDDEN_ON:MSG_HIDDEN_OFF),
                          sizeof(status));
                render_pending|=BEAM_RENDER_ALL;
                continue;
            }
            if (code>='1' && code<='9' && (qualifier&IEQUALIFIER_CONTROL)) {
                quick_launch(code-'0');   /* Ctrl+1..9: favorite 1..9 */
                if (!window) return;
                continue;
            }
            if (code==13 || code==10) { launch_selected(); }
            else if (code==27 && rows_active) {
                rows_active=0; status[0]=0; search_items(); render(); continue;
            }
            else if (code==27 && browse_active) {
                browse_exit(); query[0]=0; query_cursor=0; status[0]=0;
                search_items(); render(); continue;
            }
            else if (code==27 && args_item>=0) {
                args_item=-1; query[0]=0; query_cursor=0; status[0]=0;
                search_items(); render(); continue;
            }
            else if (code==27) {
                if (plugin_job.active) { plugin_cancel(); return; }
                if (help_mode) {
                    help_mode=0;query[0]=0;query_cursor=0;status[0]=0;
                    search_items();render();continue;
                }
                hide_window(); return;
            }
            else if (code==9) {
                if (completions.count) { accept_completion(); render(); }
                else if (help_mode || beam_search_scope_parse(query,NULL)==BEAM_SCOPE_PLUGIN)
                    launch_selected();
                else if (result_count && !query_plugin()) {
                    action_mode=beam_next_action(action_mode,selected_has_custom_action());
                    if (action_mode==BEAM_ACTION_DOPUS && !dopus_running())
                        action_mode=beam_next_action(action_mode,selected_has_custom_action());
                    render();
                } else { beam_copy(status,beam_str(MSG_NO_COMPLETION),sizeof(status)); render(); }
            } else {
                if (code==12 || code==21 || ((code==8 || code==127) &&
                                 (qualifier&IEQUALIFIER_CONTROL)))
                    op=BEAM_EDIT_CLEAR; /* Ctrl+L/U or Ctrl+Backspace */
                else if (code==8) op=BEAM_EDIT_BACKSPACE;
                else if (code==127) op=BEAM_EDIT_DELETE;
                else if (code>=32 && code<127) op=BEAM_EDIT_INSERT;
                edited=op && beam_query_edit(query,sizeof(query),&query_cursor,op,(char)code);
                if (edited) {
                    rows_active=0;
                    schedule_preview();
                    beam_query_history_end(&query_history);
                    status[0]=0;last_clicked=-1;
                    /* Coalesce: a burst of queued keys costs one search. */
                    search_pending=1; render_pending|=BEAM_RENDER_ALL;
                }
            }
        }
    }
    if (window && (render_pending || search_pending))
        render_parts(render_pending|(search_pending?BEAM_RENDER_ALL:0));
}

/* QUICK_LAUNCH=<qualifiers> adds global "<qualifiers> 1".."9" hotkeys
 * for favorites 1..9.  Invalid descriptions are skipped. */
static void quick_filters_remove(void)
{
    int i;
    for (i=0;i<9;i++) if (quick_filters[i]) {
        DeleteCxObjAll(quick_filters[i]); quick_filters[i]=NULL;
    }
}
static void quick_filters_install(void)
{
    char desc[80];
    int i;
    quick_filters_remove();
    if (!broker || !cfg.quick_launch[0] || strlen(cfg.quick_launch)>60) return;
    for (i=0;i<9;i++) {
        CxObj *filter,*sender,*translate;
        sprintf(desc,"rawkey %s %d",cfg.quick_launch,i+1);
        filter=CxFilter((STRPTR)desc);
        if (!filter) continue;
        sender=CxSender(cx_port,QUICK_ID_BASE+i+1);
        translate=CxTranslate(NULL);
        if (sender) AttachCxObj(filter,sender);
        if (translate) AttachCxObj(filter,translate);
        if (!sender || !translate || CxObjError(filter)) { DeleteCxObjAll(filter); continue; }
        AttachCxObj(broker,filter);
        quick_filters[i]=filter;
    }
}

static int install_commodity(void)
{
    struct NewBroker nb;
    CxObj *filter, *sender, *translate;
    cx_port=CreateMsgPort();
    if (!cx_port) return 0;
    memset(&nb,0,sizeof(nb));
    nb.nb_Version=NB_VERSION;
    nb.nb_Name=(STRPTR)"Beam68";
    nb.nb_Title=(STRPTR)"Beam68 Launcher";
    nb.nb_Descr=(STRPTR)beam_str(MSG_CX_DESCRIPTION);
    nb.nb_Unique=NBU_UNIQUE|NBU_NOTIFY;
    nb.nb_Flags=COF_SHOW_HIDE;
    nb.nb_Port=cx_port;
    broker=CxBroker(&nb,NULL);
    if (!broker) return 0;
    filter=CxFilter(cfg.hotkey);
    hotkey_filter=filter;
    if (!filter) return 0;
    AttachCxObj(broker,filter);
    sender=CxSender(cx_port,HOTKEY_ID);
    if (!sender) return 0;
    AttachCxObj(filter,sender);
    translate=CxTranslate(NULL); /* Eat the hotkey; do not type it in active app. */
    if (!translate) return 0;
    AttachCxObj(filter,translate);
    if (CxObjError(broker) || CxObjError(filter)) return 0;
    quick_filters_install();
    ActivateCxObj(broker,TRUE);
    return 1;
}

static void event_loop(void)
{
    ULONG mask,cxmType; CxMsg *msg;
    int cx_enabled=1;
    while (!quitting) {
        mask=(1UL<<cx_port->mp_SigBit)|SIGBREAKF_CTRL_C;
        if (control_port) mask|=(1UL<<control_port->mp_SigBit);
        if (rexx_port) mask|=(1UL<<rexx_port->mp_SigBit);
        if (window) mask|=(1UL<<window->UserPort->mp_SigBit);
        if (timer_port && timer_pending) mask|=(1UL<<timer_port->mp_SigBit);
        if (notify_port) mask|=(1UL<<notify_port->mp_SigBit);
        if (wb_reply_port) mask|=(1UL<<wb_reply_port->mp_SigBit);
        if (reply_port) mask|=(1UL<<reply_port->mp_SigBit);
        if (outgoing_rexx.reply) mask|=(1UL<<outgoing_rexx.reply->mp_SigBit);
        mask=Wait(mask);
        if (mask&SIGBREAKF_CTRL_C) quitting=1;
        /* Always drain commodity port: it may include commands from Exchange. */
        while ((msg=(CxMsg *)GetMsg(cx_port))) {
            cxmType=CxMsgType(msg);
            if (cxmType==CXM_IEVENT && CxMsgID(msg)==HOTKEY_ID && cx_enabled) {
                if (window) hide_window(); else show_window();
            } else if (cxmType==CXM_IEVENT && cx_enabled &&
                       CxMsgID(msg)>QUICK_ID_BASE && CxMsgID(msg)<=QUICK_ID_BASE+9) {
                quick_launch((int)(CxMsgID(msg)-QUICK_ID_BASE));
            } else if (cxmType==CXM_COMMAND) {
                switch (CxMsgID(msg)) {
                case CXCMD_KILL: quitting=1; break;
                case CXCMD_DISABLE:
                    /* Deactivate the broker so the hotkey reaches other
                       programs instead of being swallowed. */
                    cx_enabled=0; ActivateCxObj(broker,FALSE); hide_window(); break;
                case CXCMD_ENABLE: cx_enabled=1; ActivateCxObj(broker,TRUE); break;
                case CXCMD_APPEAR: if (cx_enabled) show_window(); break;
                case CXCMD_DISAPPEAR: hide_window(); break;
                default: break;
                }
            }
            ReplyMsg((struct Message *)msg);
        }
        if (control_port) drain_control();
        if (rexx_port) drain_rexx();
        if (notify_port && (mask&(1UL<<notify_port->mp_SigBit))) notify_drain();
        if (wb_reply_port) {
            struct Message *wbm;
            while ((wbm=GetMsg(wb_reply_port))) beam_wb_launch_reply(wbm);
        }
        if (reply_port) drain_replies();
        if (outgoing_rexx.reply) outgoing_rexx_poll();
        if (window) handle_window_messages();
        if (timer_port && timer_pending && (mask&(1UL<<timer_port->mp_SigBit))) {
            /* Only one outstanding request; reply must be consumed before
             * rearming the same IORequest. */
            WaitIO((struct IORequest *)timer_io);
            timer_pending=0;
            outgoing_rexx_poll();
            if (scan_active) scan_step();
            if (plugin_job.active) plugin_poll();
            if (notify_refresh_ticks && --notify_refresh_ticks==0) {
                if (scan_active) notify_refresh_ticks=BEAM_NOTIFY_DEBOUNCE_TICKS;
                else {
                    /* Rescan only the roots whose drawers changed; the
                       existing entries stay searchable meanwhile. */
                    unsigned long roots=notify_pending_roots?notify_pending_roots:all_roots();
                    notify_pending_roots=0;
                    scan_begin(roots);
                    beam_copy(status,beam_str(MSG_INDEX_UPDATING),sizeof(status));
                    if (window) render();
                }
            }
            if (preview_ticks && --preview_ticks==0) run_preview();
            if (recovery_grace_ticks && --recovery_grace_ticks==0 &&
                recovery_marker_owned) {
                /* Startup survived: the marker has done its job. */
                DeleteFile((STRPTR)BEAM_RECOVERY_MARKER);
                recovery_marker_owned=0;
            }
            arm_timer();
        }
    }
}

static void remove_legacy_cache(void)
{
    /* Schema-3 caches lived in ENVARC:, which the Startup-Sequence copies
       into RAM on every boot.  Remove them (and their ENV: copies) once. */
    static const char *legacy[]={
        BEAM_INDEX_CACHE_LEGACY_ENVARC, BEAM_INDEX_CACHE_LEGACY_ENVARC ".bak",
        BEAM_INDEX_CACHE_LEGACY_ENVARC ".new", BEAM_INDEX_CACHE_LEGACY_ENV,
        BEAM_INDEX_CACHE_LEGACY_ENV ".bak", NULL };
    int i;
    for (i=0;legacy[i];i++) {
        BPTR lock=Lock((STRPTR)legacy[i],ACCESS_READ);
        if (lock) { UnLock(lock); DeleteFile((STRPTR)legacy[i]); }
    }
}

static struct timeval startup_time;
static int beam_main(int argc,char **argv)
{
    int success=0,resident_started=0,requested_items,actual_items=0;
    const char *startup_failure="Required OS libraries could not be opened (AmigaOS 3.0+ required).";
    BPTR safe_lock=0;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    CxBase=OpenLibrary("commodities.library",39);
    IconBase=OpenLibrary("icon.library",39);
    /* Any version: V44+ (OS 3.1.4/3.5/3.9/3.2) provides OpenWorkbenchObjectA();
       older systems use the built-in WBStartup launcher (wb_launch.c). */
    WorkbenchBase=OpenLibrary("workbench.library",0);
    DiskfontBase=OpenLibrary("diskfont.library",36); /* optional */
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0); /* optional */
    beam_locale_open(); /* optional: built-in English without a catalog */
    if (!IntuitionBase || !GfxBase || !CxBase || !IconBase)
        goto cleanup;
    if (!WorkbenchBase || WorkbenchBase->lib_Version<44) {
        wb_reply_port=CreateMsgPort();
        if (!wb_reply_port) goto cleanup;
    }
    startup_failure="Invalid launcher configuration. Check ENV:Beam68/Beam68.cfg and ENVARC:Beam68/Beam68.cfg.";
    beam_config_defaults(&cfg);
    beam_config_read_active(&cfg);
    if (!beam_config_valid(&cfg)) goto cleanup;
    window_states_load();
    /* Do not mistake a second launch for a crash while the resident control
       port proves that the first instance is healthy. */
    if (beam_control_running()) { success=1; goto cleanup; }
    if (argc>1 && (beam_equal(argv[1],"SAFE") || beam_equal(argv[1],"SAFEMODE")))
        safe_mode=1;
    safe_lock=Lock((STRPTR)"ENV:Beam68/SafeMode",ACCESS_READ);
    if (safe_lock) { safe_mode=1;UnLock(safe_lock);safe_lock=0; }
    /* The marker guards only the first seconds of a run (see
       BEAM_RECOVERY_GRACE_TICKS): a crash during startup leads to one safe
       start; an ordinary power-off or reboot does not. */
    recovery_safe_mode=recovery_marker_exists();
    if (recovery_safe_mode) safe_mode=1;
    if (recovery_safe_mode) recovery_marker_owned=1;
    else recovery_marker_begin();
    recovery_grace_ticks=BEAM_RECOVERY_GRACE_TICKS;
    if (safe_mode) cfg.theme=BEAM_THEME_WORKBENCH;
    if (!beam_runtime_path(BEAM_INDEX_CACHE_PATH,index_cache_path,
                           sizeof(index_cache_path))) index_cache_path[0]=0;
    remove_legacy_cache();
    startup_failure="Insufficient memory for the index. Free memory or lower MAX_ITEMS in Beam68.cfg.";
    requested_items=cfg.max_items;
    if (!allocate_index(&beam_index,requested_items,&actual_items)) goto cleanup;
    if (actual_items<requested_items) {
        cfg.max_items=actual_items;
        index_memory_reduced=1;
    }
    beam_history_load(&usage,BEAM_HISTORY_PATH);
    beam_query_history_load(&query_history,BEAM_QUERY_HISTORY_PATH);
    beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    beam_favorites_load_active(&favorites);
    beam_aliases_load_active(&aliases);
    beam_hidden_size(&hidden,cfg.hidden_max);
    beam_recent_docs_size(&recent_docs,cfg.recent_docs_max);
    beam_hidden_load(&hidden,BEAM_HIDDEN_PATH);
    beam_recent_docs_load(&recent_docs,BEAM_RECENT_DOCS_PATH);
    startup_failure="Could not open timer.device or allocate its message port. Free memory and try again.";
    if (!init_timer()) goto cleanup;
    GetSysTime(&startup_time);
    help_build();   /* plugins are added on first use (ensure_plugins) */
    if (safe_mode)
        beam_copy(status,recovery_safe_mode?
            beam_str(MSG_RECOVERY_SAFE):
            beam_str(MSG_SAFE_MODE),sizeof(status));
    refresh_index(0);
    notify_start();
    startup_failure="Could not create the launcher control port. Check for another running copy or free memory.";
    if (!install_control_port()) goto cleanup;
    install_rexx_port();
    install_reply_port();
    startup_failure="Could not install the hotkey broker. Check HOTKEY syntax or free memory.";
    if (!install_commodity()) goto cleanup;
    success=1; resident_started=1;
    safe_mode_active_set(safe_mode);
    arm_timer(); /* recovery grace countdown */
    timing_startup=(unsigned long)micros_since(&startup_time);
    event_loop();
cleanup:
    if (!success) beam_startup_error("Beam68",startup_failure);
    if (resident_started) safe_mode_active_set(0);
    notify_stop();
    scan_cancel();
    outgoing_rexx_poll();
    outgoing_rexx_close();
    cleanup_timer();
    if (usage.pending) beam_history_save(&usage,BEAM_HISTORY_PATH);
    if (query_history.pending)
        beam_query_history_save(&query_history,BEAM_QUERY_HISTORY_PATH);
    if (plugin_health.pending)
        beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    hide_window();
    if (window_states_dirty) window_states_save(BEAM_WINDOW_STATE_ENVARC);
    if (broker) { DeleteCxObjAll(broker); broker=NULL; hotkey_filter=NULL; }
    remove_rexx_port();
    remove_reply_port();
    if (control_port) {
        Forbid(); RemPort(control_port); Permit();
        {
            struct Message *m;
            while ((m=GetMsg(control_port))) {
                struct BeamControlMessage *cm=(struct BeamControlMessage *)m;
                if (m->mn_ReplyPort) ReplyMsg(m);
                else if (m->mn_Length>=sizeof(*cm) && cm->magic==BEAM_CONTROL_MAGIC)
                    FreeVec(m);
            }
        }
        DeleteMsgPort(control_port); control_port=NULL;
    }
    if (cx_port) {
        CxMsg *m;
        while ((m=(CxMsg *)GetMsg(cx_port))) ReplyMsg((struct Message *)m);
        DeleteMsgPort(cx_port);
    }
    beam_index_free(&beam_index);
    if (cand_bits) FreeVec(cand_bits);
    beam_aliases_free(&aliases);
    beam_hidden_free(&hidden);
    beam_recent_docs_free(&recent_docs);
    if (wb_reply_port) {
        struct Message *wbm;
        while ((wbm=GetMsg(wb_reply_port))) beam_wb_launch_reply(wbm);
        if (beam_wb_launch_outstanding()) {
            /* Programs we started are still running and will reply later.
               Their code cannot be unloaded while they run, so leave the
               port allocated but silent instead of freeing it under them. */
            Forbid();
            wb_reply_port->mp_Flags=PA_IGNORE;
            Permit();
        } else DeleteMsgPort(wb_reply_port);
        wb_reply_port=NULL;
    }
    if (DiskfontBase) CloseLibrary(DiskfontBase);
    beam_locale_close();
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (IconBase) CloseLibrary(IconBase);
    if (CxBase) CloseLibrary(CxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    /* A clean exit, or a startup that failed gracefully, is not a crash. */
    if (recovery_marker_owned)
        DeleteFile((STRPTR)BEAM_RECOVERY_MARKER);
    return success ? 0 : 20;
}

int main(int argc,char **argv)
{
    return beam_run_with_stack(BEAM_STACK_MINIMUM,beam_main,argc,argv);
}
