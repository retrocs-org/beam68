/* Beam68Prefs -- separate native GadTools Preferences editor for AmigaOS 3.2.
 * Uses only the OS3.x NDK and no MUI/Reaction/68020/FPU requirements.
 * Install as SYS:Prefs/Beam68Prefs; launcher can live anywhere.
 */
#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
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
const char beam68prefs_version_tag[]="$VER: Beam68Prefs 0.1 (27.10.2026)";

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
#define PAGE_LAST PAGE_SOURCES
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
#define G_RELOAD_PLUGINS 48
#define G_EXPORT_DIAGNOSTICS 49
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
#define PREF_MAIN_RESTART 100UL
#define PREF_RAW_RETURN 0x44
#define PREF_RAW_ESCAPE 0x45
#define PREF_RAW_UP 0x4c
#define PREF_RAW_DOWN 0x4d
#define PREF_RAW_LEFT 0x4f
#define PREF_RAW_RIGHT 0x4e
#define PREF_RAW_F1 0x50
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
static struct Gadget *g[110];
static struct BeamPlugin plugin_metadata[BEAM_MAX_PLUGINS];
static unsigned char plugin_states[BEAM_MAX_PLUGINS];
static char detail_title[128];
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
static STRPTR page_names[]={(STRPTR)"General",(STRPTR)"Indexing",(STRPTR)"Plugins","Appearance","Favorites","Applications","Accessibility","Extras","Sources",NULL};
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
        PMSG_PAGE_EXTRAS,PMSG_PAGE_SOURCES};
    struct TagItem tags[2];
    int i;
    LocaleBase=(struct LocaleBase *)OpenLibrary("locale.library",38);
    if (LocaleBase) {
        tags[0].ti_Tag=OC_BuiltInLanguage; tags[0].ti_Data=(ULONG)"english";
        tags[1].ti_Tag=TAG_DONE; tags[1].ti_Data=0;
        prefs_catalog=OpenCatalogA(NULL,(STRPTR)"Beam68Prefs.catalog",tags);
    }
    for (i=0;i<9;i++) page_names[i]=(STRPTR)pstr(pages[i]);
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
            sprintf(plugin_labels[i],"[%s] %.60s %c",
                plugin_states[i]==BEAM_PLUGIN_OK?pstr(PMSG_STATE_ON):
                plugin_states[i]==BEAM_PLUGIN_DISABLED?pstr(PMSG_STATE_OFF):
                plugin_states[i]==BEAM_PLUGIN_QUARANTINED?pstr(PMSG_STATE_QUAR):pstr(PMSG_STATE_ERR),
                name,plugin_metadata[i].trigger?plugin_metadata[i].trigger:' ');
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
    build_labels();
    last=CreateContext(&gadgets);
    if (!last) return 0;
    /* Nine sections.  Low screens (NTSC 640x200) get two narrow columns so
       the buttons never reach Use/Save/Cancel. */
    section_height=(config.large_gadgets && h>=300)?21:(h>=260?17:15);
    section_step=(config.large_gadgets && h>=300)?24:(h>=260?20:16);
    for (i=0;i<9;i++) {
        UWORD id=(UWORD)(i<7?G_SECTION_FIRST+i:i==7?G_SECTION_EXTRAS:G_SECTION_SOURCES);
        if (h>=230) {
            sprintf(section_button_labels[i],"%c %.20s",i==page?'>':' ',page_names[i]);
            last=add_gadget(BUTTON_KIND,last,id,16,40+i*section_step,145,section_height,
                            section_button_labels[i],PLACETEXT_IN);
        } else {
            sprintf(section_button_labels[i],"%c%.7s",i==page?'>':' ',page_names[i]);
            last=add_gadget(BUTTON_KIND,last,id,8+(i&1)*78,40+(i>>1)*17,74,15,
                            section_button_labels[i],PLACETEXT_IN);
        }
    }
    if (page==PAGE_GENERAL) {
        last=add_tagged(STRING_KIND,last,G_HOTKEY,right+85,44,
                         content_width-210,14,pstr(PMSG_HOTKEY),PLACETEXT_LEFT,
                         GTST_String,(ULONG)config.hotkey,GTST_MaxChars,95);
        g[G_HOTKEY]=last;
        last=add_gadget(BUTTON_KIND,last,G_CAPTURE,w-130,42,110,17,
                        pstr(PMSG_CAPTURE),PLACETEXT_IN);
        g[G_CAPTURE]=last;
        last=add_tagged(INTEGER_KIND,last,G_MAXITEMS,right+85,75,100,14,
                        pstr(PMSG_MAX_ITEMS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.max_items,GTIN_MaxChars,4);
        g[G_MAXITEMS]=last;
        last=add_tagged(INTEGER_KIND,last,G_VISIBLE,right+85,104,100,14,
                        pstr(PMSG_RESULTS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.visible,GTIN_MaxChars,2);
        g[G_VISIBLE]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_HISTORY,right+85,137,22,12,
                        pstr(PMSG_RANK_FREQUENT),PLACETEXT_RIGHT,
                        GTCB_Checked,config.history,TAG_IGNORE,0); g[G_HISTORY]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_CACHE,right+300,137,22,12,
                        pstr(PMSG_INDEX_CACHE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_cache,TAG_IGNORE,0); g[G_CACHE]=last;
    } else if (page==PAGE_DIRS) {
        excluded_text[0]=0;
        for (i=0;i<config.excluded_dir_count;i++) {
            if (i && strlen(excluded_text)+1<sizeof(excluded_text)) strcat(excluded_text,";");
            if (strlen(excluded_text)+strlen(config.excluded_dirs[i])<sizeof(excluded_text))
                strcat(excluded_text,config.excluded_dirs[i]);
        }
        last=add_tagged(INTEGER_KIND,last,G_DEPTH,right+72,40,65,14,
                        pstr(PMSG_DEPTH),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.max_depth,GTIN_MaxChars,2);
        g[G_DEPTH]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_ICONS,right+190,40,22,12,
                        pstr(PMSG_ICONS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.scan_icons,TAG_IGNORE,0); g[G_ICONS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_EXES,right+300,40,22,12,
                        pstr(PMSG_EXECUTABLES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.scan_executables,TAG_IGNORE,0); g[G_EXES]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_FILES,right+72,62,22,12,
                        pstr(PMSG_FILES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_files,TAG_IGNORE,0);g[G_INDEX_FILES]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_DRAWERS,right+190,62,22,12,
                        pstr(PMSG_DRAWERS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_drawers,TAG_IGNORE,0);g[G_INDEX_DRAWERS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_GAMES,right+300,62,22,12,
                        pstr(PMSG_WHDLOAD_GAMES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_games,TAG_IGNORE,0);g[G_INDEX_GAMES]=last;
        last=add_tagged(STRING_KIND,last,G_FILE_PATTERNS,right+110,85,
                        content_width-110,14,pstr(PMSG_FILE_PATTERNS),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.file_patterns,
                        GTST_MaxChars,BEAM_FILE_PATTERNS_LEN-1);g[G_FILE_PATTERNS]=last;
        last=add_tagged(STRING_KIND,last,G_EXCLUDED,right+110,108,
                        content_width-110,14,pstr(PMSG_EXCLUDED),PLACETEXT_LEFT,
                        GTST_String,(ULONG)excluded_text,
                        GTST_MaxChars,sizeof(excluded_text)-1);g[G_EXCLUDED]=last;
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,140,content_width,
                        entry_y-145,
                        pstr(PMSG_DIRECTORIES),PLACETEXT_ABOVE,
                        GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0); g[G_LIST]=last;
        last=add_tagged(STRING_KIND,last,G_ENTRY,right,entry_y,entry_width,13,"",0,
                        GTST_String,(ULONG)"",GTST_MaxChars,BEAM_MAX_PATH-1); g[G_ENTRY]=last;
        last=add_gadget(BUTTON_KIND,last,G_BROWSE,button_x,entry_y-2,68,17,pstr(PMSG_BROWSE),PLACETEXT_IN); g[G_BROWSE]=last;
        last=add_gadget(BUTTON_KIND,last,G_ADD,button_x+74,entry_y-2,58,17,pstr(PMSG_ADD),PLACETEXT_IN); g[G_ADD]=last;
        last=add_gadget(BUTTON_KIND,last,G_REMOVE,button_x+136,entry_y-2,72,17,pstr(PMSG_REMOVE),PLACETEXT_IN); g[G_REMOVE]=last;
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
        last=add_tagged(CHECKBOX_KIND,last,G_SHELL_SERVICE,right+20,42,22,12,
                        pstr(PMSG_SHELL_SERVICE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.shell_commands,TAG_IGNORE,0);
        g[G_SHELL_SERVICE]=last;
        last=add_tagged(LISTVIEW_KIND,last,G_LIST,right,78,content_width-130,
                        list_height-10,pstr(PMSG_PLUGIN_LIST),PLACETEXT_ABOVE,
                        GTLV_Labels,(ULONG)&labels,TAG_IGNORE,0); g[G_LIST]=last;
        last=add_gadget(BUTTON_KIND,last,G_DETAILS,w-140,48,120,17,pstr(PMSG_DETAILS),PLACETEXT_IN);
        g[G_DETAILS]=last;
        last=add_gadget(BUTTON_KIND,last,G_SETTINGS,w-140,70,120,17,pstr(PMSG_SETTINGS),PLACETEXT_IN);
        g[G_SETTINGS]=last;
        last=add_gadget(BUTTON_KIND,last,G_TOGGLE,w-140,92,120,17,pstr(PMSG_TOGGLE),PLACETEXT_IN);
        g[G_TOGGLE]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_UP,w-140,114,58,17,pstr(PMSG_UP),PLACETEXT_IN);
        g[G_PLUGIN_UP]=last;
        last=add_gadget(BUTTON_KIND,last,G_PLUGIN_DOWN,w-78,114,58,17,pstr(PMSG_DOWN),PLACETEXT_IN);
        g[G_PLUGIN_DOWN]=last;
        last=add_gadget(BUTTON_KIND,last,G_RELOAD_PLUGINS,w-140,136,120,17,pstr(PMSG_RELOAD_PLUGINS),PLACETEXT_IN);
        g[G_RELOAD_PLUGINS]=last;
        last=add_gadget(BUTTON_KIND,last,G_EXPORT_DIAGNOSTICS,w-140,158,120,17,pstr(PMSG_EXPORT_REPORT),PLACETEXT_IN);
        g[G_EXPORT_DIAGNOSTICS]=last;
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
        last=add_gadget(BUTTON_KIND,last,G_REMOVE,button_x+136,entry_y-2,72,17,pstr(PMSG_REMOVE),PLACETEXT_IN); g[G_REMOVE]=last;
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
        beam_theme_format_colors(config.custom_colors,custom_colors_text,
                                 sizeof(custom_colors_text));
        last=add_tagged(CYCLE_KIND,last,G_THEME_CYCLE,right+55,44,175,15,
                        pstr(PMSG_THEME),PLACETEXT_LEFT,
                        GTCY_Labels,(ULONG)theme_cycle_labels,
                        GTCY_Active,(ULONG)selection); g[G_THEME_CYCLE]=last;
        last=add_gadget(BUTTON_KIND,last,G_EDIT_COLOURS,right+55,66,175,15,
                        pstr(PMSG_EDIT_COLOURS),PLACETEXT_IN); g[G_EDIT_COLOURS]=last;
        last=add_tagged(INTEGER_KIND,last,G_WIDTH,right+350,48,100,14,
                        pstr(PMSG_WINDOW_WIDTH),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.window_width,GTIN_MaxChars,4); g[G_WIDTH]=last;
        last=add_tagged(INTEGER_KIND,last,G_HEIGHT,right+350,79,100,14,
                        pstr(PMSG_WINDOW_HEIGHT),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.window_height,GTIN_MaxChars,4); g[G_HEIGHT]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_PREVIEW,right+260,111,22,12,
                        pstr(PMSG_ICON_PREVIEW),PLACETEXT_RIGHT,
                        GTCB_Checked,config.icon_preview,TAG_IGNORE,0); g[G_PREVIEW]=last;
        if (h>=230) {
            last=add_tagged(STRING_KIND,last,G_CUSTOM_COLORS,right+260,137,
                            content_width-260,14,pstr(PMSG_CUSTOM_PALETTE),PLACETEXT_ABOVE,
                            GTST_String,(ULONG)custom_colors_text,
                            GTST_MaxChars,sizeof(custom_colors_text)-1);
            g[G_CUSTOM_COLORS]=last;
            last=add_tagged(TEXT_KIND,last,G_THEME_DESCRIPTION,right+260,166,
                            content_width-260,18,pstr(PMSG_THEME_DETAILS),PLACETEXT_ABOVE,
                            GTTX_Text,(ULONG)beam_theme_description(selection),
                            GTTX_Border,TRUE);
            g[G_THEME_DESCRIPTION]=last;
        }
        if (h>=340) {
            last=add_tagged(STRING_KIND,last,G_THEME_PATH,right+76,195,
                            content_width-246,14,pstr(PMSG_THEME_FILE),PLACETEXT_LEFT,
                            GTST_String,(ULONG)theme_file_path,
                            GTST_MaxChars,BEAM_MAX_PATH-1);g[G_THEME_PATH]=last;
            last=add_gadget(BUTTON_KIND,last,G_THEME_IMPORT,w-180,193,72,17,
                            pstr(PMSG_IMPORT),PLACETEXT_IN);g[G_THEME_IMPORT]=last;
            last=add_gadget(BUTTON_KIND,last,G_THEME_EXPORT,w-102,193,72,17,
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
        last=add_tagged(INTEGER_KIND,last,G_ROW_SPACING,right+300,78,70,14,
                        pstr(PMSG_ROW_SPACING),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.row_spacing,GTIN_MaxChars,2);g[G_ROW_SPACING]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_HIGH_CONTRAST,right+40,116,22,12,
                        pstr(PMSG_HIGH_CONTRAST),PLACETEXT_RIGHT,
                        GTCB_Checked,config.high_contrast,TAG_IGNORE,0);g[G_HIGH_CONTRAST]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_LARGE_GADGETS,right+280,116,22,12,
                        pstr(PMSG_LARGE_GADGETS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.large_gadgets,TAG_IGNORE,0);g[G_LARGE_GADGETS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_REDUCED_COLORS,right+40,151,22,12,
                        pstr(PMSG_REDUCED_COLORS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.reduced_colors,TAG_IGNORE,0);g[G_REDUCED_COLORS]=last;
        last=add_tagged(CYCLE_KIND,last,G_COMPACT_MODE,right+300,148,125,15,
                        pstr(PMSG_COMPACT),PLACETEXT_LEFT,
                        GTCY_Labels,(ULONG)compact_labels,
                        GTCY_Active,(ULONG)config.compact_mode);g[G_COMPACT_MODE]=last;
    } else if (page==PAGE_SOURCES) {
        /* Index sources and how their results are opened (15-pixel rows
           so the page also fits a 640x200 screen). */
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_MODULES,right+20,44,22,12,
                        pstr(PMSG_SRC_MODULES),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_modules,TAG_IGNORE,0);g[G_INDEX_MODULES]=last;
        last=add_tagged(STRING_KIND,last,G_MODULE_PORT,right+150,59,120,13,
                        pstr(PMSG_SRC_PLAYER_PORT),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.module_port,
                        GTST_MaxChars,sizeof(config.module_port)-1);g[G_MODULE_PORT]=last;
        last=add_tagged(STRING_KIND,last,G_MODULE_AREXX,right+150,74,content_width-170,13,
                        pstr(PMSG_SRC_PLAYER_AREXX),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.module_arexx,
                        GTST_MaxChars,sizeof(config.module_arexx)-1);g[G_MODULE_AREXX]=last;
        last=add_tagged(STRING_KIND,last,G_MODULE_COMMAND,right+150,89,content_width-170,13,
                        pstr(PMSG_SRC_PLAYER_COMMAND),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.module_command,
                        GTST_MaxChars,sizeof(config.module_command)-1);g[G_MODULE_COMMAND]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_INDEX_REXX,right+20,104,22,12,
                        pstr(PMSG_SRC_REXX),PLACETEXT_RIGHT,
                        GTCB_Checked,config.index_rexx,TAG_IGNORE,0);g[G_INDEX_REXX]=last;
        last=add_tagged(STRING_KIND,last,G_BOOKMARK_FILES,right+150,119,content_width-240,13,
                        pstr(PMSG_SRC_BOOKMARKS),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.bookmark_files,
                        GTST_MaxChars,sizeof(config.bookmark_files)-1);g[G_BOOKMARK_FILES]=last;
        if (AslBase) {
            last=add_gadget(BUTTON_KIND,last,G_BOOKMARK_BROWSE,right+content_width-84,118,
                            64,15,pstr(PMSG_BROWSE),PLACETEXT_IN);g[G_BOOKMARK_BROWSE]=last;
        }
        last=add_tagged(STRING_KIND,last,G_URL_COMMAND,right+150,134,content_width-170,13,
                        pstr(PMSG_SRC_URL_COMMAND),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.url_command,
                        GTST_MaxChars,sizeof(config.url_command)-1);g[G_URL_COMMAND]=last;
    } else if (page==PAGE_EXTRAS) {
        /* Settings that used to be configuration-file only. */
        last=add_tagged(STRING_KIND,last,G_QUICK_LAUNCH,right+150,46,120,14,
                        pstr(PMSG_QUICK_LAUNCH),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.quick_launch,
                        GTST_MaxChars,sizeof(config.quick_launch)-1);g[G_QUICK_LAUNCH]=last;
        last=add_tagged(STRING_KIND,last,G_WHDLOAD_CMD,right+150,62,120,14,
                        pstr(PMSG_WHDLOAD_CMD),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.whdload_command,
                        GTST_MaxChars,sizeof(config.whdload_command)-1);g[G_WHDLOAD_CMD]=last;
        last=add_tagged(STRING_KIND,last,G_DOPUS_PORT,right+150,78,100,14,
                        pstr(PMSG_DOPUS_PORT),PLACETEXT_LEFT,
                        GTST_String,(ULONG)config.dopus_port,
                        GTST_MaxChars,sizeof(config.dopus_port)-1);g[G_DOPUS_PORT]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_DOPUS_DRAWERS,right+20,96,22,12,
                        pstr(PMSG_DOPUS_DRAWERS),PLACETEXT_RIGHT,
                        GTCB_Checked,config.dopus_drawers,TAG_IGNORE,0);g[G_DOPUS_DRAWERS]=last;
        last=add_tagged(CHECKBOX_KIND,last,G_PLUGIN_CACHE,right+20,112,22,12,
                        pstr(PMSG_PLUGIN_CACHE),PLACETEXT_RIGHT,
                        GTCB_Checked,config.plugin_cache,TAG_IGNORE,0);g[G_PLUGIN_CACHE]=last;
        last=add_tagged(INTEGER_KIND,last,G_RESULTS_MAX,right+150,130,50,14,
                        pstr(PMSG_RESULTS_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.results_max,GTIN_MaxChars,2);g[G_RESULTS_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_BROWSE_MAX,right+330,130,50,14,
                        pstr(PMSG_BROWSE_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.browse_max,GTIN_MaxChars,3);g[G_BROWSE_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_HIDDEN_MAX,right+150,146,50,14,
                        pstr(PMSG_HIDDEN_MAX),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.hidden_max,GTIN_MaxChars,3);g[G_HIDDEN_MAX]=last;
        last=add_tagged(INTEGER_KIND,last,G_RECENT_DOCS,right+330,146,50,14,
                        pstr(PMSG_RECENT_DOCS),PLACETEXT_LEFT,
                        GTIN_Number,(ULONG)config.recent_docs_max,GTIN_MaxChars,2);g[G_RECENT_DOCS]=last;
        /* Fields end at right+270 (x 460); the buttons start at x 484 on
           the narrowest (640 pixel) window, above the checkbox rows. */
        if (AslBase) {
            last=add_gadget(BUTTON_KIND,last,G_PRESET_EXPORT,w-150,44,130,15,
                            pstr(PMSG_PRESET_EXPORT),PLACETEXT_IN);g[G_PRESET_EXPORT]=last;
            last=add_gadget(BUTTON_KIND,last,G_PRESET_IMPORT,w-150,61,130,15,
                            pstr(PMSG_PRESET_IMPORT),PLACETEXT_IN);g[G_PRESET_IMPORT]=last;
        }
        last=add_gadget(BUTTON_KIND,last,G_CATALOGUE,w-150,78,130,15,
                        pstr(PMSG_CAT_BUTTON),PLACETEXT_IN);g[G_CATALOGUE]=last;
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
        last=add_tagged(BUTTON_KIND,last,G_MAIN_START,right,button_y,56,17,
                        pstr(PMSG_START),PLACETEXT_IN,GA_Disabled,launcher_running,TAG_IGNORE,0);
        g[G_MAIN_START]=last;
        last=add_tagged(BUTTON_KIND,last,G_MAIN_RESTART,right+60,button_y,76,17,
                        pstr(PMSG_RESTART),PLACETEXT_IN,GA_Disabled,!launcher_running,TAG_IGNORE,0);
        g[G_MAIN_RESTART]=last;
        last=add_tagged(BUTTON_KIND,last,G_MAIN_STOP,right+140,button_y,56,17,
                        pstr(PMSG_STOP),PLACETEXT_IN,GA_Disabled,!launcher_running,TAG_IGNORE,0);
        g[G_MAIN_STOP]=last;
    }
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
        config.window_width=(int)gadget_number(G_WIDTH);
        config.window_height=(int)gadget_number(G_HEIGHT);
        if (g[G_PREVIEW]) config.icon_preview=!!(g[G_PREVIEW]->Flags&GFLG_SELECTED);
        if (selection>=0 && selection<BEAM_THEME_COUNT) config.theme=selection;
        if (g[G_CUSTOM_COLORS]) {
            si=(struct StringInfo *)g[G_CUSTOM_COLORS]->SpecialInfo;
            custom_colors_valid=si && si->Buffer &&
                beam_theme_parse_colors((char *)si->Buffer,config.custom_colors);
        }
        if (g[G_THEME_PATH]) {
            si=(struct StringInfo *)g[G_THEME_PATH]->SpecialInfo;
            if (si && si->Buffer)
                beam_copy(theme_file_path,beam_trim((char *)si->Buffer),sizeof(theme_file_path));
        }
    } else if (page==PAGE_DIRS) {
        struct StringInfo *si;
        char work[512],*p,*next;
        config.max_depth=(int)gadget_number(G_DEPTH);
        if (g[G_ICONS]) config.scan_icons=!!(g[G_ICONS]->Flags&GFLG_SELECTED);
        if (g[G_EXES]) config.scan_executables=!!(g[G_EXES]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_FILES]) config.index_files=!!(g[G_INDEX_FILES]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_DRAWERS]) config.index_drawers=!!(g[G_INDEX_DRAWERS]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_GAMES]) config.index_games=!!(g[G_INDEX_GAMES]->Flags&GFLG_SELECTED);
        si=g[G_FILE_PATTERNS]?(struct StringInfo *)g[G_FILE_PATTERNS]->SpecialInfo:NULL;
        if (si && si->Buffer)
            beam_copy(config.file_patterns,beam_trim((char *)si->Buffer),sizeof(config.file_patterns));
        config.excluded_dir_count=0;
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
        config.row_spacing=(int)gadget_number(G_ROW_SPACING);
        if (g[G_HIGH_CONTRAST]) config.high_contrast=!!(g[G_HIGH_CONTRAST]->Flags&GFLG_SELECTED);
        if (g[G_LARGE_GADGETS]) config.large_gadgets=!!(g[G_LARGE_GADGETS]->Flags&GFLG_SELECTED);
        if (g[G_REDUCED_COLORS]) config.reduced_colors=!!(g[G_REDUCED_COLORS]->Flags&GFLG_SELECTED);
    } else if (page==PAGE_SOURCES) {
        char *text;
        if (g[G_INDEX_MODULES]) config.index_modules=!!(g[G_INDEX_MODULES]->Flags&GFLG_SELECTED);
        if (g[G_INDEX_REXX]) config.index_rexx=!!(g[G_INDEX_REXX]->Flags&GFLG_SELECTED);
        if ((text=gadget_text(G_MODULE_PORT)))
            beam_copy(config.module_port,beam_trim(text),sizeof(config.module_port));
        if ((text=gadget_text(G_MODULE_AREXX)))
            beam_copy(config.module_arexx,beam_trim(text),sizeof(config.module_arexx));
        if ((text=gadget_text(G_MODULE_COMMAND)) && *beam_trim(text))
            beam_copy(config.module_command,beam_trim(text),sizeof(config.module_command));
        if ((text=gadget_text(G_BOOKMARK_FILES)))
            beam_copy(config.bookmark_files,beam_trim(text),sizeof(config.bookmark_files));
        if ((text=gadget_text(G_URL_COMMAND)) && *beam_trim(text))
            beam_copy(config.url_command,beam_trim(text),sizeof(config.url_command));
    } else if (page==PAGE_EXTRAS) {
        char *text;
        if ((text=gadget_text(G_QUICK_LAUNCH)))
            beam_copy(config.quick_launch,beam_trim(text),sizeof(config.quick_launch));
        if ((text=gadget_text(G_WHDLOAD_CMD)) && *beam_trim(text))
            beam_copy(config.whdload_command,beam_trim(text),sizeof(config.whdload_command));
        if ((text=gadget_text(G_DOPUS_PORT)) && *beam_trim(text))
            beam_copy(config.dopus_port,beam_trim(text),sizeof(config.dopus_port));
        if (g[G_DOPUS_DRAWERS]) config.dopus_drawers=!!(g[G_DOPUS_DRAWERS]->Flags&GFLG_SELECTED);
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
static void browse_path(void)
{
    struct FileRequester *fr;
    struct Gadget *target=(page==PAGE_ADVANCED)?g[G_APP_PATH]:g[G_ENTRY];
    char path[BEAM_MAX_PATH];
    if (!AslBase || !target) { DisplayBeep(screen); return; }
    fr=AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)((page==PAGE_DIRS)?pstr(PMSG_SELECT_DIRECTORY):
                     (page==PAGE_PLUGINS)?pstr(PMSG_SELECT_PLUGIN):pstr(PMSG_SELECT_APPLICATION)),
        ASLFR_Window,(ULONG)win,
        ASLFR_PrivateIDCMP,TRUE, /* do not consume GadTools parent IDCMP */
        ASLFR_DrawersOnly,(ULONG)(page==PAGE_DIRS),
        TAG_DONE);
    if (!fr) { DisplayBeep(screen); return; }
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
        if (*path)
            GT_SetGadgetAttrs(target,win,NULL,GTST_String,(ULONG)path,TAG_DONE);
        else DisplayBeep(screen);
    }
    FreeAslRequest(fr);
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
    for (i=0;i<BEAM_THEME_COLOR_COUNT;i++) colours[i]=config.custom_colors[i];
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
    for (i=0;i<BEAM_THEME_COLOR_COUNT;i++) config.custom_colors[i]=colours[i];
    custom_colors_valid=1;
    beam_theme_format_colors(config.custom_colors,custom_colors_text,sizeof(custom_colors_text));
    if (g[G_CUSTOM_COLORS])
        GT_SetGadgetAttrs(g[G_CUSTOM_COLORS],win,NULL,GTST_String,(ULONG)custom_colors_text,TAG_DONE);
    draw_theme_preview();
}

