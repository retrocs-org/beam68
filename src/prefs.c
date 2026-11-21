/* Beam68Prefs -- separate native GadTools Preferences editor for AmigaOS 3.2.
 * Uses only the OS3.x NDK and no MUI/Reaction/68020/FPU requirements.
 * Install as SYS:Prefs/Beam68Prefs; launcher can live anywhere.
 */
#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/sghooks.h>
#include <utility/hooks.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <graphics/text.h>
#include <devices/inputevent.h>
#include <stdio.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/keymap.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <dos/dos.h>
#include <proto/wb.h>
#include <proto/alib.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <string.h>
#include "beam.h"
#include "startup_error.h"
#include "launcher_ui.h"
#include "control.h"
#include "favorites.h"
#include "aliases.h"
#include "plugin_catalog.h"
#include "../Plugins/AmiHASS/amihass.h"
#include "plugin_diagnostics.h"
#include "plugin_health.h"
#include "wb_drop.h"
#include "hotkey_capture.h"
#include "appearance.h"
#include "theme_file.h"
#include "dos_compat.h"
#include "stack_guard.h"
#include "plugin_install.h"
#include "prefs_locale.h"
#include "preset.h"
#include "catalogue_window.h"
#include <libraries/locale.h>
#include <proto/locale.h>
#include "plugin_package.h"
#include <libraries/amigaguide.h>
#include <proto/amigaguide.h>

/* Version string for the AmigaDOS Version command and the GUI installer
 * (getversion).  Non-static so the compiler keeps it. */
const char beam68prefs_version_tag[]="$VER: Beam68Prefs " BEAM68_VERSION " (" BEAM68_VERSION_DATE ")";

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;
struct Library *AslBase; /* optional: manual path editing still works without ASL */
struct Library *WorkbenchBase; /* optional: drag-and-drop only */
struct Library *KeymapBase;

#define PAGE_GENERAL 0
#define PAGE_DIRS 1
#define PAGE_PLUGINS 2
#define PAGE_APPEARANCE 3
#define PAGE_FAVORITES 4
#define PAGE_ADVANCED 5
#define PAGE_ACCESSIBILITY 6
#define PAGE_EXTRAS 7
#define PAGE_SOURCES 8
#define PAGE_LAYOUT 9
#define PAGE_LAST PAGE_LAYOUT
#define G_GROUP 108
#define G_UNLIMITED 110
#define G_INDEX_VIEW 111
#define G_DETAILED_ROWS 112
static int index_settings_view;
static int settings_qa_active;
#define G_USE 2
#define G_SAVE 3
#define G_CANCEL 4
#define G_HOTKEY 5
#define G_MAXITEMS 6
#define G_VISIBLE 7
#define G_DEPTH 8
#define G_ICONS 9
#define G_EXES 10
#define G_LIST 11
#define G_ENTRY 12
#define G_ADD 13
#define G_REMOVE 14
#define G_BROWSE 15
#define G_HISTORY 16
#define G_CACHE 17
#define G_WIDTH 18
#define G_HEIGHT 19
#define G_PREVIEW 20
#define G_TOGGLE 21
#define G_DETAILS 22
#define G_SETTINGS 23
#define G_TIMEOUT 24
#define G_CAPTURE 25
#define G_SECTION_FIRST 60
#define G_SECTION_LAST 66
#define G_SECTION_EXTRAS 89
#define G_ALIAS_EDIT 32
#define G_APP_PATH 33
#define G_APP_WORDS 34
#define G_APP_ACTION 35
#define G_APP_COMMAND 36
#define G_APP_UPDATE 37
#define G_PLUGIN_UP 38
#define G_PLUGIN_DOWN 39
#define G_MAIN_START 40
#define G_MAIN_RESTART 41
#define G_MAIN_STOP 42
#define G_SHELL_COMMANDS 43
#define G_MAIN_STATUS 44
#define G_THEME_DESCRIPTION 45
#define G_CUSTOM_COLORS 46
#define G_THEME_CYCLE 47
#define G_IMPORT_PLUGIN 49
#define G_PLUGIN_SETTINGS_BACK 50
#define G_PLUGIN_SETTING_VALUE 51
#define G_PLUGIN_SETTING_APPLY 52
#define G_PLUGIN_SETTING_NEXT 53
#define G_PLUGIN_SETTING_SAVE 54
#define G_INDEX_FILES 55
#define G_INDEX_DRAWERS 56
#define G_FILE_PATTERNS 57
#define G_EXCLUDED 58
#define G_FAVORITE_GROUP 59
#define G_FONT_NAME 67
#define G_FONT_SIZE 68
#define G_ROW_SPACING 69
#define G_HIGH_CONTRAST 70
#define G_LARGE_GADGETS 71
#define G_REDUCED_COLORS 72
#define G_COMPACT_MODE 73
#define G_THEME_PATH 74
#define G_THEME_IMPORT 75
#define G_THEME_EXPORT 76
#define G_SHELL_SERVICE 77
#define G_FONT_CHOOSE 78
#define G_INDEX_GAMES 79
#define G_QUICK_LAUNCH 80
#define G_DOPUS_PORT 81
#define G_DOPUS_DRAWERS 82
#define G_PLUGIN_CACHE 83
#define G_WHDLOAD_CMD 84
#define G_RESULTS_MAX 85
#define G_BROWSE_MAX 86
#define G_HIDDEN_MAX 87
#define G_RECENT_DOCS 88
#define G_PRESET_EXPORT 90
#define G_PRESET_IMPORT 91
#define G_RESET_PALETTE 109
#define G_EDIT_COLOURS 92
#define G_CATALOGUE 93
#define G_INDEX_MODULES 94
#define G_MODULE_PORT 95
#define G_MODULE_AREXX 96
#define G_MODULE_COMMAND 97
#define G_INDEX_REXX 98
#define G_BOOKMARK_FILES 99
#define G_BOOKMARK_BROWSE 100
#define G_URL_COMMAND 101
#define G_SECTION_SOURCES 102
#define G_ABOUT 103
#define PREF_MAIN_RESTART 100UL
#define PREF_RAW_RETURN 0x44
#define PREF_RAW_ESCAPE 0x45
#define PREF_RAW_UP 0x4c
#define PREF_RAW_DOWN 0x4d
#define PREF_RAW_LEFT 0x4f
#define PREF_RAW_RIGHT 0x4e
#define PREF_RAW_F1 0x50
#define PREF_RAW_F2 0x51
#define PREF_RAW_F7 0x56
#define PREF_RAW_F8 0x57
#define PREF_RAW_F9 0x58
#define PREF_RAW_F10 0x59
#define PREF_RAW_HELP 0x5f

static struct BeamConfig config;  /* too large for a Workbench stack */
static struct BeamConfig initial_config;
static struct BeamFavorites favorites;
static struct BeamAliases aliases;
static struct BeamPluginHealth plugin_health;
static int favorites_changed;
static int aliases_changed;
static int theme_preview_changed;
static int settings_committed;
static struct Window *win;
static struct MsgPort *app_port;
static struct AppWindow *app_window;
static struct Screen *screen;
static APTR visual;
static struct Gadget *gadgets;
static struct Gadget *g[113];
static struct BeamPlugin plugin_metadata[BEAM_MAX_PLUGINS];
static unsigned char plugin_states[BEAM_MAX_PLUGINS];
static char detail_title[128];
static unsigned int plugin_row_columns=40;
static char plugin_list_heading[96];
static char plugin_labels[BEAM_MAX_PLUGINS][BEAM_MAX_PATH+12];
static char plugin_setting_labels[BEAM_MAX_PLUGIN_SETTINGS][128];
static char plugin_setting_values[BEAM_MAX_PLUGIN_SETTINGS][64];
static struct BeamPlugin plugin_settings_meta;
static int plugin_settings_mode,plugin_settings_plugin=-1;
static char plugin_settings_path[BEAM_MAX_PATH];
static char application_labels[BEAM_ALIAS_CAP][BEAM_MAX_PATH+BEAM_ALIAS_LEN+8];
static char favorite_labels[BEAM_FAVORITES_CAP][BEAM_MAX_PATH+BEAM_FAVORITE_GROUP_LEN+6];
static char theme_labels[BEAM_THEME_COUNT][24];
static STRPTR theme_cycle_labels[BEAM_THEME_COUNT+1];
static void selected_palette(unsigned long *colors);
static void sync_palette_field(void);
static char custom_colors_text[48];
static char excluded_text[512];
static char theme_file_path[BEAM_MAX_PATH]="RAM:Beam68.theme";
static int custom_colors_valid=1;
static struct List labels;
static struct Node label_nodes[BEAM_ALIAS_CAP];
static UBYTE page=PAGE_GENERAL;
static int selection=-1;
static int done=0;
static int capturing_hotkey=0;
static int edit_active=0;
static int pending_page=-1;
static int last_launcher_running=-1;
static unsigned char launcher_state_tick=0;
static char launcher_status_text[32]="Stopped";
static STRPTR page_names[]={(STRPTR)"General",(STRPTR)"Indexing",(STRPTR)"Plugins","Appearance","Favorites","Application shortcuts","Accessibility","Extras","Sources",NULL};
static char section_button_labels[9][24];
static STRPTR compact_labels[]={(STRPTR)"Auto",(STRPTR)"Always",(STRPTR)"Never",NULL};

/* Localisation: Beam68Prefs.catalog (PROGDIR:Catalogs or LOCALE:Catalogs). */
struct LocaleBase *LocaleBase;
static struct Catalog *prefs_catalog;

const char *pstr(int id)
{
    const char *builtin=(id>=1 && id<=BEAM68PREFS_STRING_COUNT)?beam68prefs_strings[id-1]:"";
    return prefs_catalog?(const char *)GetCatalogStr(prefs_catalog,id,(STRPTR)builtin):builtin;
}

static void prefs_locale_open(void)
{
    static const int pages[]={PMSG_PAGE_GENERAL,PMSG_PAGE_INDEXING,PMSG_PAGE_PLUGINS,
        PMSG_PAGE_APPEARANCE,PMSG_PAGE_FAVORITES,PMSG_PAGE_APPLICATIONS,PMSG_PAGE_ACCESSIBILITY,
        PMSG_PAGE_EXTRAS,PMSG_PAGE_SOURCES,PMSG_PAGE_APPEARANCE};
    struct TagItem tags[2];
    int i;
    LocaleBase=(struct LocaleBase *)OpenLibrary("locale.library",38);
    if (LocaleBase) {
        tags[0].ti_Tag=OC_BuiltInLanguage; tags[0].ti_Data=(ULONG)"english";
        tags[1].ti_Tag=TAG_DONE; tags[1].ti_Data=0;
        prefs_catalog=OpenCatalogA(NULL,(STRPTR)"Beam68Prefs.catalog",tags);
    }
    for (i=0;i<10;i++) page_names[i]=(STRPTR)pstr(pages[i]);
    compact_labels[0]=(STRPTR)pstr(PMSG_COMPACT_AUTO);
    compact_labels[1]=(STRPTR)pstr(PMSG_COMPACT_ALWAYS);
    compact_labels[2]=(STRPTR)pstr(PMSG_COMPACT_NEVER);
}

static void prefs_locale_close(void)
{
    if (prefs_catalog) { CloseCatalog(prefs_catalog); prefs_catalog=NULL; }
    if (LocaleBase) { CloseLibrary((struct Library *)LocaleBase); LocaleBase=NULL; }
}
static struct TextAttr topaz={(STRPTR)"topaz.font",8,FS_NORMAL,FPF_ROMFONT};

