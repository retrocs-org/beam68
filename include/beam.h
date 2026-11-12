#ifndef BEAM68_H
#define BEAM68_H
/* Release version, compared against a plugin manifest's MIN_BEAM68. */
#define BEAM68_VERSION "0.1"
#define BEAM_MAX_DIRS 16
#define BEAM_MAX_PLUGINS 16
#define BEAM_MAX_ITEMS_LIMIT 2048
#define BEAM_MAX_PATH 240
#define BEAM_MAX_NAME 64
#define BEAM_QUERY_LEN 120
#define BEAM_MAX_VISIBLE 8
#define BEAM_MAX_PLUGIN_SETTINGS 6
#define BEAM_MAX_EXCLUDED_DIRS 8
#define BEAM_FILE_PATTERNS_LEN 96
#define BEAM_FONT_NAME_LEN 64
#define BEAM_PLUGIN_SETTING_TEXT 1
#define BEAM_PLUGIN_SETTING_NUMBER 2
#define BEAM_PLUGIN_SETTING_BOOL 3
#define BEAM_PLUGIN_SETTING_CHOICE 4

#define BEAM_THEME_WORKBENCH 0
#define BEAM_THEME_LATTE 1
#define BEAM_THEME_FRAPPE 2
#define BEAM_THEME_MACCHIATO 3
#define BEAM_THEME_MOCHA 4
#define BEAM_THEME_CUSTOM 5
#define BEAM_THEME_COUNT 6
#define BEAM_THEME_COLOR_COUNT 5

#define BEAM_ITEM_WORKBENCH 1
#define BEAM_ITEM_SHELL 2
#define BEAM_ITEM_FILE 3
#define BEAM_ITEM_DRAWER 4
#define BEAM_ITEM_VOLUME 5
#define BEAM_ITEM_GAME 6     /* WHDLoad game: project icon or .slave */
#define BEAM_ITEM_ASSIGN 7   /* live list only: an AmigaDOS assign */
#define BEAM_ITEM_SCRIPT 8   /* ARexx script in REXX: (run with RX) */
#define BEAM_ITEM_MODULE 9   /* music module (played via MODULE_COMMAND) */
#define BEAM_ITEM_BOOKMARK 10 /* browser bookmark: path is the URL */
#define BEAM_ITEM_SNIPPET 11 /* text snippet: path SNIPPET:<name>, title=text */
#define BEAM_ITEM_KIND_MAX 11

#define BEAM_COMPACT_AUTO 0
#define BEAM_COMPACT_ALWAYS 1
#define BEAM_COMPACT_NEVER 2

/* Paths are stored in a shared string pool (see item_index.h); name points
 * into path.  Per-item ranking metadata is precomputed once so the per-
 * keystroke search does no favorites/history string comparisons. */
#define BEAM_ITEM_META_NONE 0xFF
struct BeamItem {
    char *path;
    const char *name;
    const char *title;       /* optional display/search text (WHDLoad) */
    unsigned long mask;      /* beam_text_mask(name) | alias words */
    unsigned char kind;
    unsigned char root;      /* index into config dirs, or BEAM_ROOT_NONE */
    unsigned char flags;
    unsigned char favorite;  /* pin position, or BEAM_ITEM_META_NONE */
    unsigned char recent;    /* recency position, or BEAM_ITEM_META_NONE */
    unsigned char bonus;     /* bounded launch-count tie-breaker */
    signed char alias_index; /* -1, or index in the bounded alias catalog */
};

struct BeamConfig {
    char hotkey[96];
    /* Where the resident launcher and its PROGDIR: plugins are installed.
       Beam68Prefs has a different PROGDIR: when installed under SYS:Prefs. */
    char install_dir[BEAM_MAX_PATH];
    char dirs[BEAM_MAX_DIRS][BEAM_MAX_PATH];
    int dir_count;
    char excluded_dirs[BEAM_MAX_EXCLUDED_DIRS][BEAM_MAX_PATH];
    int excluded_dir_count;
    char file_patterns[BEAM_FILE_PATTERNS_LEN];
    char plugin_files[BEAM_MAX_PLUGINS][BEAM_MAX_PATH];
    int plugin_file_count;
    /* Paths in this list remain installed but are not loaded. */
    char disabled_plugins[BEAM_MAX_PLUGINS][BEAM_MAX_PATH];
    int disabled_plugin_count;
    int max_items;
    int max_depth;
    int scan_icons;
    int scan_executables;
    int index_files;
    int index_drawers;
    int visible;
    int history;
    int index_cache;
    int plugin_timeout; /* seconds, 5..120; default 30 */
    int icon_preview;     /* optional selected Workbench icon, 0 by default */
    int window_width;    /* 230..1200, capped to public screen */
    int window_height;   /* 0 = based on results, or 90..900 */
    int theme;           /* Workbench, a bundled palette, or Custom */
    unsigned long custom_colors[BEAM_THEME_COLOR_COUNT]; /* 0xRRGGBB roles */
    int shell_commands;  /* allow explicit shell:<command> launcher queries */
    char font_name[BEAM_FONT_NAME_LEN];
    int index_games;     /* WHDLoad icons and .slave files as GAME items */
    char whdload_command[96]; /* program used for icon-less .slave files */
    char quick_launch[48];    /* e.g. "control alt": +1..9 start favorites */
    char dopus_port[24];      /* Directory Opus ARexx port, "DOPUS.1" */
    int dopus_drawers;        /* open drawers in Directory Opus if running */
    int plugin_cache;         /* run plugins from a RAM copy (T:) */
    int results_max;          /* results kept and scrolled (8..64) */
    int browse_max;           /* drawer entries listed when browsing */
    int hidden_max;           /* hidden results remembered */
    int recent_docs_max;      /* recent documents remembered */
    char catalogue_url[200];  /* plugin/theme catalogue index (URL or path) */
    char download_command[120]; /* template with {url} and {file} */
    int index_rexx;           /* ARexx scripts in REXX: */
    int index_modules;        /* music modules in the indexed drawers */
    char bookmark_files[BEAM_MAX_PATH]; /* HTML hotlists, ';'-separated */
    char module_command[120]; /* template with {file} */
    char module_port[32];     /* ARexx port of a running player */
    char module_arexx[120];   /* command sent to it, with {file} */
    char url_command[120];    /* template with {url} */
    int font_size;       /* 6..32 */
    int row_spacing;     /* 2..16 pixels */
    int high_contrast;
    int large_gadgets;
    int reduced_colors;
    int compact_mode;    /* auto, always or never */
};