/* Append a hotlist file to BOOKMARK_FILES (';'-separated). */
static int ask_file(const char *title,int save,char *path,unsigned int cap);
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

/* ------------------------------------------------------------- presets */
static int ask_file(const char *title,int save,char *path,unsigned int cap)
{
    struct FileRequester *fr;
    int ok=0;
    path[0]=0;
    if (!AslBase) return 0;
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)title,ASLFR_Window,(ULONG)win,
        ASLFR_PrivateIDCMP,TRUE,ASLFR_DoSaveMode,(ULONG)save,
        ASLFR_InitialPattern,(ULONG)"#?.preset",ASLFR_DoPatterns,TRUE,
        ASLFR_InitialFile,(ULONG)(save?"Beam68.preset":""),TAG_DONE);
    if (!fr) return 0;
    if (AslRequest(fr,NULL) && fr->fr_File && *fr->fr_File && fr->fr_Drawer &&
        strlen((const char *)fr->fr_Drawer)+strlen((const char *)fr->fr_File)+2<cap) {
        beam_copy(path,(const char *)fr->fr_Drawer,cap);
        ok=AddPart((STRPTR)path,fr->fr_File,(LONG)cap)!=0;
    }
    FreeAslRequest(fr);
    return ok;
}

/* Saved settings (ENVARC:) plus every plugin settings file. */
static void export_preset(void)
{
    static char extra[BEAM_MAX_PLUGINS][64];
    const char *names[4+BEAM_MAX_PLUGINS];
    char path[BEAM_MAX_PATH];
    int n=0,i,written;
    if (!ask_file(pstr(PMSG_PRESET_EXPORT),1,path,sizeof(path))) return;
    names[n++]="Beam68.cfg"; names[n++]="Favorites.txt";
    names[n++]="Aliases.txt"; names[n++]="Hidden.txt";
    for (i=0;i<config.plugin_file_count && i<BEAM_MAX_PLUGINS;i++) {
        const char *s=plugin_metadata[i].settings;
        if (strlen(s)>22 && !strncmp(s,"ENVARC:Beam68/Plugins/",22) && strlen(s+22)<50) {
            sprintf(extra[i],"Plugins/%s",s+22);
            names[n++]=extra[i];
        }
    }
    written=beam_preset_export(path,"ENVARC:Beam68",names,n);
    set_title(pstr(written>0?PMSG_PRESET_EXPORTED:PMSG_PRESET_FAILED));
}