/* Availability probes are local; only the explicit Test connection uses IPC. */
static void dependency_setting(const struct BeamPlugin *p,const char *key,const char *fallback,char *out,unsigned int cap)
{
    BPTR f;char line[300];unsigned int i;
    beam_copy(out,fallback,cap);
    for(i=0;i<p->setting_count;i++)if(beam_equal(p->setting_defs[i].key,key))
        beam_copy(out,p->setting_defs[i].default_value,cap);
    if(!p->settings[0])return;
    f=Open((STRPTR)p->settings,MODE_OLDFILE);if(!f)return;
    while(FGets(f,line,sizeof(line))){char *a=beam_trim(line),*b=strchr(a,'=');
        if(b){*b++=0;if(beam_equal(beam_trim(a),key))beam_copy(out,beam_trim(b),cap);}}
    Close(f);
}
static int dependency_port(const char *name)
{int found;Forbid();found=FindPort((STRPTR)name)!=NULL;Permit();return found;}
static int dependency_file(const char *name)
{
    char path[BEAM_MAX_PATH];BPTR l;
    if(!name || !*name)return 0;
    if(strchr(name,':'))beam_copy(path,name,sizeof(path));
    else {if(strlen(name)+3>=sizeof(path))return 0;sprintf(path,"C:%s",name);}
    l=Lock((STRPTR)path,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;
}
static void plugin_readiness(const struct BeamPlugin *p,char *out,unsigned int cap)
{
    struct Library *network;char setting[BEAM_MAX_PATH];
    const char *message="Ready: local dependencies available";
    int wiki=beam_equal(p->id,"org.beam68.wikipedia");
    if(wiki || beam_equal(p->id,"org.beam68.aminetbrowser")){
        network=OpenLibrary("bsdsocket.library",4);
        if(!network)message="Missing TCP/IP: start your network stack";
        else {CloseLibrary(network);
            if(wiki){dependency_setting(p,"CURL","C:curl",setting,sizeof(setting));
                if(!*setting)message="Not configured: select an HTTPS downloader in Settings";
                else if(!dependency_file(setting))message="Missing curl: check HTTPS downloader in Settings";
                else message="Ready: TCP/IP and downloader found; server not contacted";
            }else message="Ready: TCP/IP available; Aminet not contacted";
        }
    }else if(beam_equal(p->id,"org.beam68.amihass"))
        message=dependency_port(AMIHASS_PORT)?"Ready: service running; use Test connection for server status":"Service unavailable: start and configure AmiHASS";
    else if(beam_equal(p->id,"org.beam68.dopus")){
        dependency_setting(p,"PORT","DOPUS.1",setting,sizeof(setting));
        message=!*setting?"Not configured: set the DOpus ARexx port":
            dependency_port(setting)?"Ready: DOpus ARexx port found":"Service unavailable: start DOpus or check its ARexx port";
    }else if(beam_equal(p->id,"org.beam68.whdload")){
        dependency_setting(p,"COMMAND","WHDLoad",setting,sizeof(setting));
        message=dependency_file(setting)?"Ready: WHDLoad executable found":"Missing WHDLoad: set its executable in Settings";
    }else if(beam_equal(p->id,"org.beam68.arexxrunner"))
        message=dependency_port("REXX")?"Ready: RexxMast running":"Service unavailable: start RexxMast";
    else if(beam_equal(p->id,"org.beam68.musiccontrols"))
        message=dependency_port("HIPPOPLAYER") || dependency_port("AMIGAMP") || dependency_port("EAGLEPLAYER") || dependency_port("LUMIAMP")?
            "Ready: a supported player port is present":"Service unavailable: start a supported music player";
    else if(beam_equal(p->id,"org.beam68.musiclibrary")){
        dependency_setting(p,"PLAYER_COMMAND","",setting,sizeof(setting));
        message=*setting?"Ready: player command configured; playback not tested":"Not configured: set the player command in Settings";
    }
    beam_copy(out,message,cap);
}
static void plugin_test_status(const struct BeamPlugin *p,char *result,unsigned int capacity)
{
    plugin_readiness(p,result,capacity);
    if(beam_equal(p->id,"org.beam68.amihass")){
        struct AmiHASSClient client;struct AmiHASSMessage *reply=NULL;int ticks=0;
        memset(&client,0,sizeof(client));
        if(amihass_send(&client,AMIHASS_STATUS,"Beam68Prefs",NULL,NULL,0,0,NULL,NULL)){
            while(ticks++<100 && !(reply=amihass_reply(&client)))Delay(1);
            if(reply){
                beam_copy(result,reply->result==AMIHASS_UNCONFIGURED?"Not configured: open AmiHASS Preferences":
                    reply->result==AMIHASS_OK && reply->status.connected?"Ready: AmiHASS reports a connected server":
                    "Service running, server disconnected: check AmiHASS settings and diagnostics",capacity);
                amihass_erase(reply,sizeof(*reply));FreeVec(reply);
            }else beam_copy(result,"Service did not reply within two seconds. No device action was sent.",capacity);
        }
        amihass_client_close(&client);
    }
}
static void plugin_connection_test(struct Window *parent)
{
    char result[240];APTR args[1];
    struct EasyStruct prompt={sizeof(struct EasyStruct),0,"Plugin readiness","%s","Close"};
    plugin_test_status(&plugin_metadata[selection],result,sizeof(result));
    args[0]=(APTR)result;EasyRequestArgs(parent,&prompt,NULL,args);
}
static void show_selected_plugin(void);
static void show_selected_application(void);
static void show_selected_appearance(void);
static void show_selected_favorite(void);
static void preview_main_theme(void);
static void draw_theme_preview(void);
static int page_list_count(void);
static void set_title(const char *text);
/* Result of the last package install/uninstall, shown after the rebuild. */
static char install_message[160];
static char discovered_specs[BEAM_MAX_PLUGINS][BEAM_MAX_PATH];
static void control_main_app(ULONG command);
static void update_main_controls(void);
static void set_launcher_transition(const char *text);
static void show_plugin_setting(void);
static char *gadget_text(int id);

static struct Gadget *add_gadget(ULONG kind,struct Gadget *previous,
    UWORD id,int left,int top,int width,int height,const char *title,ULONG flags)
{
    struct NewGadget ng;
    memset(&ng,0,sizeof(ng));
    ng.ng_LeftEdge=(WORD)left; ng.ng_TopEdge=(WORD)top;
    ng.ng_Width=(WORD)width; ng.ng_Height=(WORD)height;
    ng.ng_GadgetText=(UBYTE *)title; ng.ng_TextAttr=&topaz;
    ng.ng_GadgetID=id; ng.ng_Flags=flags; ng.ng_VisualInfo=visual;
    /* The tags used by each gadget kind are added by callers below. */
    return CreateGadget(kind,previous,&ng,TAG_DONE);
}

/* For gadget kinds with attributes we need an explicit NewGadget. */
static struct Gadget *add_tagged(ULONG kind,struct Gadget *previous,
    UWORD id,int left,int top,int width,int height,const char *title,
    ULONG flags,ULONG tag1,ULONG val1,ULONG tag2,ULONG val2)
{
    struct NewGadget ng;
    memset(&ng,0,sizeof(ng));
    ng.ng_LeftEdge=(WORD)left; ng.ng_TopEdge=(WORD)top;
    ng.ng_Width=(WORD)width; ng.ng_Height=(WORD)height;
    ng.ng_GadgetText=(UBYTE *)title; ng.ng_TextAttr=&topaz;
    ng.ng_GadgetID=id; ng.ng_Flags=flags; ng.ng_VisualInfo=visual;
    return CreateGadget(kind,previous,&ng,tag1,val1,tag2,val2,TAG_DONE);
}

static void draw_plugin_heading(void)
{
    struct RastPort *rp;
    struct TextFont *font,*old_font;
    struct DrawInfo *di;
    UBYTE pen,mode;
    if(!win || page!=PAGE_PLUGINS || plugin_settings_mode)return;
    rp=win->RPort;old_font=rp->Font;pen=rp->FgPen;mode=rp->DrawMode;
    font=OpenFont(&topaz);if(font)SetFont(rp,font);
    di=GetScreenDrawInfo(screen);
    SetAPen(rp,di && di->dri_NumPens>BEAM_PEN_TEXT?di->dri_Pens[BEAM_PEN_TEXT]:1);
    SetDrMd(rp,JAM1);
    Move(rp,192,73);Text(rp,plugin_list_heading,(LONG)strlen(plugin_list_heading));
    SetAPen(rp,pen);SetDrMd(rp,mode);SetFont(rp,old_font);
    if(di)FreeScreenDrawInfo(screen,di);
    if(font)CloseFont(font);
}

static void build_labels(void)
{
    int i,count=(page==PAGE_DIRS)?config.dir_count:
                (page==PAGE_PLUGINS)?(plugin_settings_mode?
                    (int)plugin_settings_meta.setting_count:config.plugin_file_count):
                (page==PAGE_APPEARANCE)?BEAM_THEME_COUNT:
                (page==PAGE_ADVANCED)?(int)aliases.count:(int)favorites.count;
    unsigned char claimed[128];
    NewList(&labels);
    memset(label_nodes,0,sizeof(label_nodes));
    memset(claimed,0,sizeof(claimed));
    memset(plugin_metadata,0,sizeof(plugin_metadata));
    if (page==PAGE_PLUGINS && !plugin_settings_mode)
        beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    memset(plugin_states,0,sizeof(plugin_states));
    for (i=0;i<count;i++) {
        if (page==PAGE_PLUGINS && plugin_settings_mode) {
            sprintf(plugin_setting_labels[i],"%.44s = %.56s",
                plugin_settings_meta.setting_defs[i].label,
                plugin_setting_values[i]);
        } else if (page==PAGE_PLUGINS) {
            const char *name;
            int j;
            plugin_states[i]=(unsigned char)beam_catalog_inspect(&config,i,claimed,&plugin_metadata[i]);
            if (plugin_states[i]==BEAM_PLUGIN_OK &&
                beam_plugin_health_is_quarantined(&plugin_health,config.plugin_files[i])) {
                claimed[(unsigned char)plugin_metadata[i].trigger]=0;
                plugin_states[i]=BEAM_PLUGIN_QUARANTINED;
            }
            if (plugin_states[i]==BEAM_PLUGIN_OK && plugin_metadata[i].prefix[0])
                for (j=0;j<i;j++)
                    if (plugin_states[j]==BEAM_PLUGIN_OK && plugin_metadata[j].prefix[0] &&
                        beam_equal(plugin_metadata[j].prefix,plugin_metadata[i].prefix))
                        plugin_states[i]=BEAM_PLUGIN_CONFLICT;
            name=plugin_metadata[i].name[0]?plugin_metadata[i].name:config.plugin_files[i];
            {
                char priority[8],key[2];sprintf(priority,"%d",i+1);key[0]=plugin_metadata[i].trigger?plugin_metadata[i].trigger:'-';key[1]=0;
                beam_catalog_format_row(plugin_labels[i],sizeof(plugin_labels[i]),plugin_row_columns,
                    priority,name,plugin_metadata[i].plugin_version,
                    plugin_states[i]==BEAM_PLUGIN_OK?pstr(PMSG_STATE_ON):
                    plugin_states[i]==BEAM_PLUGIN_DISABLED?pstr(PMSG_STATE_OFF):
                    plugin_states[i]==BEAM_PLUGIN_QUARANTINED?pstr(PMSG_STATE_QUAR):pstr(PMSG_STATE_ERR),key);
            }
        } else if (page==PAGE_ADVANCED) {
            sprintf(application_labels[i],"%.52s | %.40s",
                    aliases.entries[i].path,
                    aliases.entries[i].words[0]?aliases.entries[i].words:pstr(PMSG_ACTION_ONLY));
        } else if (page==PAGE_APPEARANCE) {
            sprintf(theme_labels[i],"%s",beam_theme_name(i));
            theme_cycle_labels[i]=(STRPTR)theme_labels[i];
        } else if (page==PAGE_FAVORITES) {
            sprintf(favorite_labels[i],"[%s] %s",favorites.groups[i],favorites.paths[i]);
        }
        label_nodes[i].ln_Name=(STRPTR)((page==PAGE_DIRS)?
                           config.dirs[i]:(page==PAGE_PLUGINS)?
                           (plugin_settings_mode?plugin_setting_labels[i]:plugin_labels[i]):
                           (page==PAGE_ADVANCED)?
                           application_labels[i]:(page==PAGE_APPEARANCE)?
                           theme_labels[i]:favorite_labels[i]);
        AddTail(&labels,&label_nodes[i]);
    }
    if (page==PAGE_APPEARANCE) theme_cycle_labels[count]=NULL;
}

static int build_window(void)
{
    struct Gadget *last;
    int i,w,h,top,left,right=190,content_width,launcher_running;
    int section_step,section_height;
    int button_y,entry_y,list_height,entry_width,button_x;
    if (!screen || screen->Width<626 ||
        screen->Height<(int)screen->BarHeight+176) return 0;
    /* Use a spacious two-pane layout on RTG/high-resolution Workbench, with
     * a 640x200-safe compact height for classic display modes. */
    w=screen->Width>=800?760:(screen->Width>=720?700:(int)screen->Width-6);
    if (screen->Height>=430) h=360;
    else if (screen->Height>=350) h=300;
    else if (screen->Height>=300) h=260;
    else h=(int)screen->Height-(int)screen->BarHeight-3;
    if (h<174) return 0;
    content_width=w-right-20;
    button_y=h-27;
    entry_y=h-54;
    list_height=entry_y-76;
    if (list_height<45) list_height=45;
    entry_width=content_width-221;
    button_x=right+entry_width+6;
    capturing_hotkey=0;
    edit_active=0;
    if (page==PAGE_APPEARANCE &&
        (selection<0 || selection>=BEAM_THEME_COUNT)) selection=config.theme;
    memset(g,0,sizeof(g)); gadgets=NULL;
    plugin_row_columns=(unsigned int)(content_width-130-24)/8;
    beam_catalog_format_row(plugin_list_heading,sizeof(plugin_list_heading),plugin_row_columns,
                            pstr(PMSG_COL_PRIORITY),pstr(PMSG_COL_PLUGIN),pstr(PMSG_COL_VERSION),pstr(PMSG_COL_STATE),pstr(PMSG_COL_KEY));
    build_labels();
    last=CreateContext(&gadgets);
    if (!last) return 0;
    /* Six sections; related settings use a cycle within General/Appearance. */
    section_height=(config.large_gadgets && h>=300)?21:(h>=260?17:15);
    section_step=(config.large_gadgets && h>=300)?24:(h>=260?20:16);
    for (i=0;i<6;i++) {
        int selected_page=page==PAGE_ACCESSIBILITY || page==PAGE_LAYOUT?PAGE_APPEARANCE:
            page==PAGE_EXTRAS || page==PAGE_SOURCES?PAGE_GENERAL:page;
        UWORD id=(UWORD)(G_SECTION_FIRST+i);
        sprintf(section_button_labels[i],"%c %.20s",i==selected_page?'>':' ',i==PAGE_ADVANCED?pstr(PMSG_APPLICATION_SHORTCUTS_NAV):(const char *)page_names[i]);
        last=add_gadget(BUTTON_KIND,last,id,16,40+i*section_step,145,section_height,
                        section_button_labels[i],PLACETEXT_IN);
    }
    if(page==PAGE_GENERAL || page==PAGE_EXTRAS || page==PAGE_SOURCES) {
        static STRPTR groups[]={"Startup","Behaviour","Sources",NULL};
        last=add_tagged(CYCLE_KIND,last,G_GROUP,right,23,250,15,"",0,
            GTCY_Labels,(ULONG)groups,GTCY_Active,(ULONG)(page==PAGE_GENERAL?0:page==PAGE_EXTRAS?1:2));
        g[G_GROUP]=last;
    } else if(page==PAGE_APPEARANCE || page==PAGE_ACCESSIBILITY || page==PAGE_LAYOUT) {
        static STRPTR groups[]={"Themes","Text and readability","Window layout",NULL};
        last=add_tagged(CYCLE_KIND,last,G_GROUP,right,23,290,15,"",0,
            GTCY_Labels,(ULONG)groups,GTCY_Active,(ULONG)(page==PAGE_APPEARANCE?0:page==PAGE_ACCESSIBILITY?1:2));
        g[G_GROUP]=last;
    }
    if (page==PAGE_GENERAL) {
        last=add_tagged(STRING_KIND,last,G_HOTKEY,right+85,44,
                         content_width-210,14,pstr(PMSG_HOTKEY),PLACETEXT_LEFT,
                         GTST_String,(ULONG)config.hotkey,GTST_MaxChars,95);
        g[G_HOTKEY]=last;
        last=add_gadget(BUTTON_KIND,last,G_CAPTURE,w-130,42,110,17,
                        pstr(PMSG_CAPTURE),PLACETEXT_IN);
        g[G_CAPTURE]=last;
        last=add_tagged(INTEGER_KIND,last,G_MAXITEMS,right+85,(h<230?69:75),100,14,
                        pstr(PMSG_MAX_ITEMS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.max_items,GTIN_MaxChars,4);
        g[G_MAXITEMS]=last;
        last=add_tagged(INTEGER_KIND,last,G_VISIBLE,right+85,(h<230?91:104),100,14,
                        pstr(PMSG_RESULTS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.visible,GTIN_MaxChars,2);
        g[G_VISIBLE]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_HISTORY,right+85,(h<230?106:137),22,12,
                        pstr(PMSG_RANK_FREQUENT),PLACETEXT_RIGHT,
                        GTCB_Checked,config.history,TAG_IGNORE,0); g[G_HISTORY]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_CACHE,right+280,(h<230?106:137),22,12,
                        pstr(PMSG_INDEX_CACHE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_cache,TAG_IGNORE,0); g[G_CACHE]=last;
    } else if (page==PAGE_DIRS) {
        excluded_text[0]=0;
        for (i=0;i<config.excluded_dir_count;i++) {
            if (i && strlen(excluded_text)+1<sizeof(excluded_text)) strcat(excluded_text,";");
            if (strlen(excluded_text)+strlen(config.excluded_dirs[i])<sizeof(excluded_text))
                strcat(excluded_text,config.excluded_dirs[i]);
        }
        { static STRPTR groups[]={"Directories","Scan options",NULL};
          last=add_tagged(CYCLE_KIND,last,G_INDEX_VIEW,right,23,250,15,"",0,
              GTCY_Labels,(ULONG)groups,GTCY_Active,(ULONG)index_settings_view);
          g[G_INDEX_VIEW]=last; }
        if(index_settings_view) {
        last=add_tagged(INTEGER_KIND,last,G_DEPTH,right+72,40,65,14,
                        pstr(PMSG_DEPTH),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)(config.max_depth<0?4:config.max_depth),GTIN_MaxChars,2);
        g[G_DEPTH]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_ICONS,right+300,40,22,12,
                        pstr(PMSG_ICONS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.scan_icons,TAG_IGNORE,0); g[G_ICONS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_EXES,right+300,62,22,12,
                        pstr(PMSG_EXECUTABLES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.scan_executables,TAG_IGNORE,0); g[G_EXES]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_FILES,right+72,62,22,12,
                        pstr(PMSG_FILES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_files,TAG_IGNORE,0);g[G_INDEX_FILES]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_DRAWERS,right+190,62,22,12,
                        pstr(PMSG_DRAWERS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_drawers,TAG_IGNORE,0);g[G_INDEX_DRAWERS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_UNLIMITED,right+152,40,22,12,
                        "Unlimited",PLACETEXT_RIGHT,
                        GTCB_Checked,(ULONG)(config.max_depth<0),TAG_IGNORE,0);g[G_UNLIMITED]=last;
        last=add_tagged(STRING_KIND,last,G_FILE_PATTERNS,right+110,85,
                        content_width-110,14,pstr(PMSG_FILE_PATTERNS),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.file_patterns,
                        GTST_MaxChars,BEAM_FILE_PATTERNS_LEN-1);g[G_FILE_PATTERNS]=last;
        last=add_tagged(STRING_KIND,last,G_EXCLUDED,right+110,108,
                        content_width-110,14,pstr(PMSG_EXCLUDED),PLACETEXT_LEFT,
                        GTST_String,(ULONG)excluded_text,
                        GTST_MaxChars,sizeof(excluded_text)-1);g[G_EXCLUDED]=last;
        } else {
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,55,content_width,
                        entry_y-60,
                        pstr(PMSG_DIRECTORIES),PLACETEXT_ABOVE,
                        GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0); g[G_LIST]=last;
        last=add_tagged(STRING_KIND,last,G_ENTRY,right,entry_y,entry_width,13,"",0,
                        GTST_String,(ULONG)"",GTST_MaxChars,BEAM_MAX_PATH-1); g[G_ENTRY]=last;
        last=add_gadget(BUTTON_KIND,last,G_BROWSE,button_x,entry_y-2,68,17,pstr(PMSG_BROWSE),PLACETEXT_IN); g[G_BROWSE]=last;
        last=add_gadget(BUTTON_KIND,last,G_ADD,button_x+74,entry_y-2,58,17,pstr(PMSG_ADD),PLACETEXT_IN); g[G_ADD]=last;
        last=add_gadget(BUTTON_KIND,last,G_REMOVE,button_x+136,entry_y-2,72,17,pstr(PMSG_REMOVE),PLACETEXT_IN); g[G_REMOVE]=last;
        }
    } else if (page==PAGE_PLUGINS && plugin_settings_mode) {
        last=add_tagged(TEXT_KIND,last,G_DETAILS,right,42,content_width,18,
                        pstr(PMSG_PLUGIN_SETTINGS),PLACETEXT_ABOVE,
                        GTTX_Text,(ULONG)plugin_settings_meta.name,GTTX_Border,TRUE);
        g[G_DETAILS]=last;
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,78,content_width/2,
                        list_height,pstr(PMSG_SETTINGS),PLACETEXT_ABOVE,
                        GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0);g[G_LIST]=last;
        last=add_tagged(STRING_KIND,last,G_PLUGIN_SETTING_VALUE,
                        right+content_width/2+80,82,content_width/2-80,14,
                        pstr(PMSG_VALUE),PLACETEXT_LEFT,
                        GTST_String,(ULONG)plugin_setting_values[selection<0?0:selection],
                        GTST_MaxChars,63);g[G_PLUGIN_SETTING_VALUE]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_SETTING_APPLY,
                        right+content_width/2+5,112,92,17,pstr(PMSG_APPLY_VALUE),PLACETEXT_IN);
        g[G_PLUGIN_SETTING_APPLY]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_SETTING_NEXT,
                        right+content_width/2+103,112,92,17,pstr(PMSG_NEXT_CHOICE),PLACETEXT_IN);
        g[G_PLUGIN_SETTING_NEXT]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_SETTING_SAVE,
                        right+content_width/2+5,140,92,17,pstr(PMSG_SAVE_PLUGIN),PLACETEXT_IN);
        g[G_PLUGIN_SETTING_SAVE]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_SETTINGS_BACK,
                        right+content_width/2+103,140,92,17,pstr(PMSG_BACK),PLACETEXT_IN);
        g[G_PLUGIN_SETTINGS_BACK]=last;
    } else if (page==PAGE_PLUGINS) {
        last=add_tagged(CHECKBOX_KIND,last,G_SHELL_SERVICE,right+20,(h<230?23:42),22,12,
                        pstr(PMSG_SHELL_SERVICE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.shell_commands,TAG_IGNORE,0);
        g[G_SHELL_SERVICE]=last;
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,78,content_width-130,
                        list_height-10,"",0,
                        GTLV_Labels,(ULONG)&labels,GTLV_ShowSelected,0); g[G_LIST]=last;
        last=add_gadget(BUTTON_KIND,last,G_DETAILS,w-140,(h<230?40:48),120,(h<230?13:17),pstr(PMSG_DETAILS),PLACETEXT_IN);
        g[G_DETAILS]=last;
        last=add_gadget(BUTTON_KIND,last,G_SETTINGS,w-140,(h<230?54:70),120,(h<230?13:17),pstr(PMSG_SETTINGS),PLACETEXT_IN);
        g[G_SETTINGS]=last;
        last=add_gadget(BUTTON_KIND,last,G_TOGGLE,w-140,(h<230?68:92),120,(h<230?13:17),pstr(PMSG_TOGGLE),PLACETEXT_IN);
        g[G_TOGGLE]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_UP,w-140,(h<230?82:114),120,(h<230?13:17),pstr(PMSG_UP),PLACETEXT_IN);
        g[G_PLUGIN_UP]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_DOWN,w-140,(h<230?96:136),120,(h<230?13:17),pstr(PMSG_DOWN),PLACETEXT_IN);
        g[G_PLUGIN_DOWN]=last;
        last=add_gadget(BUTTON_KIND,last,G_IMPORT_PLUGIN,w-140,(h<230?110:158),120,(h<230?13:17),pstr(PMSG_IMPORT_PLUGIN),PLACETEXT_IN);
        g[G_IMPORT_PLUGIN]=last;
        if (h>=230) {
            last=add_tagged(INTEGER_KIND,last,G_TIMEOUT,w-78,190,58,14,
                            pstr(PMSG_TIMEOUT),PLACETEXT_ABOVE,
                            GTIN_Number,(ULONG)config.plugin_timeout,GTIN_MaxChars,3);
            g[G_TIMEOUT]=last;
        }
        last=add_tagged(STRING_KIND,last,G_ENTRY,right,entry_y,entry_width,13,"",0,
                        GTST_String,(ULONG)"",GTST_MaxChars,BEAM_MAX_PATH-1); g[G_ENTRY]=last;
        last=add_gadget(BUTTON_KIND,last,G_BROWSE,button_x,entry_y-2,68,17,pstr(PMSG_BROWSE),PLACETEXT_IN); g[G_BROWSE]=last;
        last=add_gadget(BUTTON_KIND,last,G_ADD,button_x+74,entry_y-2,58,17,pstr(PMSG_ADD),PLACETEXT_IN); g[G_ADD]=last;
        last=add_tagged(BUTTON_KIND,last,G_REMOVE,button_x+136,entry_y-2,88,17,pstr(PMSG_REMOVE_PLUGIN),PLACETEXT_IN,
                        GA_Disabled,(ULONG)(selection<0 || selection>=config.plugin_file_count ||
                        beam_catalog_is_bundled(&config,selection,&plugin_metadata[selection])),TAG_IGNORE,0); g[G_REMOVE]=last;
    } else if (page==PAGE_FAVORITES) {
        last=add_tagged(STRING_KIND,last,G_FAVORITE_GROUP,right+72,44,
                        150,14,pstr(PMSG_GROUP),PLACETEXT_LEFT,
                        GTST_String,(ULONG)BEAM_FAVORITE_DEFAULT_GROUP,
                        GTST_MaxChars,BEAM_FAVORITE_GROUP_LEN-1);g[G_FAVORITE_GROUP]=last;
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,76,content_width,
                        entry_y-81,pstr(PMSG_FAVORITE_LIST),PLACETEXT_ABOVE,
                        GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0); g[G_LIST]=last;
        last=add_tagged(STRING_KIND,last,G_ENTRY,right,entry_y,entry_width,13,"",0,
                        GTST_String,(ULONG)"",GTST_MaxChars,BEAM_MAX_PATH-1); g[G_ENTRY]=last;
        last=add_gadget(BUTTON_KIND,last,G_BROWSE,button_x,entry_y-2,68,17,pstr(PMSG_BROWSE),PLACETEXT_IN); g[G_BROWSE]=last;
        last=add_gadget(BUTTON_KIND,last,G_ADD,button_x+74,entry_y-2,58,17,pstr(PMSG_ADD),PLACETEXT_IN); g[G_ADD]=last;
        last=add_gadget(BUTTON_KIND,last,G_REMOVE,button_x+136,entry_y-2,72,17,pstr(PMSG_REMOVE),PLACETEXT_IN); g[G_REMOVE]=last;
    } else if (page==PAGE_ADVANCED) {
        last=add_tagged(TEXT_KIND,last,0,right,23,content_width,12,"",0,
                        GTTX_Text,(ULONG)pstr(PMSG_APPLICATION_SHORTCUTS_HELP),TAG_IGNORE,0);
        if (h>=225) {
            int list_width=content_width/3;
            int form_left,form_width,field_x,field_width;
            if (list_width<145) list_width=145;
            form_left=right+list_width+18;
            form_width=w-20-form_left;
            field_x=form_left+78;
            field_width=form_width-78;
            last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,48,list_width,
                            button_y-55,pstr(PMSG_PAGE_APPLICATIONS),PLACETEXT_ABOVE,
                            GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0); g[G_LIST]=last;
            last=add_tagged(STRING_KIND,last,G_APP_PATH,field_x,45,
                            field_width-66,14,pstr(PMSG_PATH),PLACETEXT_LEFT,
                            GTST_String,(ULONG)"",GTST_MaxChars,BEAM_MAX_PATH-1);
            g[G_APP_PATH]=last;
            last=add_gadget(BUTTON_KIND,last,G_BROWSE,field_x+field_width-60,43,
                            60,17,pstr(PMSG_BROWSE),PLACETEXT_IN); g[G_BROWSE]=last;
            last=add_tagged(STRING_KIND,last,G_APP_WORDS,field_x,74,
                            field_width,14,pstr(PMSG_KEYWORDS),PLACETEXT_LEFT,
                            GTST_String,(ULONG)"",GTST_MaxChars,BEAM_ALIAS_LEN-1);
            g[G_APP_WORDS]=last;
            last=add_tagged(STRING_KIND,last,G_APP_ACTION,field_x,103,
                            field_width,14,pstr(PMSG_ACTION),PLACETEXT_LEFT,
                            GTST_String,(ULONG)"",GTST_MaxChars,
                            BEAM_ALIAS_ACTION_LABEL_LEN-1); g[G_APP_ACTION]=last;
            last=add_tagged(STRING_KIND,last,G_APP_COMMAND,field_x,132,
                            field_width,14,pstr(PMSG_COMMAND),PLACETEXT_LEFT,
                            GTST_String,(ULONG)"",GTST_MaxChars,
                            BEAM_ALIAS_ACTION_COMMAND_LEN-1); g[G_APP_COMMAND]=last;
            last=add_gadget(BUTTON_KIND,last,G_APP_UPDATE,field_x,157,92,17,
                            pstr(PMSG_APPLY),PLACETEXT_IN); g[G_APP_UPDATE]=last;
            last=add_gadget(BUTTON_KIND,last,G_REMOVE,field_x+100,157,75,17,
                            pstr(PMSG_REMOVE),PLACETEXT_IN); g[G_REMOVE]=last;
        } else {
            last=add_gadget(BUTTON_KIND,last,G_ALIAS_EDIT,right,55,180,19,
                            pstr(PMSG_EDIT_APPLICATIONS),PLACETEXT_IN); g[G_ALIAS_EDIT]=last;
        }
    } else if (page==PAGE_APPEARANCE) {
        { unsigned long colors[BEAM_THEME_COLOR_COUNT];selected_palette(colors);
          beam_theme_format_colors(colors,custom_colors_text,sizeof(custom_colors_text)); }
        last=add_tagged(CYCLE_KIND,last,G_THEME_CYCLE,right+55,44,175,15,
                        pstr(PMSG_THEME),PLACETEXT_LEFT,
                        GTCY_Labels,(ULONG)theme_cycle_labels,
                        GTCY_Active,(ULONG)selection); g[G_THEME_CYCLE]=last;
        last=add_gadget(BUTTON_KIND,last,G_EDIT_COLOURS,right+55,66,175,15,
                        pstr(PMSG_EDIT_COLOURS),PLACETEXT_IN); g[G_EDIT_COLOURS]=last;
        last=add_gadget(BUTTON_KIND,last,G_RESET_PALETTE,right+20,86,230,15,
                        pstr(PMSG_RESTORE_PALETTE),PLACETEXT_IN);g[G_RESET_PALETTE]=last;
        if (h>=230) {
            last=add_tagged(STRING_KIND,last,G_CUSTOM_COLORS,right+76,115,
                            content_width-76,14,pstr(PMSG_CUSTOM_PALETTE),PLACETEXT_ABOVE,
                            GTST_String,(ULONG)custom_colors_text,
                            GTST_MaxChars,sizeof(custom_colors_text)-1);
            g[G_CUSTOM_COLORS]=last;
            last=add_tagged(TEXT_KIND,last,G_THEME_DESCRIPTION,right+76,147,
                            content_width-76,18,pstr(PMSG_THEME_DETAILS),PLACETEXT_ABOVE,
                            GTTX_Text,(ULONG)beam_theme_description(selection),
                            GTTX_Border,TRUE);
            g[G_THEME_DESCRIPTION]=last;
        }
        if (h>=340) {
            last=add_tagged(STRING_KIND,last,G_THEME_PATH,right+76,183,
                            content_width-246,14,pstr(PMSG_THEME_FILE),PLACETEXT_LEFT,
                            GTST_String,(ULONG)theme_file_path,
                            GTST_MaxChars,BEAM_MAX_PATH-1);g[G_THEME_PATH]=last;
            last=add_gadget(BUTTON_KIND,last,G_THEME_IMPORT,w-180,181,72,17,
                            pstr(PMSG_IMPORT),PLACETEXT_IN);g[G_THEME_IMPORT]=last;
            last=add_gadget(BUTTON_KIND,last,G_THEME_EXPORT,w-102,181,72,17,
                            pstr(PMSG_EXPORT),PLACETEXT_IN);g[G_THEME_EXPORT]=last;
        }
        /* Resizing with the Intuition size gadget lasts for the current session.
           Save a preferred initial size here, or use 0 height for auto-sizing. */
    } else if (page==PAGE_ACCESSIBILITY) {
        last=add_tagged(STRING_KIND,last,G_FONT_NAME,right+100,48,220,14,
                        pstr(PMSG_LAUNCHER_FONT),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.font_name,
                        GTST_MaxChars,BEAM_FONT_NAME_LEN-1);g[G_FONT_NAME]=last;
        if (AslBase) {
            last=add_gadget(BUTTON_KIND,last,G_FONT_CHOOSE,right+330,46,80,17,
                            pstr(PMSG_CHOOSE),PLACETEXT_IN);g[G_FONT_CHOOSE]=last;
        }
        last=add_tagged(INTEGER_KIND,last,G_FONT_SIZE,right+100,78,70,14,
                        pstr(PMSG_FONT_SIZE),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.font_size,GTIN_MaxChars,2);g[G_FONT_SIZE]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_HIGH_CONTRAST,right+20,96,22,12,
                        pstr(PMSG_HIGH_CONTRAST),PLACETEXT_RIGHT,
                        GTCB_Checked,config.high_contrast,TAG_IGNORE,0);g[G_HIGH_CONTRAST]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_REDUCED_COLORS,right+20,118,22,12,
                        pstr(PMSG_REDUCED_COLORS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.reduced_colors,TAG_IGNORE,0);g[G_REDUCED_COLORS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_LARGE_GADGETS,right+20,(h<230?132:140),22,12,
                        pstr(PMSG_LARGE_GADGETS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.large_gadgets,TAG_IGNORE,0);g[G_LARGE_GADGETS]=last;
    } else if (page==PAGE_LAYOUT) {
        last=add_tagged(INTEGER_KIND,last,G_WIDTH,right+170,44,100,14,pstr(PMSG_WINDOW_WIDTH),PLACETEXT_LEFT,
            GTIN_Number,(ULONG)config.window_width,GTIN_MaxChars,4);g[G_WIDTH]=last;
        last=add_tagged(INTEGER_KIND,last,G_HEIGHT,right+170,68,100,14,pstr(PMSG_WINDOW_HEIGHT),PLACETEXT_LEFT,
            GTIN_Number,(ULONG)config.window_height,GTIN_MaxChars,4);g[G_HEIGHT]=last;
        last=add_tagged(INTEGER_KIND,last,G_ROW_SPACING,right+170,92,70,14,pstr(PMSG_ROW_SPACING),PLACETEXT_LEFT,
            GTIN_Number,(ULONG)config.row_spacing,GTIN_MaxChars,2);g[G_ROW_SPACING]=last;
        last=add_tagged(CYCLE_KIND,last,G_COMPACT_MODE,right+170,116,125,15,pstr(PMSG_COMPACT),PLACETEXT_LEFT,
            GTCY_Labels,(ULONG)compact_labels,GTCY_Active,(ULONG)config.compact_mode);g[G_COMPACT_MODE]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_DETAILED_ROWS,right+225,(h<230?132:140),22,12,pstr(PMSG_DETAILED_ROWS),PLACETEXT_RIGHT,
            GTCB_Checked,config.detailed_rows,TAG_IGNORE,0);g[G_DETAILED_ROWS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_PREVIEW,right+20,(h<230?132:140),22,12,pstr(PMSG_ICON_PREVIEW),PLACETEXT_RIGHT,
            GTCB_Checked,config.icon_preview,TAG_IGNORE,0);g[G_PREVIEW]=last;
    } else if (page==PAGE_SOURCES) {
        /* Index sources and how their results are opened (15-pixel rows
           so the page also fits a 640x200 screen). */
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_REXX,right+20,44,22,12,
                        pstr(PMSG_SRC_REXX),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_rexx,TAG_IGNORE,0);g[G_INDEX_REXX]=last;
        last=add_tagged(STRING_KIND,last,G_BOOKMARK_FILES,right+150,64,content_width-240,13,
                        pstr(PMSG_SRC_BOOKMARKS),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.bookmark_files,
                        GTST_MaxChars,sizeof(config.bookmark_files)-1);g[G_BOOKMARK_FILES]=last;
        if (AslBase) {
            last=add_gadget(BUTTON_KIND,last,G_BOOKMARK_BROWSE,right+content_width-84,63,
                            64,15,pstr(PMSG_BROWSE),PLACETEXT_IN);g[G_BOOKMARK_BROWSE]=last;
        }
        last=add_tagged(STRING_KIND,last,G_URL_COMMAND,right+150,84,content_width-170,13,
                        pstr(PMSG_SRC_URL_COMMAND),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.url_command,
                        GTST_MaxChars,sizeof(config.url_command)-1);g[G_URL_COMMAND]=last;
    } else if (page==PAGE_EXTRAS) {
        /* Settings that used to be configuration-file only. */
        last=add_tagged(STRING_KIND,last,G_QUICK_LAUNCH,right+150,46,120,14,
                        pstr(PMSG_QUICK_LAUNCH),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.quick_launch,
                        GTST_MaxChars,sizeof(config.quick_launch)-1);g[G_QUICK_LAUNCH]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_PLUGIN_CACHE,right+20,64,22,12,
                        pstr(PMSG_PLUGIN_CACHE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.plugin_cache,TAG_IGNORE,0);g[G_PLUGIN_CACHE]=last;
        last=add_tagged(INTEGER_KIND,last,G_RESULTS_MAX,right+150,88,50,14,
                        pstr(PMSG_RESULTS_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.results_max,GTIN_MaxChars,2);g[G_RESULTS_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_BROWSE_MAX,right+330,88,50,14,
                        pstr(PMSG_BROWSE_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.browse_max,GTIN_MaxChars,3);g[G_BROWSE_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_HIDDEN_MAX,right+150,108,50,14,
                        pstr(PMSG_HIDDEN_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.hidden_max,GTIN_MaxChars,3);g[G_HIDDEN_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_RECENT_DOCS,right+330,108,50,14,
                        pstr(PMSG_RECENT_DOCS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.recent_docs_max,GTIN_MaxChars,2);g[G_RECENT_DOCS]=last;

    }
    if (page==PAGE_GENERAL) {
        launcher_running=beam_control_running();
        beam_copy(launcher_status_text,launcher_running?pstr(PMSG_RUNNING):pstr(PMSG_STOPPED),
                  sizeof(launcher_status_text));
        last_launcher_running=launcher_running;
        launcher_state_tick=0;
        last=add_tagged(TEXT_KIND,last,G_MAIN_STATUS,right+85,120,150,14,
                        pstr(PMSG_LAUNCHER),PLACETEXT_LEFT,
                        GTTX_Text,(ULONG)launcher_status_text,
                        GTTX_Border,TRUE);
        g[G_MAIN_STATUS]=last;
        last=add_tagged(BUTTON_KIND,last,G_MAIN_START,right,(h<230?button_y-24:button_y),56,17,
                        pstr(PMSG_START),PLACETEXT_IN,GA_Disabled,launcher_running,TAG_IGNORE,0);
        g[G_MAIN_START]=last;
        last=add_tagged(BUTTON_KIND,last,G_MAIN_RESTART,right+60,(h<230?button_y-24:button_y),76,17,
                        pstr(PMSG_RESTART),PLACETEXT_IN,GA_Disabled,!launcher_running,TAG_IGNORE,0);
        g[G_MAIN_RESTART]=last;
        last=add_tagged(BUTTON_KIND,last,G_MAIN_STOP,right+140,(h<230?button_y-24:button_y),56,17,
                        pstr(PMSG_STOP),PLACETEXT_IN,GA_Disabled,!launcher_running,TAG_IGNORE,0);
        g[G_MAIN_STOP]=last;
    }
    last=add_gadget(BUTTON_KIND,last,G_ABOUT,16,button_y,145,17,
                    pstr(PMSG_ABOUT_BUTTON),PLACETEXT_IN);g[G_ABOUT]=last;
    last=add_gadget(BUTTON_KIND,last,G_USE,w-275,button_y,80,17,pstr(PMSG_USE),PLACETEXT_IN); g[G_USE]=last;
    last=add_gadget(BUTTON_KIND,last,G_SAVE,w-190,button_y,80,17,pstr(PMSG_SAVE),PLACETEXT_IN); g[G_SAVE]=last;
    last=add_gadget(BUTTON_KIND,last,G_CANCEL,w-105,button_y,85,17,pstr(PMSG_CANCEL),PLACETEXT_IN); g[G_CANCEL]=last;
    if (!last) { FreeGadgets(gadgets); gadgets=NULL; return 0; }
    left=((int)screen->Width-w)/2;
    top=((int)screen->Height-h)/2;
    if (top<(int)screen->BarHeight+1) top=(int)screen->BarHeight+1;
    win=OpenWindowTags(NULL,
        WA_PubScreen,(ULONG)screen,
        WA_Title,(ULONG)pstr(PMSG_TITLE),
        WA_Left,(ULONG)left,WA_Top,(ULONG)top,
        WA_Width,(ULONG)w,WA_Height,(ULONG)h,
        WA_Gadgets,(ULONG)gadgets,
        WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_SmartRefresh,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_MOUSEBUTTONS|
                 IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|
                 IDCMP_INTUITICKS,
        TAG_DONE);
    if (!win) { FreeGadgets(gadgets); gadgets=NULL; return 0; }
    /* Workbench AppWindow is optional: failure must not affect manual
     * Browse/Add controls, or prohibit running without Workbench loaded. */
    if (WorkbenchBase && app_port && !plugin_settings_mode &&
        (page==PAGE_DIRS || page==PAGE_PLUGINS || page==PAGE_FAVORITES ||
         page==PAGE_APPEARANCE))
        app_window=AddAppWindowA(1UL,0UL,win,app_port,NULL);
    GT_RefreshWindow(win,NULL);
    draw_plugin_heading();
    if (g[G_LIST] && page!=PAGE_APPEARANCE &&
        selection>=0 && selection<page_list_count()) {
        GT_SetGadgetAttrs(g[G_LIST],win,NULL,GTLV_Selected,(ULONG)selection,
                          GTLV_MakeVisible,(ULONG)selection,TAG_DONE);
        if (page==PAGE_ADVANCED) show_selected_application();
        else if (page==PAGE_FAVORITES) show_selected_favorite();
        else if (page==PAGE_PLUGINS) {
            if (plugin_settings_mode) show_plugin_setting();
            else show_selected_plugin();
        }
    }
    /* The list selection and description were supplied to CreateGadget.
       Only paint the custom preview after the first refresh; updating either
       gadget here can close the window on some V39 GadTools builds. */
    if(page==PAGE_DIRS && g[G_DEPTH])GT_SetGadgetAttrs(g[G_DEPTH],win,NULL,GA_Disabled,(ULONG)(config.max_depth<0),TAG_DONE);
    if (page==PAGE_APPEARANCE) draw_theme_preview();
    return 1;
}

static void close_window(void)
{
    /* Unregister the window BEFORE Intuition destroys it.  AppMessages
     * belong to Workbench: always ReplyMsg, never FreeVec or UnLock wa_Lock. */
    if (app_window) { RemoveAppWindow(app_window); app_window=NULL; }
    if (app_port) {
        struct AppMessage *pending;
        while ((pending=(struct AppMessage *)GetMsg(app_port)))
            ReplyMsg((struct Message *)pending);
    }
    if (win) {
        struct IntuiMessage *pending;
        while ((pending=GT_GetIMsg(win->UserPort))) GT_ReplyIMsg(pending);
        CloseWindow(win); win=NULL;
    }
    if (gadgets) { FreeGadgets(gadgets); gadgets=NULL; }
}
static LONG gadget_number(int id)
{
    struct StringInfo *si;
    if (!g[id]) return 0;
    si=(struct StringInfo *)g[id]->SpecialInfo;
    return si?si->LongInt:0;
}
static void collect_page(void)
{
    if (page==PAGE_GENERAL) {
        struct StringInfo *si;
        if (g[G_HOTKEY]) {
            si=(struct StringInfo *)g[G_HOTKEY]->SpecialInfo;
            if (si && si->Buffer) beam_copy(config.hotkey,(char *)si->Buffer,sizeof(config.hotkey));
        }
        config.max_items=(int)gadget_number(G_MAXITEMS);
        config.visible=(int)gadget_number(G_VISIBLE);
        if (g[G_HISTORY]) config.history=!!(g[G_HISTORY]->Flags&GFLG_SELECTED);
        if (g[G_CACHE]) config.index_cache=!!(g[G_CACHE]->Flags&GFLG_SELECTED);
        if (g[G_SHELL_COMMANDS]) config.shell_commands=!!(g[G_SHELL_COMMANDS]->Flags&GFLG_SELECTED);
    } else if (page==PAGE_PLUGINS) {
        if (g[G_TIMEOUT]) config.plugin_timeout=(int)gadget_number(G_TIMEOUT);
        if (g[G_SHELL_SERVICE])
            config.shell_commands=!!(g[G_SHELL_SERVICE]->Flags&GFLG_SELECTED);
    } else if (page==PAGE_APPEARANCE) {
        struct StringInfo *si;
        if(g[G_WIDTH])config.window_width=(int)gadget_number(G_WIDTH);
        if(g[G_HEIGHT])config.window_height=(int)gadget_number(G_HEIGHT);
        if (g[G_PREVIEW]) config.icon_preview=!!(g[G_PREVIEW]->Flags&GFLG_SELECTED);
        if (selection>=0 && selection<BEAM_THEME_COUNT) config.theme=selection;
        if (g[G_CUSTOM_COLORS]) {
            si=(struct StringInfo *)g[G_CUSTOM_COLORS]->SpecialInfo;
            unsigned long colors[BEAM_THEME_COLOR_COUNT],current[BEAM_THEME_COLOR_COUNT];
            custom_colors_valid=si && si->Buffer && beam_theme_parse_colors((char *)si->Buffer,colors);
            if(custom_colors_valid) {
                selected_palette(current);
                if(memcmp(colors,current,sizeof(colors)))beam_theme_set_palette(&config,colors);
            }
        }
        if (g[G_THEME_PATH]) {
            si=(struct StringInfo *)g[G_THEME_PATH]->SpecialInfo;
            if (si && si->Buffer)
                beam_copy(theme_file_path,beam_trim((char *)si->Buffer),sizeof(theme_file_path));
        }
    } else if (page==PAGE_DIRS) {
        struct StringInfo *si;
        char work[512],*p,*next;
        if(g[G_DEPTH]) config.max_depth=g[G_UNLIMITED] && (g[G_UNLIMITED]->Flags&GFLG_SELECTED)?-1:(int)gadget_number(G_DEPTH);
        if (g[G_ICONS]) config.scan_icons=!!(g[G_ICONS]->Flags&GFLG_SELECTED);
        if (g[G_EXES]) config.scan_executables=!!(g[G_EXES]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_FILES]) config.index_files=!!(g[G_INDEX_FILES]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_DRAWERS]) config.index_drawers=!!(g[G_INDEX_DRAWERS]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_GAMES]) config.index_games=!!(g[G_INDEX_GAMES]->Flags&GFLG_SELECTED);
        si=g[G_FILE_PATTERNS]?(struct StringInfo *)g[G_FILE_PATTERNS]->SpecialInfo:NULL;
        if (si && si->Buffer)
            beam_copy(config.file_patterns,beam_trim((char *)si->Buffer),sizeof(config.file_patterns));
        if(g[G_EXCLUDED])config.excluded_dir_count=0;
        si=g[G_EXCLUDED]?(struct StringInfo *)g[G_EXCLUDED]->SpecialInfo:NULL;
        if (si && si->Buffer) {
            beam_copy(work,(char *)si->Buffer,sizeof(work));p=work;
            while (p && *p && config.excluded_dir_count<BEAM_MAX_EXCLUDED_DIRS) {
                next=strchr(p,';');if(next)*next++=0;p=beam_trim(p);
                if (*p) beam_copy(config.excluded_dirs[config.excluded_dir_count++],p,BEAM_MAX_PATH);
                p=next;
            }
        }
    } else if (page==PAGE_ACCESSIBILITY) {
        char *text=gadget_text(G_FONT_NAME);
        if (text) beam_copy(config.font_name,beam_trim(text),sizeof(config.font_name));
        config.font_size=(int)gadget_number(G_FONT_SIZE);
        if(g[G_ROW_SPACING])config.row_spacing=(int)gadget_number(G_ROW_SPACING);
        if (g[G_HIGH_CONTRAST]) config.high_contrast=!!(g[G_HIGH_CONTRAST]->Flags&GFLG_SELECTED);
        if (g[G_LARGE_GADGETS]) config.large_gadgets=!!(g[G_LARGE_GADGETS]->Flags&GFLG_SELECTED);
        if (g[G_REDUCED_COLORS]) config.reduced_colors=!!(g[G_REDUCED_COLORS]->Flags&GFLG_SELECTED);
    } else if(page==PAGE_LAYOUT) {
        config.window_width=(int)gadget_number(G_WIDTH);config.window_height=(int)gadget_number(G_HEIGHT);
        config.row_spacing=(int)gadget_number(G_ROW_SPACING);
        if(g[G_DETAILED_ROWS])config.detailed_rows=!!(g[G_DETAILED_ROWS]->Flags&GFLG_SELECTED);
        if(g[G_PREVIEW])config.icon_preview=!!(g[G_PREVIEW]->Flags&GFLG_SELECTED);
    } else if (page==PAGE_SOURCES) {
        char *text;
        if (g[G_INDEX_REXX]) config.index_rexx=!!(g[G_INDEX_REXX]->Flags&GFLG_SELECTED);
        if ((text=gadget_text(G_BOOKMARK_FILES)))
            beam_copy(config.bookmark_files,beam_trim(text),sizeof(config.bookmark_files));
        if ((text=gadget_text(G_URL_COMMAND)) && *beam_trim(text))
            beam_copy(config.url_command,beam_trim(text),sizeof(config.url_command));
    } else if (page==PAGE_EXTRAS) {
        char *text;
        if ((text=gadget_text(G_QUICK_LAUNCH)))
            beam_copy(config.quick_launch,beam_trim(text),sizeof(config.quick_launch));
        if (g[G_PLUGIN_CACHE]) config.plugin_cache=!!(g[G_PLUGIN_CACHE]->Flags&GFLG_SELECTED);
        if (g[G_RESULTS_MAX]) config.results_max=(int)gadget_number(G_RESULTS_MAX);
        if (g[G_BROWSE_MAX]) config.browse_max=(int)gadget_number(G_BROWSE_MAX);
        if (g[G_HIDDEN_MAX]) config.hidden_max=(int)gadget_number(G_HIDDEN_MAX);
        if (g[G_RECENT_DOCS]) config.recent_docs_max=(int)gadget_number(G_RECENT_DOCS);
    }
}
static char *entry_value(void)
{
    struct StringInfo *si;
    if (!g[G_ENTRY]) return NULL;
    si=(struct StringInfo *)g[G_ENTRY]->SpecialInfo;
    return si?(char *)si->Buffer:NULL;
}
static char *gadget_text(int id)
{
    struct StringInfo *si;
    if (id<0 || id>=(int)(sizeof(g)/sizeof(g[0])) || !g[id]) return NULL;
    si=(struct StringInfo *)g[id]->SpecialInfo;
    return si?(char *)si->Buffer:NULL;
}
/* Standard OS3.x ASL file requester.  The Indexing page restricts
 * selection to drawers, while Plugins picks a .plugin manifest file.
 * It only fills the path field; Add is an explicit second step. */