struct BeamPluginSetting {
    char key[32];
    char label[48];
    char default_value[64];
    char choices[128];
    unsigned char type;
};

struct BeamPlugin {
    char name[BEAM_MAX_NAME];
    char command[BEAM_MAX_PATH];
    char description[96];
    /* Optional plugin-owned plain-text preferences file. */
    char settings[BEAM_MAX_PATH];
    /* Optional case-insensitive launcher prefix, for example FIND:. */
    char prefix[24];
    /* Optional presentation/documentation metadata.  Protocol 1 plugins
       without these keys remain fully compatible. */
    char category[32];
    char icon[BEAM_MAX_PATH];
    char example[96];
    char help[128];
    /* Comma-separated bounded declarations such as FILES,SHELL,AREXX. */
    char capabilities[64];
    /* Optional declarative Preferences fields. The execution ABI remains
       protocol 1; plugins continue to read their own SETTINGS file. */
    struct BeamPluginSetting setting_defs[BEAM_MAX_PLUGIN_SETTINGS];
    unsigned char setting_count;
    char trigger;
    unsigned char protocol;  /* manifest VERSION: 1 (reply file) or 2 (rows) */
    unsigned char preview;   /* PREVIEW=YES: run after a typing pause */
    /* Package metadata (optional for local plugins, required in packages). */
    char id[64];             /* stable unique ID, e.g. org.example.weather */
    char plugin_version[16]; /* dotted numeric, e.g. 1.2 */
    char min_beam68[16];     /* lowest Beam68 release it needs */
    char author[64];
    char license[32];
    char url[128];
    char guide[BEAM_MAX_PATH]; /* optional AmigaGuide help, PLUGINDIR: form */
};

int beam_equal(const char *a, const char *b);
void beam_copy(char *dest, const char *src, unsigned int cap);
char *beam_trim(char *s);
void beam_config_defaults(struct BeamConfig *c);
int beam_config_read(struct BeamConfig *c, const char *file);
int beam_plugin_read(struct BeamPlugin *p, const char *file);
int beam_plugin_has_capability(const struct BeamPlugin *p, const char *name);
int beam_capability_list_has(const char *list, const char *name);
/* PLUGINDIR: in a manifest means "the drawer this manifest is in".
 * Resolves spec against manifest_path; other specs are copied unchanged.
 * Rejects relative parts that could leave the drawer (':' or a leading '/',
 * which is the AmigaDOS parent reference, or "//"). */
int beam_is_plugindir_path(const char *spec);
int beam_plugindir_resolve(const char *manifest_path, const char *spec,
                           char *out, unsigned int cap);
/* Dotted numeric versions ("1", "0.10", "2.3.1"); missing parts count as
 * zero.  Returns <0, 0, >0.  Invalid text sorts lowest. */
int beam_version_compare(const char *a, const char *b);
int beam_version_valid(const char *v);
/* 0.1 shipped plugins as PROGDIR:Plugins/<Name>.plugin; packages live in
 * PROGDIR:Plugins/<Name>/<Name>.plugin.  Rewrites the old form in place. */
void beam_plugin_path_migrate(char *spec, unsigned int cap);
int beam_plugin_enabled(const struct BeamConfig *c, const char *path);
int beam_plugin_set_enabled(struct BeamConfig *c, const char *path, int enabled);
void beam_plugin_forget_disabled(struct BeamConfig *c, const char *path);

/* Version 0.1 config storage paths and persistent settings helpers. */
#define BEAM_CONFIG_ENV "ENV:Beam68/Beam68.cfg"
#define BEAM_CONFIG_ENVARC "ENVARC:Beam68/Beam68.cfg"
#define BEAM_CONFIG_SESSION "T:Beam68/Beam68.cfg"
#define BEAM_CONFIG_DEFAULT "PROGDIR:Beam68.cfg"
#define BEAM_SAFE_MODE_ACTIVE "T:Beam68/SafeMode.active"

int beam_config_read_active(struct BeamConfig *c);
int beam_config_write(struct BeamConfig *c, const char *path);
int beam_config_valid(const struct BeamConfig *c);
#endif