static void import_preset(void)
{
    static const char *dirs[]={"ENVARC:Beam68","ENV:Beam68"};
    struct EasyStruct es;
    char path[BEAM_MAX_PATH];
    APTR args[1];
    if (!ask_file(pstr(PMSG_PRESET_IMPORT),0,path,sizeof(path))) return;
    es.es_StructSize=sizeof(es); es.es_Flags=0;
    es.es_Title=(UBYTE *)pstr(PMSG_PRESET_IMPORT);
    es.es_TextFormat=(UBYTE *)pstr(PMSG_PRESET_CONFIRM);
    es.es_GadgetFormat=(UBYTE *)pstr(PMSG_PRESET_BUTTONS);
    args[0]=(APTR)path;
    if (EasyRequestArgs(win,&es,NULL,args)!=1) return;
    if (beam_preset_import(path,dirs,2)<=0) { set_title(pstr(PMSG_PRESET_FAILED)); return; }
    /* Show the imported settings and let a running launcher apply them. */
    beam_config_defaults(&config);
    beam_config_read_active(&config);
    beam_favorites_load_active(&favorites);
    beam_aliases_load_active(&aliases);
    if (beam_control_running()) beam_control_send(BEAM_CTL_RELOAD);
    close_window(); if (!build_window()) done=1;
    if (!done) set_title(pstr(PMSG_PRESET_IMPORTED));
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
        for (i=selection;i<n-1;i++) beam_copy(data[i],data[i+1],BEAM_MAX_PATH);
        --n;
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
        if (n>=max) { DisplayBeep(screen); return; }
        for (i=0;i<n;i++) if (beam_equal(path,data[i])) return;
        beam_copy(data[n++],path,BEAM_MAX_PATH);
    }
    if (page==PAGE_DIRS) config.dir_count=n;
    else config.plugin_file_count=n;
    collect_page();
    close_window(); if (!build_window()) done=1;
    if (!done && page==PAGE_PLUGINS && install_message[0]) {
        set_title(install_message); install_message[0]=0;
    }
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
    if (beam_control_send_theme((unsigned long)config.theme,
                                config.custom_colors))
        theme_preview_changed=1;
}