static int browse_path(void)
{
    struct FileRequester *fr;
    struct Gadget *target=(page==PAGE_ADVANCED)?g[G_APP_PATH]:g[G_ENTRY];
    char path[BEAM_MAX_PATH];
    int accepted=0;
    if (!AslBase || !target) { DisplayBeep(screen); return 0; }
    fr=AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)((page==PAGE_DIRS)?pstr(PMSG_SELECT_DIRECTORY):
                     (page==PAGE_PLUGINS)?pstr(PMSG_SELECT_PLUGIN):pstr(PMSG_SELECT_APPLICATION)),
        ASLFR_Window,(ULONG)win,
        ASLFR_PrivateIDCMP,TRUE, /* do not consume GadTools parent IDCMP */
        ASLFR_DrawersOnly,(ULONG)(page==PAGE_DIRS),
        TAG_DONE);
    if (!fr) { DisplayBeep(screen); return 0; }
    if (AslRequest(fr,NULL)) {
        /* Do not silently accept a truncated directory or file path. */
        if (!fr->fr_Drawer || strlen((const char *)fr->fr_Drawer)>=sizeof(path))
            path[0]=0;
        else beam_copy(path,(const char *)fr->fr_Drawer,sizeof(path));
        if (page!=PAGE_DIRS && *path) {
            if (!fr->fr_File || !*fr->fr_File) {
                /* Plugins: a drawer alone selects a package drawer. */
                if (page!=PAGE_PLUGINS) path[0]=0;
            } else if (strlen(path)+strlen((const char *)fr->fr_File)+2>=sizeof(path) ||
                !AddPart(path,(STRPTR)fr->fr_File,sizeof(path))) path[0]=0;
        }
        if (*path) {
            GT_SetGadgetAttrs(target,win,NULL,GTST_String,(ULONG)path,TAG_DONE);
            accepted=1;
        } else DisplayBeep(screen);
    }
    FreeAslRequest(fr);
    return accepted;
}

/* ASL font requester for the launcher font.  The launcher loads it with
 * diskfont.library, so any font in FONTS: can be used. */
static void choose_font(void)
{
    struct FontRequester *fo;
    if (!AslBase || !g[G_FONT_NAME] || !g[G_FONT_SIZE]) { DisplayBeep(screen); return; }
    fo=(struct FontRequester *)AllocAslRequestTags(ASL_FontRequest,
        ASLFO_TitleText,(ULONG)pstr(PMSG_SELECT_FONT),
        ASLFO_Window,(ULONG)win,
        ASLFO_PrivateIDCMP,TRUE,
        ASLFO_InitialName,(ULONG)config.font_name,
        ASLFO_InitialSize,(ULONG)config.font_size,
        ASLFO_MinHeight,6UL,
        ASLFO_MaxHeight,32UL,
        TAG_DONE);
    if (!fo) { DisplayBeep(screen); return; }
    if (AslRequest(fo,NULL) && fo->fo_Attr.ta_Name &&
        strlen((const char *)fo->fo_Attr.ta_Name)<BEAM_FONT_NAME_LEN) {
        beam_copy(config.font_name,(const char *)fo->fo_Attr.ta_Name,
                  sizeof(config.font_name));
        config.font_size=(int)fo->fo_Attr.ta_YSize;
        GT_SetGadgetAttrs(g[G_FONT_NAME],win,NULL,
                          GTST_String,(ULONG)config.font_name,TAG_DONE);
        GT_SetGadgetAttrs(g[G_FONT_SIZE],win,NULL,
                          GTIN_Number,(ULONG)config.font_size,TAG_DONE);
    }
    FreeAslRequest(fo);
}

/* -------------------------------------------------------- colour picker
 * A small window with a role cycle and red/green/blue sliders edits the
 * custom palette (CUSTOM_COLORS) without typing hex values.  The swatch
 * shows the nearest pen the screen can offer; the launcher allocates the
 * exact colour when it has free pens. */
#define CP_ROLE 1
#define CP_RED 2
#define CP_GREEN 3
#define CP_BLUE 4
#define CP_OK 5
#define CP_CANCEL 6
static void selected_palette(unsigned long *colors)
{
    int i;
    if(config.theme_modified[config.theme] || config.theme==BEAM_THEME_CUSTOM) {
        const unsigned long *p=beam_theme_config_palette(&config);
        for(i=0;i<BEAM_THEME_COLOR_COUNT;i++)colors[i]=p[i];
    } else if(config.theme==BEAM_THEME_WORKBENCH && screen) {
        static const int pens[5]={BEAM_PEN_TEXT,BEAM_PEN_SHADOW,BEAM_PEN_FILL,BEAM_PEN_FILLTEXT,BEAM_PEN_BACKGROUND};
        struct DrawInfo *di=GetScreenDrawInfo(screen);
        beam_theme_original_palette(BEAM_THEME_CUSTOM,colors);
        if(di && screen->ViewPort.ColorMap)for(i=0;i<5;i++) {
            ULONG rgb[3];
            if(di->dri_NumPens>pens[i]) {
                GetRGB32(screen->ViewPort.ColorMap,di->dri_Pens[pens[i]],1,rgb);
                colors[i]=((rgb[0]>>24)<<16)|((rgb[1]>>24)<<8)|(rgb[2]>>24);
            }
        }
        if(di)FreeScreenDrawInfo(screen,di);
    } else beam_theme_original_palette(config.theme,colors);
}
static void sync_palette_field(void)
{
    unsigned long colors[BEAM_THEME_COLOR_COUNT];selected_palette(colors);
    beam_theme_format_colors(colors,custom_colors_text,sizeof(custom_colors_text));
    custom_colors_valid=1;
    if(g[G_CUSTOM_COLORS])GT_SetGadgetAttrs(g[G_CUSTOM_COLORS],win,NULL,GTST_String,(ULONG)custom_colors_text,TAG_DONE);
}
static void edit_colours(void)
{
    static STRPTR roles[6];
    static const int role_ids[5]={PMSG_ROLE_TEXT,PMSG_ROLE_SHADOW,PMSG_ROLE_FILL,
                                  PMSG_ROLE_FILL_TEXT,PMSG_ROLE_BACKGROUND};
    unsigned long colours[BEAM_THEME_COLOR_COUNT];
    struct Gadget *list=NULL,*last,*sl[3];
    struct NewGadget ng;
    struct Window *cw;
    struct IntuiMessage *m;
    int role=0,i,finished=0,accept=0;
    LONG pen=-1;
    for (i=0;i<5;i++) roles[i]=(STRPTR)pstr(role_ids[i]);
    roles[5]=NULL;
    selected_palette(colours);
    last=CreateContext(&list);
    memset(&ng,0,sizeof(ng));
    ng.ng_TextAttr=&topaz; ng.ng_VisualInfo=visual;
    ng.ng_LeftEdge=90; ng.ng_TopEdge=(WORD)(screen->WBorTop+screen->Font->ta_YSize+8);
    ng.ng_Width=150; ng.ng_Height=14; ng.ng_GadgetText=(UBYTE *)pstr(PMSG_COLOUR_ROLE);
    ng.ng_GadgetID=CP_ROLE; ng.ng_Flags=PLACETEXT_LEFT;
    last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)roles,TAG_DONE);
    for (i=0;i<3;i++) {
        ng.ng_TopEdge+=20; ng.ng_Width=150; ng.ng_Height=11;
        ng.ng_GadgetText=(UBYTE *)(i==0?"R":i==1?"G":"B");
        ng.ng_GadgetID=(UWORD)(CP_RED+i);
        last=CreateGadget(SLIDER_KIND,last,&ng,GTSL_Min,0,GTSL_Max,255,
                          GTSL_Level,(ULONG)((colours[0]>>(16-8*i))&255UL),
                          GTSL_LevelFormat,(ULONG)"%3ld",GTSL_MaxLevelLen,3,
                          GTSL_LevelPlace,PLACETEXT_RIGHT,
                          GA_RelVerify,TRUE,TAG_DONE);
        sl[i]=last;
    }
    ng.ng_TopEdge+=24; ng.ng_LeftEdge=20; ng.ng_Width=90; ng.ng_Height=15;
    ng.ng_GadgetText=(UBYTE *)pstr(PMSG_OK); ng.ng_GadgetID=CP_OK; ng.ng_Flags=PLACETEXT_IN;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=210; ng.ng_GadgetText=(UBYTE *)pstr(PMSG_CANCEL); ng.ng_GadgetID=CP_CANCEL;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    if (!last) { FreeGadgets(list); DisplayBeep(screen); return; }
    cw=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,
        WA_Left,win?win->LeftEdge+60:40,WA_Top,win?win->TopEdge+40:40,
        WA_Width,330,WA_Height,(ULONG)(ng.ng_TopEdge+15+10),
        WA_Title,(ULONG)pstr(PMSG_EDIT_COLOURS),WA_Gadgets,(ULONG)list,
        WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,SLIDERIDCMP|BUTTONIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,
        TAG_DONE);
    if (!cw) { FreeGadgets(list); DisplayBeep(screen); return; }
    GT_RefreshWindow(cw,NULL);
    while (!finished) {
        unsigned long rgb=colours[role];
        /* Swatch: the closest pen this screen can show for the colour. */
        if (pen>=0) ReleasePen(screen->ViewPort.ColorMap,(ULONG)pen);
        pen=ObtainBestPenA(screen->ViewPort.ColorMap,((rgb>>16)&255UL)*0x01010101UL,
                           ((rgb>>8)&255UL)*0x01010101UL,(rgb&255UL)*0x01010101UL,NULL);
        if (pen>=0) {
            SetAPen(cw->RPort,(UBYTE)pen);
            RectFill(cw->RPort,284,(LONG)(sl[0]->TopEdge),318,(LONG)(sl[2]->TopEdge+sl[2]->Height));
        }
        WaitPort(cw->UserPort);
        while ((m=GT_GetIMsg(cw->UserPort))) {
            ULONG cls=m->Class; UWORD code=m->Code;
            struct Gadget *gad=(struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls==IDCMP_CLOSEWINDOW) finished=1;
            else if (cls==IDCMP_REFRESHWINDOW) { GT_BeginRefresh(cw); GT_EndRefresh(cw,TRUE); }
            else if ((cls==IDCMP_MOUSEMOVE || cls==IDCMP_GADGETUP) && gad) {
                int id=gad->GadgetID;
                if (id==CP_ROLE) {
                    role=code<5?code:0;
                    for (i=0;i<3;i++)
                        GT_SetGadgetAttrs(sl[i],cw,NULL,GTSL_Level,
                            (ULONG)((colours[role]>>(16-8*i))&255UL),TAG_DONE);
                } else if (id>=CP_RED && id<=CP_BLUE) {
                    int shift=16-8*(id-CP_RED);
                    colours[role]=(colours[role]&~(255UL<<shift))|((unsigned long)(code&255)<<shift);
                } else if (id==CP_OK) { accept=1; finished=1; }
                else if (id==CP_CANCEL) finished=1;
            }
        }
    }
    if (pen>=0) ReleasePen(screen->ViewPort.ColorMap,(ULONG)pen);
    CloseWindow(cw);
    FreeGadgets(list);
    if (!accept) return;
    beam_theme_set_palette(&config,colours);
    custom_colors_valid=1;
    sync_palette_field();
    if (g[G_CUSTOM_COLORS])
        GT_SetGadgetAttrs(g[G_CUSTOM_COLORS],win,NULL,GTST_String,(ULONG)custom_colors_text,TAG_DONE);
    draw_theme_preview();
}

/* Append a hotlist file to BOOKMARK_FILES (';'-separated). */
static void add_bookmark_file(void)
{
    char path[BEAM_MAX_PATH];
    unsigned int n;
    struct FileRequester *fr;
    collect_page();
    if (!AslBase) return;
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)pstr(PMSG_SRC_BOOKMARKS),ASLFR_Window,(ULONG)win,
        ASLFR_PrivateIDCMP,TRUE,TAG_DONE);
    if (!fr) return;
    path[0]=0;
    if (AslRequest(fr,NULL) && fr->fr_File && *fr->fr_File && fr->fr_Drawer &&
        strlen((const char *)fr->fr_Drawer)+strlen((const char *)fr->fr_File)+2<sizeof(path)) {
        beam_copy(path,(const char *)fr->fr_Drawer,sizeof(path));
        if (!AddPart((STRPTR)path,fr->fr_File,sizeof(path))) path[0]=0;
    }
    FreeAslRequest(fr);
    if (!path[0] || strchr(path,';')) return;
    n=(unsigned int)strlen(config.bookmark_files);
    if (n+strlen(path)+2>=sizeof(config.bookmark_files)) { DisplayBeep(screen); return; }
    if (n) strcat(config.bookmark_files,";");
    strcat(config.bookmark_files,path);
    if (g[G_BOOKMARK_FILES])
        GT_SetGadgetAttrs(g[G_BOOKMARK_FILES],win,NULL,GTST_String,(ULONG)config.bookmark_files,TAG_DONE);
}

static void modify_list(int remove)
{
    char path[BEAM_MAX_PATH],*input;
    int i,n,max;
    char (*data)[BEAM_MAX_PATH];
    if (page==PAGE_FAVORITES) {
        int changed=0;
        if (remove) {
            if (selection<0 || selection>=(int)favorites.count) return;
            changed=beam_favorites_remove(&favorites,favorites.paths[selection]);
        } else {
            char *group;
            input=entry_value();
            if (!input) { DisplayBeep(screen); return; }
            beam_copy(path,beam_trim(input),sizeof(path));
            if (!*path || strlen(input)>=BEAM_MAX_PATH) { DisplayBeep(screen); return; }
            group=gadget_text(G_FAVORITE_GROUP);
            if (!group || !*beam_trim(group)) group=(char *)BEAM_FAVORITE_DEFAULT_GROUP;
            if (!beam_favorites_contains(&favorites,path)) {
                changed=beam_favorites_add_group(&favorites,path,beam_trim(group));
                if (!changed) { DisplayBeep(screen); return; }
            } else changed=beam_favorites_set_group(&favorites,path,beam_trim(group));
        }
        if (!changed) return;
        favorites_changed=1;
        close_window(); if (!build_window()) done=1;
        return;
    }
    if (page==PAGE_DIRS) {
        n=config.dir_count; data=config.dirs; max=BEAM_MAX_DIRS;
    } else {
        n=config.plugin_file_count; data=config.plugin_files; max=BEAM_MAX_PLUGINS;
    }
    if (remove) {
        if (selection<0 || selection>=n) return;
        if (page==PAGE_PLUGINS && beam_catalog_is_bundled(&config,selection,&plugin_metadata[selection])) {
            DisplayBeep(screen);set_title(pstr(PMSG_BUNDLED_KEEP));return;
        }
        if (page==PAGE_PLUGINS) {
            static char package_dir[BEAM_MAX_PATH];
            install_message[0]=0;
            if (beam_plugin_package_dir(&config,data[selection],package_dir,sizeof(package_dir))) {
                struct EasyStruct es;
                APTR args[2];
                int answer;
                es.es_StructSize=sizeof(es); es.es_Flags=0;
                es.es_Title=(UBYTE *)pstr(PMSG_REMOVE_TITLE);
                es.es_TextFormat=(UBYTE *)pstr(PMSG_REMOVE_BODY);
                es.es_GadgetFormat=(UBYTE *)pstr(PMSG_REMOVE_BUTTONS);
                args[0]=(APTR)data[selection]; args[1]=(APTR)package_dir;
                answer=(int)EasyRequestArgs(win,&es,NULL,args);
                if (answer==0) return;
                if (answer==1) {
                    if (beam_plugin_delete_package(package_dir))
                        beam_copy(install_message,pstr(PMSG_FILES_DELETED),sizeof(install_message));
                    else beam_copy(install_message,pstr(PMSG_FILES_NOT_DELETED),sizeof(install_message));
                }
            }
            beam_plugin_forget_disabled(&config,data[selection]);
        }
        for (i=selection;i<n-1;i++) {
            beam_copy(data[i],data[i+1],BEAM_MAX_PATH);
            if (page==PAGE_PLUGINS) {
                config.plugin_preview_delay[i]=config.plugin_preview_delay[i+1];
                config.plugin_preview_disabled[i]=config.plugin_preview_disabled[i+1];
            }
        }
        --n;
        if (page==PAGE_PLUGINS) {
            config.plugin_preview_delay[n]=0;config.plugin_preview_disabled[n]=0;
        }
    } else {
        input=entry_value();
        if (!input) { DisplayBeep(screen); return; }
        beam_copy(path,beam_trim(input),sizeof(path));
        if (!*path || strlen(input)>=BEAM_MAX_PATH) { DisplayBeep(screen); return; }
        if (page==PAGE_PLUGINS && beam_plugin_install_candidate(path)) {
            /* A package drawer or archive is installed, not merely listed. */
            collect_page();
            if (!beam_plugin_install(win,&config,path,install_message,sizeof(install_message)))
                DisplayBeep(screen);
            close_window(); if (!build_window()) done=1;
            if (!done) set_title(install_message);
            return;
        }
        if (n>=max || (page==PAGE_PLUGINS && config.disabled_plugin_count>=BEAM_MAX_PLUGINS)) { DisplayBeep(screen); return; }
        for (i=0;i<n;i++) if (beam_equal(path,data[i])) return;
        if (page==PAGE_PLUGINS) {
            if(!beam_plugin_review_reference(win,&config,path,install_message,sizeof(install_message))) {
                set_title(install_message);return;
            }
            config.plugin_preview_delay[n]=0;config.plugin_preview_disabled[n]=0;
        }
        beam_copy(data[n++],path,BEAM_MAX_PATH);
        if(page==PAGE_PLUGINS) {config.plugin_file_count=n;beam_plugin_set_enabled(&config,path,0);}
    }
    if (page==PAGE_DIRS) config.dir_count=n;
    else config.plugin_file_count=n;
    collect_page();
    close_window(); if (!build_window()) done=1;
    if (!done && page==PAGE_PLUGINS && install_message[0]) {
        set_title(install_message); install_message[0]=0;
    }
}

static void import_plugin(void)
{
    static struct BeamPlugin metadata;
    char *path;
    if(page!=PAGE_PLUGINS || !browse_path())return;
    path=entry_value();
    if(!path || (!beam_plugin_install_candidate(path) && !beam_plugin_read(&metadata,path))) {
        DisplayBeep(screen);set_title(pstr(PMSG_PLUGIN_IMPORT_INVALID));return;
    }
    modify_list(0);
}

static void show_selected_application(void)
{
    const struct BeamAlias *entry;
    if (!win || page!=PAGE_ADVANCED || selection<0 ||
        selection>=(int)aliases.count || !g[G_APP_PATH]) return;
    entry=&aliases.entries[selection];
    GT_SetGadgetAttrs(g[G_APP_PATH],win,NULL,
        GTST_String,(ULONG)entry->path,TAG_DONE);
    GT_SetGadgetAttrs(g[G_APP_WORDS],win,NULL,
        GTST_String,(ULONG)entry->words,TAG_DONE);
    GT_SetGadgetAttrs(g[G_APP_ACTION],win,NULL,
        GTST_String,(ULONG)entry->action_label,TAG_DONE);
    GT_SetGadgetAttrs(g[G_APP_COMMAND],win,NULL,
        GTST_String,(ULONG)entry->action_command,TAG_DONE);
    sprintf(detail_title,pstr(PMSG_APPLICATIONS_TITLE),
            (long)selection+1,(long)aliases.count);
    SetWindowTitles(win,(STRPTR)detail_title,(STRPTR)~0UL);
}

static void show_selected_appearance(void)
{
    if (!win || page!=PAGE_APPEARANCE || selection<0 ||
        selection>=BEAM_THEME_COUNT) return;
    config.theme=selection;
    sync_palette_field();
    if (g[G_THEME_DESCRIPTION])
        GT_SetGadgetAttrs(g[G_THEME_DESCRIPTION],win,NULL,
            GTTX_Text,(ULONG)beam_theme_description(selection),TAG_DONE);
    sprintf(detail_title,pstr(PMSG_APPEARANCE_TITLE),beam_theme_name(selection));
    SetWindowTitles(win,(STRPTR)detail_title,(STRPTR)~0UL);
    draw_theme_preview();
    preview_main_theme();
}