static int import_theme_file(const char *path)
{
    unsigned long colors[BEAM_THEME_COLOR_COUNT];
    int i;
    if (!path || !beam_theme_file_load(path,colors)) return 0;
    /* Commit only after the whole file has passed signature and color
       validation, so an unreadable drop cannot damage the active palette. */
    for (i=0;i<BEAM_THEME_COLOR_COUNT;i++) config.custom_colors[i]=colors[i];
    config.theme=BEAM_THEME_CUSTOM;selection=BEAM_THEME_CUSTOM;
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
    collect_page();
    if (!path || !beam_theme_file_save(beam_trim(path),config.custom_colors)) {
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
    if (selection!=BEAM_THEME_WORKBENCH && screen->ViewPort.ColorMap) {
        tags[0].ti_Tag=OBP_Precision; tags[0].ti_Data=PRECISION_GUI;
        tags[1].ti_Tag=TAG_END; tags[1].ti_Data=0;
        for (role=0;role<BEAM_THEME_ROLE_COUNT;role++) {
            LONG pen;
            if (!beam_theme_color_custom(selection,role,config.custom_colors,
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
        unsigned long colors[BEAM_THEME_COLOR_COUNT];int role;
        if (!beam_theme_file_load(candidate,colors)) return 0;
        for(role=0;role<BEAM_THEME_COLOR_COUNT;role++)config.custom_colors[role]=colors[role];
        config.theme=BEAM_THEME_CUSTOM;selection=BEAM_THEME_CUSTOM;
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
    selection=target;
    close_window();
    if (!build_window()) done=1;
}
/* A short title updates on selection without opening a requester. Details
 * uses EasyRequest for a richer, read-only diagnostics panel. */
static void show_selected_plugin(void)
{
    const char *name;
    if (!win || page!=PAGE_PLUGINS || plugin_settings_mode || selection<0 ||
        selection>=config.plugin_file_count) return;
    name=plugin_metadata[selection].name[0]?
        plugin_metadata[selection].name:config.plugin_files[selection];
    sprintf(detail_title,"Beam68 - %.65s : %s",name,
            beam_plugin_diagnostic(plugin_states[selection]));
    SetWindowTitles(win,(STRPTR)detail_title,(STRPTR)~0UL);
}
static void plugin_details(void)
{
    struct EasyStruct es;
    ULONG args[1];
    static char body[1800];
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
    /* Three translatable parts; each is bounded by its %.Ns fields. */
    sprintf(body,pstr(PMSG_DETAILS1),
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
    strcat(body,"\n\n");
    strcat(body,beam_plugin_advice(plugin_states[selection]));
    es.es_StructSize=sizeof(es);
    es.es_Flags=0;
    es.es_Title=(STRPTR)pstr(PMSG_DETAILS_TITLE);
    es.es_TextFormat=(STRPTR)"%s";
    {
        const char *cursor=body;
        static char text_page[520];
        do {
            unsigned int lines=0,used=0;
            while (*cursor && lines<8) {
                unsigned int next,n=beam_text_line(cursor,60,&next);
                if (!next) break;
                memcpy(text_page+used,cursor,n);used+=n;
                text_page[used++]='\n';cursor+=next;++lines;
            }
            text_page[used]=0;
            es.es_GadgetFormat=(STRPTR)(*cursor?"Next|Close":pstr(PMSG_OK));
            args[0]=(ULONG)text_page;
            if (EasyRequestArgs(win,&es,NULL,args)!=1) break;
        } while (*cursor);
    }
}

static void export_diagnostics(void)
{
    BPTR f;
    char line[512],health_text[160];
    int i;
    const struct BeamPluginHealthEntry *health;
    collect_page();
    beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    f=Open((STRPTR)"RAM:Beam68-Diagnostics.txt",MODE_NEWFILE);
    if (!f) { DisplayBeep(screen);set_title(pstr(PMSG_REPORT_FAILED));return; }
    sprintf(line,"Beam68 0.1 diagnostics\nlauncher=%s hotkey=%s\nitems=%ld depth=%ld theme=%s shell=%s\nplugins=%ld timeout=%lds\n",
        beam_control_running()?"running":"stopped",config.hotkey,
        (long)config.max_items,(long)config.max_depth,beam_theme_name(config.theme),
        config.shell_commands?"on":"off",(long)config.plugin_file_count,
        (long)config.plugin_timeout);
    FPuts(f,(STRPTR)line);
    for (i=0;i<config.plugin_file_count;i++) {
        health=beam_plugin_health_find(&plugin_health,config.plugin_files[i]);
        if (!beam_plugin_health_format(health,health_text,sizeof(health_text)))
            beam_copy(health_text,"no health history",sizeof(health_text));
        sprintf(line,"%ld %s name=%.48s category=%.24s prefix=%.20s caps=%.60s path=%.120s | %.150s\n",
            (long)i+1,beam_plugin_diagnostic(plugin_states[i]),
            plugin_metadata[i].name[0]?plugin_metadata[i].name:pstr(PMSG_UNKNOWN),
            plugin_metadata[i].category[0]?plugin_metadata[i].category:"-",
            plugin_metadata[i].prefix[0]?plugin_metadata[i].prefix:"-",
            plugin_metadata[i].capabilities[0]?plugin_metadata[i].capabilities:"-",
            config.plugin_files[i],health_text);
        FPuts(f,(STRPTR)line);
    }
    if (!Close(f)) { DisplayBeep(screen);set_title(pstr(PMSG_REPORT_CLOSE_FAILED));return; }
    set_title(pstr(PMSG_REPORT_EXPORTED));
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
    if (!apply_plugin_setting_value()) return 0;
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
    return 1;
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
        load_plugin_setting_values();plugin_settings_mode=1;selection=0;
        close_window();if(!build_window())done=1;return;
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
    if (next==page) return;
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
        "PREFS_EXTRAS","PREFS_SOURCES"};
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
    int restart_recovery=0;
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
    } else beam_control_send(BEAM_CTL_RELOAD); /* safe when not running */
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
            GT_BeginRefresh(win); GT_EndRefresh(win,TRUE);
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
            if (!save_plugin_settings()) {
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
        else if (id==G_RELOAD_PLUGINS) {
            if (!beam_control_send(BEAM_CTL_RELOAD_PLUGINS)) {
                DisplayBeep(screen);set_title(pstr(PMSG_NOT_RUNNING));
            } else set_title(pstr(PMSG_RELOAD_REQUESTED));
        }
        else if (id==G_EXPORT_DIAGNOSTICS) export_diagnostics();
        else if (id==G_THEME_IMPORT) import_theme_gadget();
        else if (id==G_FONT_CHOOSE) choose_font();
        else if (id==G_PRESET_EXPORT) export_preset();
        else if (id==G_EDIT_COLOURS) edit_colours();
        else if (id==G_BOOKMARK_BROWSE) add_bookmark_file();
        else if (id==G_CATALOGUE) {
            collect_page();
            if (beam_catalogue_window(screen,visual,&config,import_theme_file,
                                      install_message,sizeof(install_message))) {
                close_window(); if (!build_window()) done=1;
            }
            if (!done && install_message[0]) { set_title(install_message); install_message[0]=0; }
        }
        else if (id==G_PRESET_IMPORT) import_preset();
        else if (id==G_THEME_EXPORT) export_theme_gadget();
        else if (id==G_CAPTURE) begin_hotkey_capture();
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
                selection=BEAM_THEME_CUSTOM;
                config.theme=selection;
                GT_SetGadgetAttrs(g[G_THEME_CYCLE],win,NULL,
                    GTCY_Active,(ULONG)selection,TAG_DONE);
                show_selected_appearance();
            }
        }
        else if (id==G_USE) store(0);
        else if (id==G_SAVE) store(1);
        /* GadTools may clear the visual inversion when focus moves from a
         * ListView to a button. Re-assert the selected ordinal after every
         * plugin-page event so Details/Settings/Toggle never leave the user
         * wondering which row those actions target. */
        if (id!=G_LIST && !done && win && page==PAGE_PLUGINS && !plugin_settings_mode &&
            selection>=0 && selection<config.plugin_file_count && g[G_LIST])
            GT_SetGadgetAttrs(g[G_LIST],win,NULL,
                GTLV_Selected,(ULONG)selection,
                GTLV_MakeVisible,(ULONG)selection,TAG_DONE);
    }
}

static int prefs_main(int argc,char **argv)
{
    int ok=0;
    const char *startup_failure="Required libraries could not be opened (Intuition, graphics, GadTools and keymap). AmigaOS 3.0+ is required.";
    (void)argc; (void)argv;
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
    /* Every PROGDIR:Plugins/<Drawer>/<Name>.plugin on disk is listed;
       newly found ones start disabled. */
    beam_catalog_merge_discovered(&config,(const char (*)[BEAM_MAX_PATH])discovered_specs,
        beam_plugin_scan_installed(&config,discovered_specs,BEAM_MAX_PLUGINS));
    beam_catalog_discover_bundled(&config);
    initial_config=config;
    beam_favorites_load_active(&favorites);
    beam_aliases_load_active(&aliases);
    beam_plugin_health_load(&plugin_health,BEAM_PLUGIN_HEALTH_PATH);
    startup_failure="No public screen is available. Open Workbench and try again.";
    screen=LockPubScreen(NULL);
    if (!screen) goto cleanup;
    startup_failure="Not enough memory for Preferences screen resources.";
    visual=GetVisualInfoA(screen,NULL);
    if (!visual) goto cleanup;
    startup_failure="Could not open Preferences. Free memory or use a larger public screen.";
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
        beam_control_send_theme((unsigned long)initial_config.theme,
                                initial_config.custom_colors);
    close_window();
    if (app_port) { DeleteMsgPort(app_port); app_port=NULL; }
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (visual) FreeVisualInfo(visual);
    if (screen) UnlockPubScreen(NULL,screen);
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