static void show_selected_favorite(void)
{
    if (!win || page!=PAGE_FAVORITES || selection<0 ||
        selection>=(int)favorites.count) return;
    if (g[G_ENTRY]) GT_SetGadgetAttrs(g[G_ENTRY],win,NULL,
        GTST_String,(ULONG)favorites.paths[selection],TAG_DONE);
    if (g[G_FAVORITE_GROUP]) GT_SetGadgetAttrs(g[G_FAVORITE_GROUP],win,NULL,
        GTST_String,(ULONG)favorites.groups[selection],TAG_DONE);
}

static void preview_main_theme(void)
{
    BPTR lock=Lock((STRPTR)BEAM_SAFE_MODE_ACTIVE,ACCESS_READ);
    if (lock) {
        UnLock(lock);
        set_title(pstr(PMSG_APPEARANCE_WAITS));
        return;
    }
    if (beam_control_send_theme((unsigned long)beam_theme_config_mode(&config),
                                beam_theme_config_palette(&config)))
        theme_preview_changed=1;
}

static int import_theme_file(const char *path)
{
    unsigned long colors[BEAM_THEME_COLOR_COUNT];
    if (!path || !beam_theme_file_load(path,colors)) return 0;
    /* Commit only after the whole file has passed signature and color
       validation, so an unreadable drop cannot damage the active palette. */
    beam_theme_set_palette(&config,colors);selection=config.theme;
    beam_copy(theme_file_path,path,sizeof(theme_file_path));
    close_window();if(!build_window()){done=1;return 0;}
    show_selected_appearance();
    set_title(pstr(PMSG_THEME_IMPORTED));
    return 1;
}

static void import_theme_gadget(void)
{
    char *path=gadget_text(G_THEME_PATH);
    if (!path || !import_theme_file(beam_trim(path))) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_THEME_INVALID));
    }
}

static void export_theme_gadget(void)
{
    char *path=gadget_text(G_THEME_PATH);
    unsigned long colors[BEAM_THEME_COLOR_COUNT];
    collect_page();selected_palette(colors);
    if (!path || !beam_theme_file_save(beam_trim(path),colors)) {
        DisplayBeep(screen);set_title(pstr(PMSG_THEME_EXPORT_FAILED));
    } else set_title(pstr(PMSG_THEME_EXPORTED));
}

static void preview_text(struct RastPort *rp,int x,int baseline,
                         unsigned short pen,const char *text)
{
    SetAPen(rp,pen);
    Move(rp,(WORD)x,(WORD)baseline);
    Text(rp,(STRPTR)text,(ULONG)strlen(text));
}

static void draw_theme_preview(void)
{
    struct DrawInfo *di;
    struct BeamPalette palette,workbench;
    struct TagItem tags[2];
    LONG obtained[BEAM_THEME_ROLE_COUNT];
    int count=0,role,x,y,w,bottom;
    unsigned long red,green,blue;
    struct RastPort *rp;
    if (!win || !screen || page!=PAGE_APPEARANCE || !GfxBase ||
        selection<0 || selection>=BEAM_THEME_COUNT || win->Height<280) return;
    x=190; y=win->Height>=340?228:202;
    w=(int)win->Width-x-20; bottom=(int)win->Height-37;
    if (w<180 || bottom-y<58) return;
    di=GetScreenDrawInfo(screen);
    beam_palette_init(&workbench,di?di->dri_Pens:NULL,
                      di?di->dri_NumPens:0);
    if (di) FreeScreenDrawInfo(screen,di);
    palette=workbench;
    if (beam_theme_config_mode(&config)!=BEAM_THEME_WORKBENCH && screen->ViewPort.ColorMap) {
        tags[0].ti_Tag=OBP_Precision; tags[0].ti_Data=PRECISION_GUI;
        tags[1].ti_Tag=TAG_END; tags[1].ti_Data=0;
        for (role=0;role<BEAM_THEME_ROLE_COUNT;role++) {
            LONG pen;
            if (!beam_theme_color_custom(beam_theme_config_mode(&config),role,beam_theme_config_palette(&config),
                                         &red,&green,&blue)) break;
            pen=ObtainBestPenA(screen->ViewPort.ColorMap,red,green,blue,tags);
            if (pen<0) break;
            obtained[count++]=pen;
        }
        if (count==BEAM_THEME_ROLE_COUNT) {
            palette.text=(unsigned short)obtained[BEAM_THEME_ROLE_TEXT];
            palette.shadow=(unsigned short)obtained[BEAM_THEME_ROLE_SHADOW];
            palette.fill=(unsigned short)obtained[BEAM_THEME_ROLE_FILL];
            palette.fill_text=(unsigned short)obtained[BEAM_THEME_ROLE_FILL_TEXT];
            palette.background=(unsigned short)obtained[BEAM_THEME_ROLE_BACKGROUND];
            palette.highlight=palette.fill_text;
        } else palette=workbench;
    }
    rp=win->RPort;
    SetDrMd(rp,JAM1);
    preview_text(rp,x,y-4,workbench.text,pstr(PMSG_LAUNCHER_EXAMPLE));
    SetAPen(rp,palette.shadow);
    RectFill(rp,(WORD)x,(WORD)y,(WORD)(x+w-1),(WORD)bottom);
    SetAPen(rp,palette.background);
    RectFill(rp,(WORD)(x+1),(WORD)(y+1),(WORD)(x+w-2),(WORD)(bottom-1));
    preview_text(rp,x+7,y+11,palette.text,"BEAM68  > screen");
    SetAPen(rp,palette.shadow);
    RectFill(rp,(WORD)(x+5),(WORD)(y+16),(WORD)(x+w-6),(WORD)(y+16));
    SetAPen(rp,palette.fill);
    RectFill(rp,(WORD)(x+4),(WORD)(y+21),(WORD)(x+w-5),(WORD)(y+33));
    preview_text(rp,x+7,y+31,palette.fill_text,"ScreenMode");
    preview_text(rp,x+7,y+44,palette.text,"Shell");
    preview_text(rp,x+7,bottom-4,palette.text,"1/2 | Enter: Launch");
    if (screen->ViewPort.ColorMap)
        while (count>0) ReleasePen(screen->ViewPort.ColorMap,
                                   (ULONG)obtained[--count]);
}

static void modify_application(int remove)
{
    char path[BEAM_MAX_PATH],words[BEAM_ALIAS_LEN];
    char label[BEAM_ALIAS_ACTION_LABEL_LEN];
    char command[BEAM_ALIAS_ACTION_COMMAND_LEN];
    char *value;
    int next=-1;
    if (page!=PAGE_ADVANCED) return;
    if (remove) {
        if (!beam_aliases_remove(&aliases,selection)) {
            DisplayBeep(screen); return;
        }
        aliases_changed=1;
        if (aliases.count) next=selection<(int)aliases.count?selection:(int)aliases.count-1;
    } else {
        value=gadget_text(G_APP_PATH);
        if (!value || strlen(value)>=sizeof(path)) { DisplayBeep(screen); return; }
        beam_copy(path,beam_trim(value),sizeof(path));
        value=gadget_text(G_APP_WORDS);
        if (!value || strlen(value)>=sizeof(words)) { DisplayBeep(screen); return; }
        beam_copy(words,beam_trim(value),sizeof(words));
        value=gadget_text(G_APP_ACTION);
        if (!value || strlen(value)>=sizeof(label)) { DisplayBeep(screen); return; }
        beam_copy(label,beam_trim(value),sizeof(label));
        value=gadget_text(G_APP_COMMAND);
        if (!value || strlen(value)>=sizeof(command)) { DisplayBeep(screen); return; }
        beam_copy(command,beam_trim(value),sizeof(command));
        if (!beam_aliases_set_full(&aliases,path,words,label,command)) {
            DisplayBeep(screen);
            set_title(pstr(PMSG_APPLICATIONS_HINT));
            return;
        }
        aliases_changed=1;
        next=beam_aliases_find(&aliases,path);
    }
    selection=next;
    close_window();
    if (!build_window()) done=1;
}
/* WBArg locks are owned by Workbench and valid only until ReplyMsg().
 * Convert them to an absolute path before the reply, inspect the object,
 * then copy the resulting path into our bounded, editable settings lists. */
static int dropped_path(const struct WBArg *arg, char *out, int *is_dir)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int ok=0;
    if (!arg || !arg->wa_Lock || !NameFromLock(arg->wa_Lock,out,BEAM_MAX_PATH))
        return 0;
    if (arg->wa_Name && *arg->wa_Name &&
        !AddPart(out,(STRPTR)arg->wa_Name,BEAM_MAX_PATH)) return 0;
    lock=Lock((STRPTR)out,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib) {
        if (Examine(lock,fib)) {
            *is_dir=fib->fib_DirEntryType>0;
            ok=1;
        }
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    return ok;
}
static int accept_drop(const char *path, int is_dir)
{
    int i, target;
    char candidate[BEAM_MAX_PATH];
    char (*list)[BEAM_MAX_PATH];
    int *count;
    if (plugin_settings_mode) return 0;
    if (page==PAGE_DIRS) target=BEAM_DROP_DIRECTORY;
    else if (page==PAGE_PLUGINS) target=BEAM_DROP_PLUGIN;
    else if (page==PAGE_FAVORITES) target=BEAM_DROP_FAVORITE;
    else if (page==PAGE_APPEARANCE) target=BEAM_DROP_THEME;
    else return 0;
    if (!beam_drop_candidate(path,is_dir,target,candidate,sizeof(candidate)))
        return 0;
    if (page==PAGE_APPEARANCE) {
        unsigned long colors[BEAM_THEME_COLOR_COUNT];
        if (!beam_theme_file_load(candidate,colors)) return 0;
        beam_theme_set_palette(&config,colors);selection=config.theme;
        beam_copy(theme_file_path,candidate,sizeof(theme_file_path));
        return 1;
    }
    if (page==PAGE_FAVORITES) {
        if (beam_favorites_contains(&favorites,candidate)) return 0;
        if (!beam_favorites_add_group(&favorites,candidate,BEAM_FAVORITE_DEFAULT_GROUP)) return 0;
        favorites_changed=1;
        return 1;
    }
    if (page==PAGE_PLUGINS && beam_plugin_install_candidate(candidate)) {
        if (!beam_plugin_install(win,&config,candidate,install_message,sizeof(install_message)))
            DisplayBeep(screen);
        return 1; /* rebuild either way: the title shows the outcome */
    }
    if (page==PAGE_DIRS) { list=config.dirs;count=&config.dir_count; }
    else { list=config.plugin_files;count=&config.plugin_file_count; }
    if (*count>=(page==PAGE_DIRS?BEAM_MAX_DIRS:BEAM_MAX_PLUGINS)) return 0;
    for (i=0;i<*count;i++) if (beam_equal(list[i],candidate)) return 0;
    beam_copy(list[(*count)++],candidate,BEAM_MAX_PATH);
    return 1;
}
static int handle_drops(void)
{
    struct AppMessage *m;
    int changed=0,rejected=0;
    if (!app_port) return 0;
    while ((m=(struct AppMessage *)GetMsg(app_port))) {
        LONG i;
        if (m->am_Type==AMTYPE_APPWINDOW && m->am_NumArgs>0 && m->am_ArgList) {
            for (i=0;i<m->am_NumArgs;i++) {
                char path[BEAM_MAX_PATH];
                int is_dir=0;
                if (dropped_path(&m->am_ArgList[i],path,&is_dir) &&
                    accept_drop(path,is_dir)) changed=1;
                else rejected=1;
            }
        }
        ReplyMsg((struct Message *)m);
    }
    if (changed && win) {
        /* Rebuilding the GadTools list also renews the AppWindow handle. */
        if (page!=PAGE_APPEARANCE) collect_page();
        close_window();if (!build_window()) done=1;
        if (!done && page==PAGE_PLUGINS && install_message[0]) {
            set_title(install_message); install_message[0]=0;
        }
        if (!done && page==PAGE_APPEARANCE) {
            show_selected_appearance();
            set_title(pstr(PMSG_THEME_DROPPED));
        }
    }
    if (rejected && screen) DisplayBeep(screen);
    return changed;
}

static void toggle_plugin(void)
{
    const char *path;
    int enabled, old_selection=selection;
    if (page!=PAGE_PLUGINS || selection<0 ||
        selection>=config.plugin_file_count) { DisplayBeep(screen); return; }
    path=config.plugin_files[selection];
    collect_page();
    if (plugin_states[selection]==BEAM_PLUGIN_QUARANTINED) {
        beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
        if (!beam_plugin_health_clear_quarantine(&plugin_health,path) ||
            !beam_plugin_health_save(&plugin_health,BEAM_PLUGIN_HEALTH_PATH)) {
            DisplayBeep(screen);return;
        }
        beam_control_send(BEAM_CTL_RELOAD_PLUGINS);
        close_window();if(!build_window())done=1;
        set_title(pstr(PMSG_QUARANTINE_CLEARED));return;
    }
    enabled=beam_plugin_enabled(&config,path);
    if (!beam_plugin_set_enabled(&config,path,!enabled)) {
        DisplayBeep(screen); return;
    }
    close_window();
    if (!build_window()) { done=1; return; }
    /* Keep selection and scroll position when toggling many plugins. */
    selection=old_selection;
    show_selected_plugin();
    GT_SetGadgetAttrs(g[G_LIST],win,NULL,
        GTLV_Selected,(ULONG)selection, GTLV_MakeVisible,(ULONG)selection,
        TAG_DONE);
}

static void move_plugin(int direction)
{
    char path[BEAM_MAX_PATH];
    int target;
    unsigned short delay;
    unsigned char disabled;
    if (page!=PAGE_PLUGINS || selection<0 ||
        selection>=config.plugin_file_count) { DisplayBeep(screen); return; }
    target=selection+direction;
    if (target<0 || target>=config.plugin_file_count) {
        DisplayBeep(screen); return;
    }
    collect_page();
    beam_copy(path,config.plugin_files[selection],sizeof(path));
    beam_copy(config.plugin_files[selection],config.plugin_files[target],BEAM_MAX_PATH);
    beam_copy(config.plugin_files[target],path,BEAM_MAX_PATH);
    delay=config.plugin_preview_delay[selection];disabled=config.plugin_preview_disabled[selection];
    config.plugin_preview_delay[selection]=config.plugin_preview_delay[target];
    config.plugin_preview_disabled[selection]=config.plugin_preview_disabled[target];
    config.plugin_preview_delay[target]=delay;config.plugin_preview_disabled[target]=disabled;
    selection=target;
    close_window();
    if (!build_window()) done=1;
}
/* A short title updates on selection without opening a requester. Details
 * opens the complete read-only plugin document. */
static void show_selected_plugin(void)
{
    const char *name;
    if (!win || page!=PAGE_PLUGINS || plugin_settings_mode)return;
    if(g[G_SETTINGS])GT_SetGadgetAttrs(g[G_SETTINGS],win,NULL,GA_Disabled,
        (ULONG)(selection<0 || selection>=config.plugin_file_count ||
        !beam_catalog_has_settings(&plugin_metadata[selection])),TAG_DONE);
    if(g[G_REMOVE])GT_SetGadgetAttrs(g[G_REMOVE],win,NULL,GA_Disabled,
        (ULONG)(selection<0 || selection>=config.plugin_file_count ||
        beam_catalog_is_bundled(&config,selection,&plugin_metadata[selection])),TAG_DONE);
    if(selection<0 || selection>=config.plugin_file_count)return;
    name=plugin_metadata[selection].name[0]?
        plugin_metadata[selection].name:config.plugin_files[selection];
    {char readiness[160];plugin_readiness(&plugin_metadata[selection],readiness,sizeof(readiness));
     sprintf(detail_title,"%.35s : %.80s",name,readiness);}
    SetWindowTitles(win,(STRPTR)detail_title,(STRPTR)~0UL);
}
/* A local gadget list avoids disturbing the main Preferences page. The
 * value is committed only after explicit OK/Return and strict validation. */
/* Active string gadgets consume Escape before the window sees RAWKEY.
 * End editing explicitly and carry a distinct cancellation code to GADGETUP.
 * Intuition calls hooks with hook/object/message in a0/a2/a1. */
static ULONG preview_delay_edit(register struct Hook *hook __asm("a0"),
                                register struct SGWork *work __asm("a2"),
                                register ULONG *command __asm("a1"))
{
    (void)hook;
    if (*command!=SGH_KEY) return 0;
    if (work->IEvent && work->IEvent->ie_Code==0x45) {
        work->Actions=SGA_END;
        work->Code=0x45;
    }
    return 1;
}

static int plugin_preview_delay_entry(int *value)
{
    struct Gadget *list=NULL,*last,*input;
    struct NewGadget ng;
    struct Hook edit_hook;
    struct Window *dialog=NULL;
    struct IntuiMessage *message;
    char initial[16];
    int accepted=0,finished=0,top,height,width=360;
    top=(int)screen->WBorTop+(int)screen->Font->ta_YSize+28;
    height=top+62;
    if (screen->Width<width || (int)screen->Height-(int)screen->BarHeight-1<height) {
        DisplayBeep(screen);return 0;
    }
    last=CreateContext(&list);
    if (!last) goto cleanup;
    memset(&ng,0,sizeof(ng));
    ng.ng_TextAttr=&topaz;ng.ng_VisualInfo=visual;
    ng.ng_LeftEdge=20;ng.ng_TopEdge=(WORD)top;
    ng.ng_Width=320;ng.ng_Height=14;ng.ng_Flags=PLACETEXT_ABOVE;
    ng.ng_GadgetText=(UBYTE *)pstr(PMSG_PREVIEW_DELAY_LABEL);ng.ng_GadgetID=1;
    memset(&edit_hook,0,sizeof(edit_hook));
    edit_hook.h_Entry=(HOOKFUNC)(void (*)(void))preview_delay_edit;
    sprintf(initial,"%ld",(long)*value);
    input=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)initial,
                       GTST_MaxChars,12,GTST_EditHook,(ULONG)&edit_hook,TAG_DONE);
    if (!input) goto cleanup;
    ng.ng_TopEdge=(WORD)(top+30);ng.ng_Width=140;ng.ng_Height=17;
    ng.ng_Flags=PLACETEXT_IN;ng.ng_GadgetText=(UBYTE *)pstr(PMSG_OK);ng.ng_GadgetID=2;
    last=CreateGadget(BUTTON_KIND,input,&ng,TAG_DONE);
    if (!last) goto cleanup;
    ng.ng_LeftEdge=200;ng.ng_GadgetText=(UBYTE *)pstr(PMSG_CANCEL);ng.ng_GadgetID=3;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    if (!last) goto cleanup;
    dialog=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,
        WA_Left,(ULONG)(((int)screen->Width-width)/2),
        WA_Top,(ULONG)((int)screen->BarHeight+1+
            ((int)screen->Height-(int)screen->BarHeight-1-height)/2),
        WA_Width,(ULONG)width,WA_Height,(ULONG)height,
        WA_Title,(ULONG)pstr(PMSG_PREVIEW_TITLE),WA_Gadgets,(ULONG)list,
        WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|
                  IDCMP_VANILLAKEY|IDCMP_RAWKEY,TAG_DONE);
    if (!dialog) goto cleanup;
    GT_RefreshWindow(dialog,NULL);ActivateGadget(input,dialog,NULL);
    while (!finished) {
        WaitPort(dialog->UserPort);
        while ((message=GT_GetIMsg(dialog->UserPort))) {
            ULONG cls=message->Class;
            UWORD code=message->Code;
            struct Gadget *gadget=(struct Gadget *)message->IAddress;
            int apply=0;
            GT_ReplyIMsg(message);
            if (finished) continue;
            if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(dialog);GT_EndRefresh(dialog,TRUE);
            } else if (cls==IDCMP_CLOSEWINDOW ||
                       (cls==IDCMP_VANILLAKEY && code==27) ||
                       (cls==IDCMP_RAWKEY && code==0x45)) finished=1;
            else if (cls==IDCMP_GADGETUP && gadget) {
                if (gadget->GadgetID==3) finished=1;
                else if (gadget->GadgetID==2 || (gadget->GadgetID==1 && code==0)) apply=1;
                else if (gadget->GadgetID==1 && code==0x45) finished=1;
            } else if (cls==IDCMP_VANILLAKEY && (code==13 || code==10)) apply=1;
            if (apply) {
                struct StringInfo *info=(struct StringInfo *)input->SpecialInfo;
                int parsed;
                if (info && beam_plugin_preview_parse_delay((const char *)info->Buffer,&parsed)) {
                    *value=parsed;accepted=1;finished=1;
                } else {
                    DisplayBeep(screen);
                    SetWindowTitles(dialog,(STRPTR)pstr(PMSG_PREVIEW_DELAY_INVALID),(STRPTR)~0UL);
                    ActivateGadget(input,dialog,NULL);
                }
            }
        }
    }
cleanup:
    if (dialog) CloseWindow(dialog);
    else DisplayBeep(screen);
    if (list) FreeGadgets(list);
    return accepted;
}

static void plugin_preview_controls(void)
{
    struct EasyStruct es;
    ULONG args[3];
    int answer,delay,enabled;
    const char *manifest;
    if (selection<0 || selection>=config.plugin_file_count) return;
    manifest=config.plugin_files[selection];
    es.es_StructSize=sizeof(es);es.es_Flags=0;
    es.es_Title=(STRPTR)pstr(PMSG_PREVIEW_TITLE);
    if (!beam_catalog_can_preview(&plugin_metadata[selection])) {
        es.es_TextFormat=(STRPTR)pstr(PMSG_PREVIEW_UNAVAILABLE);
        es.es_GadgetFormat=(STRPTR)pstr(PMSG_OK);
        EasyRequestArgs(win,&es,NULL,NULL);return;
    }
    collect_page();
    for (;;) {
        enabled=!config.plugin_preview_disabled[selection];
        delay=config.plugin_preview_delay[selection]?config.plugin_preview_delay[selection]:400;
        es.es_TextFormat=(STRPTR)pstr(PMSG_PREVIEW_BODY);
        es.es_GadgetFormat=(STRPTR)pstr(PMSG_PREVIEW_BUTTONS);
        args[0]=(ULONG)(plugin_metadata[selection].name[0]?plugin_metadata[selection].name:manifest);
        args[1]=(ULONG)pstr(enabled?PMSG_STATE_ON:PMSG_STATE_OFF);
        args[2]=(ULONG)delay;
        answer=(int)EasyRequestArgs(win,&es,NULL,args);
        if (!answer) break;
        if (answer==1) enabled=!enabled;
        else if (answer==2) { delay-=100;if(delay<100)delay=100; }
        else if (answer==3) { delay+=100;if(delay>5000)delay=5000; }
        else if (answer==4) {
            if (!plugin_preview_delay_entry(&delay)) continue;
        } else if (answer==5) { enabled=1;delay=400; }
        if (!beam_plugin_preview_set(&config,manifest,enabled,delay)) {DisplayBeep(screen);break;}
    }
}

/* One scrollable document, sized to show every line when the screen permits. */
static void plugin_details_window(const char *body)
{
    static struct Node nodes[128];
    static char lines[128][104];
    struct List text;
    struct Gadget *list=NULL,*last;
    struct Window *dialog;
    struct IntuiMessage *message;
    const char *cursor=body;
    int count=0,w,h,top,finished=0,scroll=0,visible;
    int border=(int)screen->WBorTop+(int)screen->Font->ta_YSize+8;
    w=(int)screen->Width-12;if(w>760)w=760;
    NewList(&text);memset(nodes,0,sizeof(nodes));
    while(*cursor && count<128) {
        unsigned int next,n=beam_text_line(cursor,(unsigned int)(w-40)/8,&next);
        if(!next)break;
        if(n>=sizeof(lines[0]))n=sizeof(lines[0])-1;
        memcpy(lines[count],cursor,n);lines[count][n]=0;
        nodes[count].ln_Name=lines[count];AddTail(&text,&nodes[count]);
        cursor+=next;count++;
    }
    h=border+count*8+44;
    if(h>(int)screen->Height-(int)screen->BarHeight-5)h=(int)screen->Height-(int)screen->BarHeight-5;
    if(h<120)h=120;
    top=((int)screen->Height-h)/2;if(top<(int)screen->BarHeight+1)top=(int)screen->BarHeight+1;
    visible=(h-border-40)/8;if(visible<1)visible=1;
    last=CreateContext(&list);
    last=add_tagged(LISTVIEW_KIND,last,1,12,border,w-24,h-border-40,"",0,
                    GTLV_Labels,(ULONG)&text,GTLV_ReadOnly,TRUE);
    last=add_tagged(BUTTON_KIND,last,2,12,h-28,200,18,pstr(PMSG_TYPING_PREVIEW),PLACETEXT_IN,
        GA_Disabled,(ULONG)(selection<0 || selection>=config.plugin_file_count ||
        !beam_catalog_can_preview(&plugin_metadata[selection])),TAG_IGNORE,0);
    last=add_gadget(BUTTON_KIND,last,4,w/2-65,h-28,150,18,
        beam_equal(plugin_metadata[selection].id,"org.beam68.amihass")?"Test connection":"Check readiness",PLACETEXT_IN);
    last=add_gadget(BUTTON_KIND,last,3,w-112,h-28,100,18,pstr(PMSG_DETAILS_CLOSE),PLACETEXT_IN);
    if(!last){FreeGadgets(list);DisplayBeep(screen);return;}
    dialog=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Title,(ULONG)pstr(PMSG_DETAILS_TITLE),
        WA_Left,(ULONG)(((int)screen->Width-w)/2),WA_Top,(ULONG)top,
        WA_Width,(ULONG)w,WA_Height,(ULONG)h,WA_Gadgets,(ULONG)list,
        WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_SmartRefresh,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,TAG_DONE);
    if(!dialog){FreeGadgets(list);DisplayBeep(screen);return;}
    GT_RefreshWindow(dialog,NULL);
    while(!finished) {
        WaitPort(dialog->UserPort);
        while((message=GT_GetIMsg(dialog->UserPort))) {
            ULONG cls=message->Class;UWORD code=message->Code;
            struct Gadget *gadget=(struct Gadget *)message->IAddress;
            UWORD id=cls==IDCMP_GADGETUP && gadget?gadget->GadgetID:0;
            GT_ReplyIMsg(message);
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_GADGETUP && id==3) ||
               (cls==IDCMP_RAWKEY && code==PREF_RAW_ESCAPE))finished=1;
            else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(dialog);GT_EndRefresh(dialog,TRUE);}
            else if(cls==IDCMP_GADGETUP && id==2){plugin_preview_controls();ActivateWindow(dialog);}
            else if(cls==IDCMP_GADGETUP && id==4){plugin_connection_test(dialog);ActivateWindow(dialog);}
            else if(cls==IDCMP_RAWKEY && (code==PREF_RAW_UP || code==PREF_RAW_DOWN)) {
                scroll+=code==PREF_RAW_UP?-1:1;
                if(scroll>count-visible)scroll=count-visible;
                if(scroll<0)scroll=0;
                GT_SetGadgetAttrs(list->NextGadget,dialog,NULL,GTLV_Top,(ULONG)scroll,TAG_DONE);
            }
        }
    }
    CloseWindow(dialog);FreeGadgets(list);ActivateWindow(win);
}

static void plugin_details(void)
{
    static char body[4096];
    char health_text[160];
    const struct BeamPlugin *p;
    const struct BeamPluginHealthEntry *health;
    const char *manifest;
    if (page!=PAGE_PLUGINS || selection<0 ||
        selection>=config.plugin_file_count) { DisplayBeep(screen); return; }
    p=&plugin_metadata[selection];
    manifest=config.plugin_files[selection];
    /* The resident launcher may update this file while Preferences is open. */
    beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    health=beam_plugin_health_find(&plugin_health,manifest);
    if (!beam_plugin_health_format(health,health_text,sizeof(health_text)))
        beam_copy(health_text,pstr(PMSG_HEALTH_UNAVAILABLE),sizeof(health_text));
    /* The single details view wraps complete metadata without page truncation. */
    sprintf(body,pstr(PMSG_PLUGIN_VERSION),p->plugin_version[0]?p->plugin_version:"-");
    sprintf(body+strlen(body),pstr(PMSG_DETAILS1),
        p->name[0]?p->name:pstr(PMSG_UNKNOWN),p->category[0]?p->category:pstr(PMSG_NONE),
        p->trigger?p->trigger:'-',p->prefix[0]?p->prefix:pstr(PMSG_NONE),
        beam_plugin_diagnostic(plugin_states[selection]),
        (long)selection+1,(long)config.plugin_file_count);
    sprintf(body+strlen(body),pstr(PMSG_DETAILS2),
        health_text,
        p->capabilities[0]?p->capabilities:pstr(PMSG_NONE),
        p->description[0]?p->description:pstr(PMSG_NONE),
        p->example[0]?p->example:pstr(PMSG_NONE),
        p->help[0]?p->help:pstr(PMSG_NONE));
    sprintf(body+strlen(body),pstr(PMSG_DETAILS3),
        p->icon[0]?p->icon:pstr(PMSG_NONE),
        p->command[0]?p->command:pstr(PMSG_NOT_AVAILABLE),
        p->settings[0]?p->settings:pstr(PMSG_NO_SETTINGS),manifest);
    sprintf(body+strlen(body),pstr(PMSG_DETAILS4),(long)(p->protocol?p->protocol:1),
            p->guide[0]?p->guide:pstr(PMSG_NONE));
    sprintf(body+strlen(body),pstr(PMSG_DETAILS_META),
            p->id,p->author,p->license,p->min_beam68,p->url);
    strcat(body,"\n\n");
    strcat(body,beam_plugin_advice(plugin_states[selection]));
    strcat(body,"\n\n");strcat(body,beam_plugin_requirements(p->id));
    {char readiness[160];plugin_readiness(p,readiness,sizeof(readiness));
     sprintf(body+strlen(body),"\nReadiness: %s\n",readiness);}
    plugin_details_window(body);
}

static int plugin_setting_index(const char *key)
{
    int i;
    for (i=0;i<plugin_settings_meta.setting_count;i++)
        if (beam_equal(plugin_settings_meta.setting_defs[i].key,key)) return i;
    return -1;
}

static void load_plugin_setting_values(void)
{
    BPTR f;char line[256],*eq,*key,*value;int i,index;
    for (i=0;i<plugin_settings_meta.setting_count;i++)
        beam_copy(plugin_setting_values[i],
            plugin_settings_meta.setting_defs[i].default_value,
            sizeof(plugin_setting_values[i]));
    for(i=0;i<plugin_settings_meta.setting_count;i++) {
        const char *key=plugin_settings_meta.setting_defs[i].key;
        if(beam_equal(plugin_settings_meta.id,"org.beam68.whdload") && beam_equal(key,"COMMAND"))
            beam_copy(plugin_setting_values[i],config.whdload_command,sizeof(plugin_setting_values[i]));
        if(beam_equal(plugin_settings_meta.id,"org.beam68.dopus")) {
            if(beam_equal(key,"PORT"))beam_copy(plugin_setting_values[i],config.dopus_port,sizeof(plugin_setting_values[i]));
            if(beam_equal(key,"OPEN_DRAWERS"))beam_copy(plugin_setting_values[i],config.dopus_drawers?"YES":"NO",sizeof(plugin_setting_values[i]));
        }
    }
    f=Open((STRPTR)plugin_settings_path,MODE_OLDFILE);if (!f) return;
    while (FGets(f,line,sizeof(line))) {
        key=beam_trim(line);if (!*key||*key=='#'||*key==';') continue;
        eq=strchr(key,'=');if (!eq) continue;*eq++=0;
        key=beam_trim(key);value=beam_trim(eq);index=plugin_setting_index(key);
        if (index>=0 && strlen(value)<sizeof(plugin_setting_values[index]))
            beam_copy(plugin_setting_values[index],value,
                      sizeof(plugin_setting_values[index]));
    }
    Close(f);
}

static int choice_value(const char *choices,const char *value,char *canonical)
{
    char copy[128],*p,*comma;
    beam_copy(copy,choices,sizeof(copy));p=copy;
    while (*p) {
        comma=strchr(p,',');if (comma) *comma=0;
        p=beam_trim(p);
        if (beam_equal(p,value)) {beam_copy(canonical,p,64);return 1;}
        if (!comma) break;
        p=comma+1;
    }
    return 0;
}

static int apply_plugin_setting_value(void)
{
    struct BeamPluginSetting *d;char *value,canonical[64];const char *p;
    if (!plugin_settings_mode || selection<0 ||
        selection>=plugin_settings_meta.setting_count) return 0;
    value=gadget_text(G_PLUGIN_SETTING_VALUE);if (!value) return 0;
    value=beam_trim(value);d=&plugin_settings_meta.setting_defs[selection];
    if (strlen(value)>=sizeof(plugin_setting_values[selection])) return 0;
    if (d->type==BEAM_PLUGIN_SETTING_BOOL) {
        if (beam_equal(value,"YES")||beam_equal(value,"TRUE")||!strcmp(value,"1"))
            beam_copy(canonical,"YES",sizeof(canonical));
        else if (beam_equal(value,"NO")||beam_equal(value,"FALSE")||!strcmp(value,"0"))
            beam_copy(canonical,"NO",sizeof(canonical));
        else return 0;
        value=canonical;
    } else if (d->type==BEAM_PLUGIN_SETTING_NUMBER) {
        p=value;if (*p=='-'||*p=='+') p++;if (!*p) return 0;
        while (*p) if (*p<'0'||*p>'9') return 0;else p++;
    } else if (d->type==BEAM_PLUGIN_SETTING_CHOICE) {
        if (!choice_value(d->choices,value,canonical)) return 0;
        value=canonical;
    }
    beam_copy(plugin_setting_values[selection],value,
              sizeof(plugin_setting_values[selection]));
    return 1;
}

static void show_plugin_setting(void)
{
    const char *kind;struct BeamPluginSetting *d;
    if (!win || !plugin_settings_mode || selection<0 ||
        selection>=plugin_settings_meta.setting_count) return;
    d=&plugin_settings_meta.setting_defs[selection];
    if (g[G_PLUGIN_SETTING_VALUE])
        GT_SetGadgetAttrs(g[G_PLUGIN_SETTING_VALUE],win,NULL,
            GTST_String,(ULONG)plugin_setting_values[selection],TAG_DONE);
    kind=d->type==BEAM_PLUGIN_SETTING_BOOL?pstr(PMSG_TYPE_BOOL):
         d->type==BEAM_PLUGIN_SETTING_CHOICE?d->choices:
         d->type==BEAM_PLUGIN_SETTING_NUMBER?pstr(PMSG_TYPE_NUMBER):pstr(PMSG_TYPE_TEXT);
    sprintf(detail_title,"Beam68 - %s - %.44s (%s)",
            plugin_settings_meta.name,d->label,kind);
    SetWindowTitles(win,(STRPTR)detail_title,(STRPTR)~0UL);
}

static void cycle_plugin_setting(void)
{
    struct BeamPluginSetting *d;char copy[128],canonical[64],*p,*comma,*first;
    int found=0;
    if (!plugin_settings_mode || selection<0 ||
        selection>=plugin_settings_meta.setting_count) return;
    d=&plugin_settings_meta.setting_defs[selection];
    if (d->type==BEAM_PLUGIN_SETTING_BOOL) {
        beam_copy(plugin_setting_values[selection],
            beam_equal(plugin_setting_values[selection],"YES")?"NO":"YES",64);
    } else if (d->type==BEAM_PLUGIN_SETTING_CHOICE) {
        beam_copy(copy,d->choices,sizeof(copy));p=copy;first=p;canonical[0]=0;
        while (*p) {
            comma=strchr(p,',');if (comma) *comma=0;p=beam_trim(p);
            if (found) {beam_copy(canonical,p,sizeof(canonical));break;}
            if (beam_equal(p,plugin_setting_values[selection])) found=1;
            if (!comma) break;
            p=comma+1;
        }
        if (!canonical[0]) {first=beam_trim(first);beam_copy(canonical,first,sizeof(canonical));}
        beam_copy(plugin_setting_values[selection],canonical,64);
    } else { DisplayBeep(screen);return; }
    show_plugin_setting();
}

static int save_plugin_settings(void)
{
    char tmp[BEAM_MAX_PATH+5],bak[BEAM_MAX_PATH+5],line[256],copy[256],*eq,*key;
    BPTR in,out,old;int i,skip,had_old,ok=1;
    if (strlen(plugin_settings_path)+5>=sizeof(tmp)) return 0;
    sprintf(tmp,"%s.new",plugin_settings_path);sprintf(bak,"%s.bak",plugin_settings_path);
    out=Open((STRPTR)tmp,MODE_NEWFILE);if (!out) return 0;
    in=Open((STRPTR)plugin_settings_path,MODE_OLDFILE);
    while (in && FGets(in,line,sizeof(line))) {
        beam_copy(copy,line,sizeof(copy));key=beam_trim(copy);skip=0;
        if (*key && *key!='#' && *key!=';') {
            eq=strchr(key,'=');if (eq){*eq=0;if(plugin_setting_index(beam_trim(key))>=0)skip=1;}
        }
        if (!skip && FPuts(out,(STRPTR)line)<0) {ok=0;break;}
    }
    if (in) Close(in);
    for (i=0;ok && i<plugin_settings_meta.setting_count;i++) {
        sprintf(line,"%.31s=%.63s\n",plugin_settings_meta.setting_defs[i].key,
                plugin_setting_values[i]);
        if (FPuts(out,(STRPTR)line)<0) ok=0;
    }
    if (!Close(out)) ok=0;
    if (!ok) {DeleteFile((STRPTR)tmp);return 0;}
    old=Lock((STRPTR)plugin_settings_path,ACCESS_READ);had_old=old!=0;if(old)UnLock(old);
    if (had_old) {DeleteFile((STRPTR)bak);if(!Rename((STRPTR)plugin_settings_path,(STRPTR)bak)){DeleteFile((STRPTR)tmp);return 0;}}
    if (!Rename((STRPTR)tmp,(STRPTR)plugin_settings_path)) {
        if(had_old)Rename((STRPTR)bak,(STRPTR)plugin_settings_path);
        DeleteFile((STRPTR)tmp);return 0;
    }
    /* Notify the resident without applying unsaved parent Preferences edits. */
    if(!settings_qa_active)beam_control_send(BEAM_CTL_RELOAD_PLUGINS);
    return 1;
}

/* Plugin settings have an independent modal window and transaction. The main
 * Preferences gadgets, selection, unsaved edits and done flag are untouched. */
static void plugin_settings_window(void)
{
    static char choices[BEAM_MAX_PLUGIN_SETTINGS][128];
    static char *choice_labels[BEAM_MAX_PLUGIN_SETTINGS][65];
    static char labels[BEAM_MAX_PLUGIN_SETTINGS][34];
    static char edited[BEAM_MAX_PLUGIN_SETTINGS][64];
    struct Gadget *list=NULL,*last,*fields[BEAM_MAX_PLUGIN_SETTINGS];
    struct Window *dialog=NULL;struct IntuiMessage *message;
    struct Hook edit_hook;
    unsigned int active[BEAM_MAX_PLUGIN_SETTINGS],choice_counts[BEAM_MAX_PLUGIN_SETTINGS];
    int i,width,height,border,row_step,finished=0,valid;
    char title[120];
    width=(int)screen->Width-12;if(width>720)width=720;
    border=(int)screen->WBorTop+(int)screen->Font->ta_YSize+10;
    row_step=screen->Height<300?16:20;
    height=border+plugin_settings_meta.setting_count*row_step+36;
    if(width<500 || height>(int)screen->Height-(int)screen->BarHeight-2){DisplayBeep(screen);return;}
    memset(fields,0,sizeof(fields));memset(active,0,sizeof(active));
    memset(choice_counts,0,sizeof(choice_counts));
    memset(&edit_hook,0,sizeof(edit_hook));
    edit_hook.h_Entry=(HOOKFUNC)(void (*)(void))preview_delay_edit;
    last=CreateContext(&list);if(!last)return;
    for(i=0;i<plugin_settings_meta.setting_count;i++) {
        const struct BeamPluginSetting *d=&plugin_settings_meta.setting_defs[i];
        int y=border+i*row_step;
        beam_copy(labels[i],d->label,sizeof(labels[i]));
        if(d->type==BEAM_PLUGIN_SETTING_BOOL) {
            last=add_tagged(CHECKBOX_KIND,last,100+i,285,y,22,12,labels[i],PLACETEXT_LEFT,
                GTCB_Checked,(ULONG)(beam_equal(plugin_setting_values[i],"YES") ||
                    beam_equal(plugin_setting_values[i],"TRUE") || !strcmp(plugin_setting_values[i],"1")),TAG_IGNORE,0);
        } else if(d->type==BEAM_PLUGIN_SETTING_CHOICE) {
            char *p,*comma;unsigned int n=0;
            beam_copy(choices[i],d->choices,sizeof(choices[i]));p=choices[i];
            while(*p && n<64) {
                comma=strchr(p,',');if(comma)*comma=0;
                choice_labels[i][n]=beam_trim(p);
                if(beam_equal(choice_labels[i][n],plugin_setting_values[i]))active[i]=n;
                n++;if(!comma)break;p=comma+1;
            }
            choice_labels[i][n]=NULL;choice_counts[i]=n;
            last=add_tagged(CYCLE_KIND,last,100+i,285,y,width-300,15,labels[i],PLACETEXT_LEFT,
                GTCY_Labels,(ULONG)choice_labels[i],GTCY_Active,(ULONG)active[i]);
        } else {
            struct NewGadget ng;
            memset(&ng,0,sizeof(ng));ng.ng_TextAttr=&topaz;ng.ng_VisualInfo=visual;
            ng.ng_LeftEdge=285;ng.ng_TopEdge=y;ng.ng_Width=width-300;ng.ng_Height=14;
            ng.ng_GadgetID=100+i;ng.ng_GadgetText=(UBYTE *)labels[i];ng.ng_Flags=PLACETEXT_LEFT;
            last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)plugin_setting_values[i],
                GTST_MaxChars,63,GTST_EditHook,(ULONG)&edit_hook,TAG_DONE);
        }
        fields[i]=last;if(!last)goto cleanup;
    }
    last=add_gadget(BUTTON_KIND,last,3,15,height-26,160,18,pstr(PMSG_RESTORE_DEFAULTS),PLACETEXT_IN);
    if(!last)goto cleanup;
    last=add_gadget(BUTTON_KIND,last,1,width-210,height-26,90,18,pstr(PMSG_SAVE),PLACETEXT_IN);
    if(!last)goto cleanup;
    last=add_gadget(BUTTON_KIND,last,2,width-110,height-26,95,18,pstr(PMSG_CANCEL),PLACETEXT_IN);
    if(!last)goto cleanup;
    sprintf(title,"Beam68 - %.64s - %s",plugin_settings_meta.name,pstr(PMSG_SETTINGS));
    dialog=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Title,(ULONG)title,
        WA_Left,(ULONG)(((int)screen->Width-width)/2),
        WA_Top,(ULONG)((int)screen->BarHeight+2),WA_Width,(ULONG)width,WA_Height,(ULONG)height,
        WA_Gadgets,(ULONG)list,WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_SmartRefresh,TRUE,WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!dialog)goto cleanup;
    GT_RefreshWindow(dialog,NULL);
    while(!finished) {
        WaitPort(dialog->UserPort);
        while((message=GT_GetIMsg(dialog->UserPort))) {
            ULONG cls=message->Class;UWORD code=message->Code;
            struct Gadget *gadget=(struct Gadget *)message->IAddress;
            unsigned int id=cls==IDCMP_GADGETUP && gadget?gadget->GadgetID:0;
            GT_ReplyIMsg(message);if(finished)continue;
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==PREF_RAW_ESCAPE) ||
                (cls==IDCMP_GADGETUP && (id==2 || (id>=100 && code==0x45))))finished=1;
            else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(dialog);GT_EndRefresh(dialog,TRUE);}
            else if(cls==IDCMP_GADGETUP && id>=100 && id<100U+(unsigned int)plugin_settings_meta.setting_count) {
                i=id-100;if(plugin_settings_meta.setting_defs[i].type==BEAM_PLUGIN_SETTING_CHOICE)active[i]=code;
            } else if(cls==IDCMP_GADGETUP && id==3) {
                /* Stage defaults in the gadgets; no file or resident changes. */
                if(!beam_catalog_setting_defaults(&plugin_settings_meta,edited)) {
                    DisplayBeep(screen);continue;
                }
                for(i=0;i<plugin_settings_meta.setting_count;i++) {
                    const struct BeamPluginSetting *d=&plugin_settings_meta.setting_defs[i];
                    const char *value=edited[i];
                    if(d->type==BEAM_PLUGIN_SETTING_BOOL)
                        GT_SetGadgetAttrs(fields[i],dialog,NULL,GTCB_Checked,
                            (ULONG)(beam_equal(value,"YES") || beam_equal(value,"TRUE") || !strcmp(value,"1")),TAG_DONE);
                    else if(d->type==BEAM_PLUGIN_SETTING_CHOICE) {
                        unsigned int n;active[i]=0;
                        for(n=0;n<choice_counts[i];n++)if(beam_equal(value,choice_labels[i][n]))active[i]=n;
                        GT_SetGadgetAttrs(fields[i],dialog,NULL,GTCY_Active,(ULONG)active[i],TAG_DONE);
                    } else GT_SetGadgetAttrs(fields[i],dialog,NULL,GTST_String,(ULONG)value,TAG_DONE);
                }
            } else if(cls==IDCMP_GADGETUP && id==1) {
                valid=1;
                for(i=0;i<plugin_settings_meta.setting_count;i++) {
                    const struct BeamPluginSetting *d=&plugin_settings_meta.setting_defs[i];
                    char *value;
                    if(d->type==BEAM_PLUGIN_SETTING_BOOL)value=(fields[i]->Flags&GFLG_SELECTED)?"YES":"NO";
                    else if(d->type==BEAM_PLUGIN_SETTING_CHOICE)
                        value=active[i]<choice_counts[i]?choice_labels[i][active[i]]:"";
                    else value=((struct StringInfo *)fields[i]->SpecialInfo)->Buffer;
                    if(!beam_catalog_setting_value(d,value,edited[i],sizeof(edited[i]))) {
                        valid=0;break;
                    }
                }
                if(!valid){DisplayBeep(screen);SetWindowTitles(dialog,(STRPTR)pstr(PMSG_SETTING_INVALID),(STRPTR)~0UL);}
                else {
                    memcpy(plugin_setting_values,edited,sizeof(plugin_setting_values));
                    if(save_plugin_settings())finished=1;
                    else {DisplayBeep(screen);SetWindowTitles(dialog,(STRPTR)pstr(PMSG_SETTING_SAVE_FAILED),(STRPTR)~0UL);}
                }
            }
        }
    }
cleanup:
    if(dialog)CloseWindow(dialog);
    FreeGadgets(list);ActivateWindow(win);
}

/* Protocol-1 plugins can opt into an integrated page with SETTING records.
 * Older plugins retain the safe C:Ed fallback for their declared text file. */
static void edit_plugin_settings(void)
{
    char path[BEAM_MAX_PATH],cmd[BEAM_MAX_PATH+18];
    const struct BeamPlugin *p;
    if (page!=PAGE_PLUGINS || selection<0 ||
        selection>=config.plugin_file_count) { DisplayBeep(screen); return; }
    p=&plugin_metadata[selection];
    if (!p->settings[0] || !beam_catalog_resolve(&config,p->settings,path,sizeof(path)) ||
        !strchr(path,':') || strlen(path)+8>=sizeof(cmd)) {
        DisplayBeep(screen); return;
    }
    if (p->setting_count) {
        plugin_settings_plugin=selection;plugin_settings_meta=*p;
        beam_copy(plugin_settings_path,path,sizeof(plugin_settings_path));
        load_plugin_setting_values();
        plugin_settings_window();plugin_settings_plugin=-1;return;
    }
    sprintf(cmd,"C:Ed \"%s\"",path);
    if (beam_system(cmd,NULL,NULL,1)==-1) DisplayBeep(screen);
}
static void set_title(const char *text)
{
    if (win) SetWindowTitles(win,(STRPTR)text,(STRPTR)~0UL);
}
static unsigned capture_modifiers(UWORD qualifier)
{
    unsigned modifiers=0;
    if (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
        modifiers|=BEAM_HOTKEY_SHIFT;
    if (qualifier&IEQUALIFIER_CONTROL) modifiers|=BEAM_HOTKEY_CONTROL;
    if (qualifier&(IEQUALIFIER_LALT|IEQUALIFIER_RALT))
        modifiers|=BEAM_HOTKEY_ALT;
    if (qualifier&IEQUALIFIER_LCOMMAND) modifiers|=BEAM_HOTKEY_LCOMMAND;
    if (qualifier&IEQUALIFIER_RCOMMAND) modifiers|=BEAM_HOTKEY_RCOMMAND;
    return modifiers;
}
static void begin_hotkey_capture(void)
{
    capturing_hotkey=1;
    set_title(pstr(PMSG_CAPTURE_PRESS));
}
static void capture_hotkey(UWORD code, UWORD qualifier)
{
    struct InputEvent event;
    char mapped[8], value[96];
    int mapped_length=0, mapped_char=0;
    unsigned modifiers;
    if (!capturing_hotkey || (code&IECODE_UP_PREFIX)) return;
    modifiers=capture_modifiers(qualifier);
    if (code==PREF_RAW_ESCAPE && !modifiers) {
        capturing_hotkey=0;
        set_title(pstr(PMSG_CAPTURE_CANCELLED));
        return;
    }
    if (beam_hotkey_is_modifier(code)) return;
    memset(&event,0,sizeof(event));
    event.ie_Class=IECLASS_RAWKEY;
    event.ie_Code=code;
    /* Translate without qualifiers: they are represented separately in the
     * Commodities descriptor, while the base key follows the active keymap. */
    event.ie_Qualifier=0;
    if (KeymapBase) mapped_length=MapRawKey(&event,mapped,sizeof(mapped),NULL);
    if (mapped_length==1) mapped_char=(unsigned char)mapped[0];
    if (!beam_hotkey_format(value,sizeof(value),code,modifiers,mapped_char)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_CAPTURE_UNSUPPORTED));
        return;
    }
    beam_copy(config.hotkey,value,sizeof(config.hotkey));
    GT_SetGadgetAttrs(g[G_HOTKEY],win,NULL,GTST_String,(ULONG)config.hotkey,TAG_DONE);
    capturing_hotkey=0;
    set_title(pstr(PMSG_CAPTURED));
}
static void switch_page(UBYTE next)
{
    if (next==page && page!=PAGE_DIRS) return;
    collect_page();
    close_window();
    plugin_settings_mode=0;plugin_settings_plugin=-1;
    page=next; selection=-1;
    custom_colors_valid=1; /* an abandoned invalid edit is not committed */
    if (!build_window()) done=1;
}

static int page_list_count(void)
{
    if (page==PAGE_DIRS) return config.dir_count;
    if (page==PAGE_PLUGINS) return plugin_settings_mode?
        (int)plugin_settings_meta.setting_count:config.plugin_file_count;
    if (page==PAGE_FAVORITES) return (int)favorites.count;
    if (page==PAGE_ADVANCED) return (int)aliases.count;
    if (page==PAGE_APPEARANCE) return BEAM_THEME_COUNT;
    return 0;
}

static void move_list_selection(int direction)
{
    int count=page_list_count();
    if ((!g[G_LIST] && !(page==PAGE_APPEARANCE && g[G_THEME_CYCLE])) ||
        count<=0) { DisplayBeep(screen); return; }
    if (selection<0) selection=direction>0?0:count-1;
    else {
        selection+=direction;
        if (selection<0) selection=count-1;
        if (selection>=count) selection=0;
    }
    if (page==PAGE_APPEARANCE)
        GT_SetGadgetAttrs(g[G_THEME_CYCLE],win,NULL,
            GTCY_Active,(ULONG)selection,TAG_DONE);
    else
        GT_SetGadgetAttrs(g[G_LIST],win,NULL,
            GTLV_Selected,(ULONG)selection,GTLV_MakeVisible,(ULONG)selection,
            TAG_DONE);
    if (plugin_settings_mode) show_plugin_setting();
    else show_selected_plugin();
    show_selected_appearance();
    show_selected_favorite();
}

/* Quitting saves history and health files first, which can take several
 * seconds on a floppy or slow CF card; allow up to ~10 seconds. */
static int wait_for_main_state(int running)
{
    int attempts;
    for (attempts=0;attempts<100;attempts++) {
        if (beam_control_running()==running) return 1;
        Delay(5);
    }
    return beam_control_running()==running;
}

static void update_main_controls(void)
{
    int running;
    if (page!=PAGE_GENERAL || !win) return;
    running=beam_control_running();
    if (running==last_launcher_running) return;
    last_launcher_running=running;
    if (g[G_MAIN_START])
        GT_SetGadgetAttrs(g[G_MAIN_START],win,NULL,GA_Disabled,running,TAG_DONE);
    if (g[G_MAIN_RESTART])
        GT_SetGadgetAttrs(g[G_MAIN_RESTART],win,NULL,GA_Disabled,!running,TAG_DONE);
    if (g[G_MAIN_STOP])
        GT_SetGadgetAttrs(g[G_MAIN_STOP],win,NULL,GA_Disabled,!running,TAG_DONE);
    beam_copy(launcher_status_text,running?pstr(PMSG_RUNNING):pstr(PMSG_STOPPED),
              sizeof(launcher_status_text));
    if (g[G_MAIN_STATUS])
        GT_SetGadgetAttrs(g[G_MAIN_STATUS],win,NULL,
                          GTTX_Text,(ULONG)launcher_status_text,TAG_DONE);
}

static void set_launcher_transition(const char *text)
{
    if (page!=PAGE_GENERAL || !win) return;
    beam_copy(launcher_status_text,text,sizeof(launcher_status_text));
    if (g[G_MAIN_STATUS])
        GT_SetGadgetAttrs(g[G_MAIN_STATUS],win,NULL,
                          GTTX_Text,(ULONG)launcher_status_text,TAG_DONE);
    if (g[G_MAIN_START])
        GT_SetGadgetAttrs(g[G_MAIN_START],win,NULL,GA_Disabled,TRUE,TAG_DONE);
    if (g[G_MAIN_RESTART])
        GT_SetGadgetAttrs(g[G_MAIN_RESTART],win,NULL,GA_Disabled,TRUE,TAG_DONE);
    if (g[G_MAIN_STOP])
        GT_SetGadgetAttrs(g[G_MAIN_STOP],win,NULL,GA_Disabled,TRUE,TAG_DONE);
    /* Force the final state to repaint even when Restart ends in Running. */
    last_launcher_running=-1;
}

static int start_main_app(void)
{
    BPTR lock;
    char path[BEAM_MAX_PATH],command_text[BEAM_MAX_PATH+4];
    if (beam_control_running()) {
        set_title(pstr(PMSG_ALREADY_RUNNING));
        update_main_controls();
        return 1;
    }
    set_launcher_transition(pstr(PMSG_STARTING));
    beam_copy(path,config.install_dir,sizeof(path));
    if (!AddPart(path,(STRPTR)"Beam68",sizeof(path))) path[0]=0;
    lock=*path?Lock((STRPTR)path,ACCESS_READ):0;
    if (!lock) {
        beam_copy(path,"PROGDIR:Beam68",sizeof(path));
        lock=Lock((STRPTR)path,ACCESS_READ);
    }
    if (!lock || !NameFromLock(lock,path,sizeof(path))) {
        if (lock) UnLock(lock);
        DisplayBeep(screen);
        set_title(pstr(PMSG_EXE_NOT_FOUND));
        update_main_controls();
        return 0;
    }
    UnLock(lock);
    if (strchr(path,'\"') || strchr(path,'*') || strlen(path)+3>sizeof(command_text)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_INVALID_LAUNCHER_PATH));
        update_main_controls();
        return 0;
    }
    sprintf(command_text,"\"%s\"",path);
    if (beam_system(command_text,NULL,NULL,1)==-1) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_START_FAILED));
        update_main_controls();
        return 0;
    }
    if (wait_for_main_state(1))
        set_title(pstr(PMSG_STARTED));
    else set_title(pstr(PMSG_START_TIMEOUT));
    update_main_controls();
    return 1;
}

static void control_main_app(ULONG command)
{
    if (command==BEAM_CTL_SHOW) {
        start_main_app();
        return;
    }
    if (!beam_control_running()) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_NOT_RUNNING));
        update_main_controls();
        return;
    }
    set_launcher_transition(command==PREF_MAIN_RESTART?pstr(PMSG_RESTARTING):pstr(PMSG_STOPPING));
    if (!beam_control_send(BEAM_CTL_QUIT)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_NO_CONTACT));
        update_main_controls();
        return;
    }
    if (!wait_for_main_state(0)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_DID_NOT_STOP));
        update_main_controls();
        return;
    }
    if (command==PREF_MAIN_RESTART) {
        set_title(pstr(PMSG_RESTARTING_LAUNCHER));
        start_main_app();
    } else {
        set_title(pstr(PMSG_LAUNCHER_STOPPED));
        update_main_controls();
    }
}

static void edit_aliases(void)
{
    BPTR lock;
    char cmd[BEAM_MAX_PATH+18];
    if (aliases_changed) {
        if (!beam_aliases_save(&aliases,BEAM_ALIASES_ENVARC) ||
            !beam_aliases_save(&aliases,BEAM_ALIASES_ENV)) {
            DisplayBeep(screen);
            set_title(pstr(PMSG_APPS_SAVE_FAILED));
            return;
        }
        aliases_changed=0;
    }
    lock=Lock((STRPTR)BEAM_ALIASES_ENVARC,ACCESS_READ);
    if (lock) UnLock(lock);
    else if (!beam_aliases_save(&aliases,BEAM_ALIASES_ENVARC)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_ALIASES_CREATE_FAILED));
        return;
    }
    sprintf(cmd,"C:Ed \"%s\"",BEAM_ALIASES_ENVARC);
    if (beam_system(cmd,NULL,NULL,1)==-1) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_ED_FAILED));
    } else set_title(pstr(PMSG_EDIT_HINT));
}

/* Help opens the AmigaGuide manual (installed next to Beam68) at the node
 * for the current page.  amigaguide.library is optional; the call returns
 * when the user closes the guide. */
struct Library *AmigaGuideBase;
static void show_guide(void)
{
    static const char *nodes[]={"PREFS_GENERAL","PREFS_INDEXING","PREFS_PLUGINS",
        "PREFS_APPEARANCE","PREFS_FAVORITES","PREFS_APPLICATIONS","PREFS_ACCESSIBILITY",
        "PREFS_EXTRAS","PREFS_SOURCES","PREFS_APPEARANCE"};
    char path[BEAM_MAX_PATH],manifest[BEAM_MAX_PATH];
    struct NewAmigaGuide nag;
    AMIGAGUIDECONTEXT handle;
    BPTR lock=0;
    const char *node=nodes[page<=PAGE_LAST?page:0];
    /* On the Plugins page, a plugin's own GUIDE= wins over the manual. */
    if (page==PAGE_PLUGINS && !plugin_settings_mode && selection>=0 &&
        selection<config.plugin_file_count && plugin_metadata[selection].guide[0] &&
        beam_catalog_resolve(&config,config.plugin_files[selection],manifest,sizeof(manifest)) &&
        beam_plugindir_resolve(manifest,plugin_metadata[selection].guide,path,sizeof(path)) &&
        (lock=Lock((STRPTR)path,ACCESS_READ))!=0) node="MAIN";
    else if (!beam_catalog_resolve(&config,"PROGDIR:Beam68.guide",path,sizeof(path)) ||
        !(lock=Lock((STRPTR)path,ACCESS_READ))) {
        beam_copy(path,"PROGDIR:Beam68.guide",sizeof(path));
        lock=Lock((STRPTR)path,ACCESS_READ);
    }
    if (!lock) { DisplayBeep(screen); set_title(pstr(PMSG_GUIDE_MISSING)); return; }
    UnLock(lock);
    AmigaGuideBase=OpenLibrary("amigaguide.library",34);
    if (!AmigaGuideBase) { DisplayBeep(screen); set_title(pstr(PMSG_AMIGAGUIDE_MISSING)); return; }
    memset(&nag,0,sizeof(nag));
    nag.nag_Name=(STRPTR)path;
    nag.nag_Node=(STRPTR)node;
    nag.nag_Screen=screen;
    handle=OpenAmigaGuideA(&nag,NULL);
    if (handle) CloseAmigaGuide(handle);
    else DisplayBeep(screen);
    CloseLibrary(AmigaGuideBase); AmigaGuideBase=NULL;
}

static void show_about(void)
{
    struct EasyStruct es;
    ULONG args[3];char running[64];int reported;
    reported=beam_control_version(running,sizeof(running));
    if(reported==0) beam_copy(running,pstr(PMSG_ABOUT_NOT_RUNNING),sizeof(running));
    else if(reported<0) beam_copy(running,pstr(PMSG_ABOUT_UNAVAILABLE),sizeof(running));
    es.es_StructSize=sizeof(es);es.es_Flags=0;
    es.es_Title=(STRPTR)pstr(PMSG_ABOUT_TITLE);
    es.es_TextFormat=(STRPTR)pstr(PMSG_ABOUT_BODY);
    es.es_GadgetFormat=(STRPTR)pstr(PMSG_OK);
    args[0]=(ULONG)BEAM68_VERSION;args[1]=(ULONG)running;args[2]=(ULONG)BEAM68_VERSION_DATE;
    EasyRequestArgs(win,&es,NULL,args);
}

static void handle_rawkey(UWORD code,UWORD qualifier)
{
    int previous;
    if (code&IECODE_UP_PREFIX) return;
    if (capturing_hotkey) { capture_hotkey(code,qualifier); return; }
    if (code==PREF_RAW_ESCAPE) {
        if (plugin_settings_mode) {
            int restore=plugin_settings_plugin;
            plugin_settings_mode=0;plugin_settings_plugin=-1;selection=restore;
            close_window();if(!build_window())done=1;
        } else done=1;
        return;
    }
    if (code==PREF_RAW_HELP) { show_guide(); return; }
    if (code==PREF_RAW_F2) { show_about();return; }
    if (code==PREF_RAW_F1) {
        set_title(pstr(PMSG_F1_HINT));
        return;
    }
    if (code==PREF_RAW_F9) {
        control_main_app((qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))?
                         PREF_MAIN_RESTART:BEAM_CTL_SHOW);
        return;
    }
    if (code==PREF_RAW_F10) {
        control_main_app(BEAM_CTL_QUIT);
        return;
    }
    if (code==PREF_RAW_F7) {
        previous=(int)page-1; if (previous<0) previous=PAGE_LAST;
        switch_page((UBYTE)previous); return;
    }
    if (code==PREF_RAW_F8) {
        switch_page((UBYTE)((page+1)%(PAGE_LAST+1))); return;
    }
    if (qualifier&IEQUALIFIER_CONTROL) {
        if (code==PREF_RAW_LEFT || code==PREF_RAW_UP) {
            previous=(int)page-1; if (previous<0) previous=PAGE_LAST;
            switch_page((UBYTE)previous);
        } else if (code==PREF_RAW_RIGHT || code==PREF_RAW_DOWN) {
            switch_page((UBYTE)((page+1)%(PAGE_LAST+1)));
        }
        return;
    }
    /* Plain Left/Right is the reliable AmigaOS path. Preserve cursor
     * editing whenever a string or integer gadget is currently active. */
    if ((code==PREF_RAW_LEFT || code==PREF_RAW_RIGHT) && !edit_active) {
        if (code==PREF_RAW_LEFT) {
            previous=(int)page-1; if (previous<0) previous=PAGE_LAST;
            switch_page((UBYTE)previous);
        } else switch_page((UBYTE)((page+1)%(PAGE_LAST+1)));
        return;
    }
    if (code==PREF_RAW_UP) move_list_selection(-1);
    else if (code==PREF_RAW_DOWN) move_list_selection(1);
    else if (code==PREF_RAW_RETURN && page==PAGE_PLUGINS) {
        if (plugin_settings_mode) {
            if (!apply_plugin_setting_value()) DisplayBeep(screen);
            else {close_window();if(!build_window())done=1;}
        } else plugin_details();
    }
}
static int edit_gadget_id(UWORD id)
{
    return id==G_HOTKEY || id==G_ENTRY || id==G_MAXITEMS || id==G_VISIBLE ||
           id==G_DEPTH || id==G_WIDTH || id==G_HEIGHT || id==G_TIMEOUT ||
           id==G_APP_PATH || id==G_APP_WORDS || id==G_APP_ACTION ||
           id==G_APP_COMMAND || id==G_CUSTOM_COLORS ||
           id==G_PLUGIN_SETTING_VALUE || id==G_FILE_PATTERNS ||
           id==G_EXCLUDED || id==G_FAVORITE_GROUP || id==G_FONT_NAME ||
           id==G_FONT_SIZE || id==G_ROW_SPACING || id==G_THEME_PATH;
}

static int edit_gadget_at(WORD x,WORD y)
{
    static const UWORD ids[]={G_HOTKEY,G_ENTRY,G_MAXITEMS,G_VISIBLE,G_DEPTH,
                              G_WIDTH,G_HEIGHT,G_TIMEOUT,G_APP_PATH,G_APP_WORDS,
                              G_APP_ACTION,G_APP_COMMAND,G_CUSTOM_COLORS,
                              G_PLUGIN_SETTING_VALUE,G_FILE_PATTERNS,G_EXCLUDED,
                              G_FAVORITE_GROUP,G_FONT_NAME,G_FONT_SIZE,
                              G_ROW_SPACING,G_THEME_PATH};
    unsigned int i;
    for (i=0;i<sizeof(ids)/sizeof(ids[0]);i++) if (g[ids[i]]) {
        struct Gadget *p=g[ids[i]];
        if (x>=p->LeftEdge && x<p->LeftEdge+p->Width &&
            y>=p->TopEdge && y<p->TopEdge+p->Height) return 1;
    }
    return 0;
}
static void store(int permanent)
{
    BPTR safe_lock;
    int restart_recovery=0,applied;char reload_text[160];
    collect_page();
    if (!custom_colors_valid) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_BAD_COLORS));
        return;
    }
    if (beam_hotkey_has_duplicates(config.hotkey)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_BAD_HOTKEY));
        return;
    }
    if (!beam_config_valid(&config)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_INVALID_VALUES)); return;
    }
    /* Use writes to T:, which remains active for this Amiga session without
       relying on ENV:. Some Workbench installations map ENV: directly onto
       ENVARC:, so writing ENV: is neither reliably temporary nor portable. */
    if (permanent && !beam_config_write(&config,BEAM_CONFIG_ENVARC)) {
        DisplayBeep(screen);
        set_title(pstr(PMSG_ENVARC_FAILED)); return;
    }
    if (!beam_config_write(&config,BEAM_CONFIG_SESSION)) {
        DisplayBeep(screen); set_title(pstr(PMSG_SESSION_FAILED)); return;
    }
    /* Only touch favorites when this Prefs session actually edited them.
     * Otherwise the user may have pinned something with F6 meanwhile. */
    if (favorites_changed) {
        if (permanent && !beam_favorites_save(&favorites,BEAM_FAVS_ENVARC)) {
            DisplayBeep(screen); set_title(pstr(PMSG_FAV_ENVARC_FAILED)); return;
        }
        if (!beam_favorites_save(&favorites,BEAM_FAVS_SESSION)) {
            DisplayBeep(screen); set_title(pstr(PMSG_FAV_SESSION_FAILED)); return;
        }
    }
    if (aliases_changed) {
        if (permanent && !beam_aliases_save(&aliases,BEAM_ALIASES_ENVARC)) {
            DisplayBeep(screen); set_title(pstr(PMSG_APPS_ENVARC_FAILED)); return;
        }
        if (!beam_aliases_save(&aliases,BEAM_ALIASES_SESSION)) {
            DisplayBeep(screen); set_title(pstr(PMSG_APPS_SESSION_FAILED)); return;
        }
    }
    /* Recovery mode intentionally ignores custom themes.  Applying settings
       is an explicit, safe opportunity to perform a clean restart: the old
       process removes its crash marker and the replacement starts normally
       with the selected theme. */
    safe_lock=Lock((STRPTR)BEAM_SAFE_MODE_ACTIVE,ACCESS_READ);
    if (safe_lock) {
        UnLock(safe_lock);
        restart_recovery=beam_control_running();
    }
    if (restart_recovery) {
        set_title(pstr(PMSG_THEME_RESTART));
        control_main_app(PREF_MAIN_RESTART);
        if (!beam_control_running()) {
            DisplayBeep(screen);
            set_title(pstr(PMSG_RESTART_FAILED));
            return;
        }
    } else {
        applied=beam_control_reload(reload_text,sizeof(reload_text));
        settings_committed=1;
        if(applied<0 || applied==2) {
            DisplayBeep(screen);set_title(reload_text);return;
        }
    }
    settings_committed=1;
    done=1;
}
static void handle_events(void)
{
    struct IntuiMessage *m;
    while (!done && (m=GT_GetIMsg(win->UserPort))) {
        ULONG cls=m->Class;
        UWORD code=m->Code, qualifier=m->Qualifier, id=0;
        WORD mouse_x=m->MouseX,mouse_y=m->MouseY;
        if ((cls==IDCMP_GADGETUP || cls==IDCMP_GADGETDOWN) && m->IAddress)
            id=((struct Gadget *)m->IAddress)->GadgetID;
        GT_ReplyIMsg(m);
        if (cls==IDCMP_REFRESHWINDOW) {
            GT_BeginRefresh(win);draw_plugin_heading();GT_EndRefresh(win,TRUE);
            draw_theme_preview();
        } else if (cls==IDCMP_INTUITICKS) {
            if (++launcher_state_tick>=5) {
                launcher_state_tick=0;
                update_main_controls();
            }
        } else if (cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
            edit_active=edit_gadget_at(mouse_x,mouse_y);
        } else if (cls==IDCMP_GADGETDOWN) {
            edit_active=edit_gadget_id(id);
        } else if (cls==IDCMP_RAWKEY) handle_rawkey(code,qualifier);
        else if (cls==IDCMP_CLOSEWINDOW || id==G_CANCEL) done=1;
        else if ((id>=G_SECTION_FIRST && id<=G_SECTION_LAST) || id==G_SECTION_EXTRAS ||
                 id==G_SECTION_SOURCES) {
            UBYTE next=(UBYTE)(id==G_SECTION_EXTRAS?PAGE_EXTRAS:
                               id==G_SECTION_SOURCES?PAGE_SOURCES:id-G_SECTION_FIRST);
            /* Rebuilding while GadTools is draining this gadget event can
               leave a half-refreshed window on OS 3.x.  Defer it. */
            pending_page=(int)next;
            return;
        } else if(id==G_INDEX_VIEW) {
            index_settings_view=(int)code;pending_page=PAGE_DIRS;return;
        } else if(id==G_GROUP) {
            if(page==PAGE_GENERAL || page==PAGE_EXTRAS || page==PAGE_SOURCES)
                pending_page=code==0?PAGE_GENERAL:code==1?PAGE_EXTRAS:PAGE_SOURCES;
            else pending_page=code==0?PAGE_APPEARANCE:code==1?PAGE_ACCESSIBILITY:PAGE_LAYOUT;
            return;
        } else if(id==G_UNLIMITED) {
            GT_SetGadgetAttrs(g[G_DEPTH],win,NULL,GA_Disabled,(ULONG)!!(g[G_UNLIMITED]->Flags&GFLG_SELECTED),TAG_DONE);
        } else if (id==G_THEME_CYCLE) {
            selection=(int)code;
            show_selected_appearance();
        } else if (id==G_COMPACT_MODE) {
            config.compact_mode=(int)code;
        } else if (id==G_LIST) {
            selection=(code==0xffff)?-1:(int)code;
            if (page==PAGE_PLUGINS) {
                if (plugin_settings_mode) show_plugin_setting();
                else show_selected_plugin();
            }
            else if (page==PAGE_ADVANCED) show_selected_application();
            else if (page==PAGE_FAVORITES) show_selected_favorite();
            else if (page==PAGE_APPEARANCE) show_selected_appearance();
        } else if (id==G_DETAILS && !plugin_settings_mode) plugin_details();
        else if (id==G_SETTINGS) edit_plugin_settings();
        else if (id==G_PLUGIN_SETTING_APPLY) {
            if (!apply_plugin_setting_value()) {
                DisplayBeep(screen);set_title(pstr(PMSG_SETTING_INVALID));
            } else { close_window();if(!build_window())done=1; }
        }
        else if (id==G_PLUGIN_SETTING_NEXT) cycle_plugin_setting();
        else if (id==G_PLUGIN_SETTING_SAVE) {
            if (!apply_plugin_setting_value() || !save_plugin_settings()) {
                DisplayBeep(screen);set_title(pstr(PMSG_SETTING_SAVE_FAILED));
            } else set_title(pstr(PMSG_SETTING_SAVED));
        }
        else if (id==G_PLUGIN_SETTINGS_BACK) {
            int restore=plugin_settings_plugin;
            plugin_settings_mode=0;plugin_settings_plugin=-1;selection=restore;
            close_window();if(!build_window())done=1;
        }
        else if (id==G_BROWSE) browse_path();
        else if (id==G_ADD) modify_list(0);
        else if (id==G_REMOVE) {
            if (page==PAGE_ADVANCED) modify_application(1);
            else modify_list(1);
        }
        else if (id==G_APP_UPDATE) modify_application(0);
        else if (id==G_TOGGLE) toggle_plugin();
        else if (id==G_PLUGIN_UP) move_plugin(-1);
        else if (id==G_PLUGIN_DOWN) move_plugin(1);
        else if (id==G_IMPORT_PLUGIN) import_plugin();
        else if (id==G_THEME_IMPORT) import_theme_gadget();
        else if (id==G_FONT_CHOOSE) choose_font();
        else if (id==G_EDIT_COLOURS) { edit_colours();preview_main_theme(); }
        else if (id==G_RESET_PALETTE) {
            beam_theme_reset_palette(&config);sync_palette_field();
            show_selected_appearance();
        }
        else if (id==G_BOOKMARK_BROWSE) add_bookmark_file();
        else if (id==G_THEME_EXPORT) export_theme_gadget();
        else if (id==G_CAPTURE) begin_hotkey_capture();
        else if (id==G_ABOUT) show_about();
        else if (id==G_MAIN_START) control_main_app(BEAM_CTL_SHOW);
        else if (id==G_MAIN_RESTART) control_main_app(PREF_MAIN_RESTART);
        else if (id==G_MAIN_STOP) control_main_app(BEAM_CTL_QUIT);
        else if (id==G_ALIAS_EDIT) edit_aliases();
        else if (id==G_CUSTOM_COLORS) {
            collect_page();
            if (!custom_colors_valid) {
                DisplayBeep(screen);
                set_title(pstr(PMSG_BAD_COLORS));
            } else {
                show_selected_appearance();
            }
        }
        else if (id==G_USE) store(0);
        else if (id==G_SAVE) store(1);
        /* GadTools may clear the visual inversion when focus moves from a
         * ListView to a button. Re-assert the selected ordinal after every
         * plugin-page event so Details/Settings/Toggle never leave the user
         * wondering which row those actions target. */
        if (!done && win && page==PAGE_PLUGINS && !plugin_settings_mode &&
            selection>=0 && selection<config.plugin_file_count && g[G_LIST])
            GT_SetGadgetAttrs(g[G_LIST],win,NULL,
                GTLV_Selected,(ULONG)selection,
                GTLV_MakeVisible,(ULONG)selection,TAG_DONE);
    }
}

/* Test entry point: all writes are confined to a new T: drawer. */
static int plugin_settings_qa(void)
{
    char manifest[BEAM_MAX_PATH],path[BEAM_MAX_PATH],canonical[64],line[256];
    static char expected[BEAM_MAX_PLUGIN_SETTINGS][64];
    int i,j,slot,total=0;BPTR lock,file;
    for(slot=0;slot<100;slot++) {
        sprintf(path,"T:Beam68SettingsQA%02d",slot);lock=CreateDir((STRPTR)path);
        if(lock){UnLock(lock);break;}
    }
    if(slot==100)return 0;
    for(i=0;i<config.plugin_file_count;i++) {
        if(!beam_catalog_resolve(&config,config.plugin_files[i],manifest,sizeof(manifest)) ||
           !beam_plugin_read(&plugin_settings_meta,manifest))return 0;
        if(!plugin_settings_meta.setting_count)continue;
        sprintf(plugin_settings_path,"%.32s/plugin-%d.cfg",path,i);
        file=Open((STRPTR)plugin_settings_path,MODE_NEWFILE);if(!file)return 0;
        FPuts(file,"# Keep comments and unknown keys\nUNKNOWN_OPTION=preserved\n");Close(file);
        load_plugin_setting_values();
        for(j=0;j<plugin_settings_meta.setting_count;j++) {
            if(!beam_catalog_setting_value(&plugin_settings_meta.setting_defs[j],
                plugin_setting_values[j],canonical,sizeof(canonical)))return 0;
            beam_copy(plugin_setting_values[j],canonical,sizeof(plugin_setting_values[j]));
        }
        memcpy(expected,plugin_setting_values,sizeof(expected));
        if(!save_plugin_settings())return 0;
        memset(plugin_setting_values,0,sizeof(plugin_setting_values));
        load_plugin_setting_values();
        for(j=0;j<plugin_settings_meta.setting_count;j++)
            if(strcmp(expected[j],plugin_setting_values[j]))return 0;
        file=Open((STRPTR)plugin_settings_path,MODE_OLDFILE);if(!file)return 0;
        j=0;while(FGets(file,line,sizeof(line)))if(strstr(line,"UNKNOWN_OPTION=preserved"))j=1;
        Close(file);if(!j)return 0;
        printf("PASS: %s settings validate, save, reload and retain unknown keys\n",plugin_settings_meta.name);total++;
    }
    printf("PASS: %d bundled plugin settings files\n",total);return total>0;
}

static int prefs_main(int argc,char **argv)
{
    int settings_qa=argc>1 && !strcmp(argv[1],"SETTINGSQA");
    int ok=0,layout_qa=argc>1 && !strcmp(argv[1],"LAYOUTQA");
    const char *startup_failure="Required libraries could not be opened (Intuition, graphics, GadTools and keymap). AmigaOS 3.0+ is required.";
    settings_qa_active=settings_qa;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    AslBase=OpenLibrary("asl.library",38);
    WorkbenchBase=OpenLibrary("workbench.library",39);
    KeymapBase=OpenLibrary("keymap.library",37);
    prefs_locale_open();
    beam_copy(launcher_status_text,pstr(PMSG_STOPPED),sizeof(launcher_status_text));
    if (!IntuitionBase || !GfxBase || !GadToolsBase || !KeymapBase) goto cleanup;
    if (WorkbenchBase) app_port=CreateMsgPort();
    beam_config_defaults(&config);
    beam_config_read_active(&config);
    if(layout_qa || settings_qa) {
        beam_config_defaults(&config);beam_config_read(&config,"PROGDIR:Beam68.cfg");
        if(!NameFromLock(GetProgramDir(),config.install_dir,sizeof(config.install_dir)))goto cleanup;
    }
    /* Every PROGDIR:Plugins/<Drawer>/<Name>.plugin on disk is listed;
       newly found ones start disabled. */
    beam_catalog_merge_discovered(&config,(const char (*)[BEAM_MAX_PATH])discovered_specs,
        beam_plugin_scan_installed(&config,discovered_specs,BEAM_MAX_PLUGINS));
    beam_catalog_discover_bundled(&config);
    initial_config=config;
    beam_favorites_load_active(&favorites);
    beam_aliases_load_active(&aliases);
    beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    if(settings_qa){ok=plugin_settings_qa();goto cleanup;}
    startup_failure="No public screen is available. Open Workbench and try again.";
    screen=layout_qa?OpenScreenTags(NULL,SA_Width,640,SA_Height,200,SA_Depth,3,
        SA_DisplayID,0x8000UL,SA_Type,PUBLICSCREEN,SA_Title,(ULONG)"Beam68 Preferences layout QA",TAG_DONE):LockPubScreen(NULL);
    if (!screen) goto cleanup;
    startup_failure="Not enough memory for Preferences screen resources.";
    visual=GetVisualInfoA(screen,NULL);
    if (!visual) goto cleanup;
    startup_failure="Could not open Preferences. Free memory or use a larger public screen.";
    if(layout_qa) {
        int test_page,a,b,view;ok=1;
        for(test_page=0;test_page<=PAGE_LAST;test_page++) {
            for(view=0;view<(test_page==PAGE_DIRS?2:1);view++) {
                page=(UBYTE)test_page;index_settings_view=view;
                if(!build_window()){printf("FAIL: cannot build page %d/%d\n",test_page,view);ok=0;continue;}
                for(a=0;a<113;a++)if(g[a]) {
                    struct Gadget *ga=g[a];
                    if(ga->LeftEdge<0 || ga->TopEdge<win->BorderTop ||
                       ga->LeftEdge+ga->Width>win->Width-win->BorderRight ||
                       ga->TopEdge+ga->Height>win->Height-win->BorderBottom) {
                        printf("FAIL: page %d/%d gadget %d outside window\n",test_page,view,a);ok=0;
                    }
                    for(b=a+1;b<113;b++)if(g[b] && g[b]!=ga) {
                        struct Gadget *gb=g[b];
                        if(ga->LeftEdge<gb->LeftEdge+gb->Width && gb->LeftEdge<ga->LeftEdge+ga->Width &&
                           ga->TopEdge<gb->TopEdge+gb->Height && gb->TopEdge<ga->TopEdge+ga->Height) {
                            printf("FAIL: page %d/%d gadgets %d and %d overlap\n",test_page,view,a,b);ok=0;
                        }
                    }
                }
                close_window();
            }
        }
        puts(ok?"Preferences 640x200 gadget geometry: PASS":"Preferences layout: FAIL");
        goto cleanup;
    }
    if (!build_window()) goto cleanup;
    ok=1;
    while (!done && win) {
        ULONG signals=1UL<<win->UserPort->mp_SigBit;
        int rebuilt=0;
        if (app_port) signals|=1UL<<app_port->mp_SigBit;
        signals=Wait(signals);
        /* A drop may rebuild the Preferences window; its old IDCMP port
         * is drained by close_window before the next handle_events call. */
        if (app_port && (signals&(1UL<<app_port->mp_SigBit)))
            rebuilt=handle_drops();
        /* If AppWindow drop rebuilt the window, the old IDCMP signal is
         * stale. Only consume IDCMP on the next event-loop iteration. */
        if (!rebuilt && !done && win && (signals&(1UL<<win->UserPort->mp_SigBit)))
            handle_events();
        if (!done && pending_page>=0) {
            UBYTE next=(UBYTE)pending_page;
            pending_page=-1;
            switch_page(next);
        }
    }
cleanup:
    if (!ok) beam_startup_error("Beam68 Preferences",startup_failure);
    if (theme_preview_changed && !settings_committed)
        beam_control_send_theme((unsigned long)beam_theme_config_mode(&initial_config),
                                beam_theme_config_palette(&initial_config));
    close_window();
    if (app_port) { DeleteMsgPort(app_port); app_port=NULL; }
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (visual) FreeVisualInfo(visual);
    if (screen) {if(layout_qa)CloseScreen(screen);else UnlockPubScreen(NULL,screen);}
    if (AslBase) CloseLibrary(AslBase);
    if (KeymapBase) CloseLibrary(KeymapBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    beam_aliases_free(&aliases);
    prefs_locale_close();
    return ok?0:20;
}

int main(int argc,char **argv)
{
    return beam_run_with_stack(16384UL,prefs_main,argc,argv);
}
