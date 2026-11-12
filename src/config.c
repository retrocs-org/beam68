#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "beam.h"
#include "appearance.h"

static int ascii_lower(int c)
{ return (c>='A' && c<='Z') ? c+32 : c; }
int beam_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (ascii_lower((unsigned char)*a++) != ascii_lower((unsigned char)*b++)) return 0;
    }
    return !*a && !*b;
}
void beam_copy(char *dst, const char *src, unsigned int capacity)
{
    unsigned int i=0;
    if (!capacity) return;
    while (src[i] && i+1<capacity) { dst[i]=src[i]; ++i; }
    dst[i]=0;
}
char *beam_trim(char *s)
{
    char *start=s; char *end;
    while (*start==' ' || *start=='\t') ++start;
    end=start+strlen(start);
    while (end>start && (end[-1]==' ' || end[-1]=='\t' || end[-1]=='\r' || end[-1]=='\n')) --end;
    *end=0;
    return start;
}
void beam_config_defaults(struct BeamConfig *c)
{
    memset(c,0,sizeof(*c));
    beam_copy(c->hotkey,"rawkey control alt space",sizeof(c->hotkey));
    beam_copy(c->install_dir,"SYS:Utilities/Beam68",sizeof(c->install_dir));
    c->max_items=512; c->max_depth=4; c->scan_icons=1; c->scan_executables=0;
    c->index_files=0; c->index_drawers=1; c->visible=6; c->history=1;
    c->index_cache=1; c->plugin_timeout=30; c->icon_preview=0;
    c->window_width=550; c->window_height=0;
    c->theme=BEAM_THEME_WORKBENCH; c->shell_commands=1;
    beam_copy(c->file_patterns,"*",sizeof(c->file_patterns));
    beam_copy(c->font_name,"topaz.font",sizeof(c->font_name));
    c->font_size=8;c->row_spacing=7;c->high_contrast=0;
    c->large_gadgets=0;c->reduced_colors=0;c->compact_mode=BEAM_COMPACT_AUTO;
    c->index_games=1;
    beam_copy(c->whdload_command,"WHDLoad",sizeof(c->whdload_command));
    beam_copy(c->dopus_port,"DOPUS.1",sizeof(c->dopus_port));
    c->dopus_drawers=0;
    c->plugin_cache=0;
    c->results_max=32; c->browse_max=128; c->hidden_max=32; c->recent_docs_max=16;
    c->catalogue_url[0]=0;
    beam_copy(c->download_command,"wget -q -O \"{file}\" \"{url}\"",sizeof(c->download_command));
    c->index_rexx=1; c->index_modules=0; c->bookmark_files[0]=0;
    beam_copy(c->module_command,"Run >NIL: HippoPlayer \"{file}\"",sizeof(c->module_command));
    beam_copy(c->module_port,"HIPPOPLAYER",sizeof(c->module_port));
    beam_copy(c->module_arexx,"LOAD \"{file}\"",sizeof(c->module_arexx));
    beam_copy(c->url_command,"OpenURL \"{url}\"",sizeof(c->url_command));
    c->custom_colors[BEAM_THEME_ROLE_TEXT]=0xcdd6f4UL;
    c->custom_colors[BEAM_THEME_ROLE_SHADOW]=0x585b70UL;
    c->custom_colors[BEAM_THEME_ROLE_FILL]=0xf38ba8UL;
    c->custom_colors[BEAM_THEME_ROLE_FILL_TEXT]=0x1e1e2eUL;
    c->custom_colors[BEAM_THEME_ROLE_BACKGROUND]=0x11111bUL;
    beam_copy(c->dirs[c->dir_count++],"SYS:Tools",BEAM_MAX_PATH);
    beam_copy(c->dirs[c->dir_count++],"SYS:Utilities",BEAM_MAX_PATH);
    beam_copy(c->dirs[c->dir_count++],"SYS:Prefs",BEAM_MAX_PATH);
    beam_copy(c->dirs[c->dir_count++],"SYS:System",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/Calc/Calc.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/Clock/Clock.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/Memory/Memory.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/FileFinder.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/SystemCommands.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/DosTools.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/TaskManager.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/ClipboardTools.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/DateUtilities.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/WindowSwitcher.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/DeveloperTools.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/ARexxRunner.plugin",BEAM_MAX_PATH);
    beam_copy(c->plugin_files[c->plugin_file_count++],"PROGDIR:Plugins/MusicControls.plugin",BEAM_MAX_PATH);
}

static int config_theme(const char *name)
{
    if (beam_equal(name,"LATTE")) return BEAM_THEME_LATTE;
    if (beam_equal(name,"FRAPPE")) return BEAM_THEME_FRAPPE;
    if (beam_equal(name,"MACCHIATO")) return BEAM_THEME_MACCHIATO;
    if (beam_equal(name,"MOCHA")) return BEAM_THEME_MOCHA;
    if (beam_equal(name,"CUSTOM")) return BEAM_THEME_CUSTOM;
    return BEAM_THEME_WORKBENCH;
}

static const char *config_theme_name(int theme)
{
    if (theme==BEAM_THEME_LATTE) return "LATTE";
    if (theme==BEAM_THEME_FRAPPE) return "FRAPPE";
    if (theme==BEAM_THEME_MACCHIATO) return "MACCHIATO";
    if (theme==BEAM_THEME_MOCHA) return "MOCHA";
    if (theme==BEAM_THEME_CUSTOM) return "CUSTOM";
    return "WORKBENCH";
}

/* Disablement is keyed by path rather than by list position. Hand-edited
 * PLUGIN_DISABLED lines may appear before or after PLUGIN entries. */
int beam_plugin_enabled(const struct BeamConfig *c, const char *path)
{
    int i;
    if (!c || !path) return 0;
    for (i=0;i<c->disabled_plugin_count;i++)
        if (beam_equal(c->disabled_plugins[i],path)) return 0;
    return 1;
}
void beam_plugin_forget_disabled(struct BeamConfig *c, const char *path)
{
    int i,j;
    if (!c || !path) return;
    for (i=0;i<c->disabled_plugin_count;) {
        if (!beam_equal(c->disabled_plugins[i],path)) { i++; continue; }
        for (j=i;j<c->disabled_plugin_count-1;j++)
            beam_copy(c->disabled_plugins[j],c->disabled_plugins[j+1],BEAM_MAX_PATH);
        --c->disabled_plugin_count;
    }
}
int beam_plugin_set_enabled(struct BeamConfig *c, const char *path, int enabled)
{
    int i,found=0;
    if (!c || !path || !*path || strlen(path)>=BEAM_MAX_PATH ||
        strchr(path,'\r') || strchr(path,'\n')) return 0;
    for (i=0;i<c->plugin_file_count;i++)
        if (beam_equal(c->plugin_files[i],path)) { found=1; break; }
    if (!found) return 0;
    if (enabled) { beam_plugin_forget_disabled(c,path); return 1; }
    if (!beam_plugin_enabled(c,path)) return 1;
    if (c->disabled_plugin_count>=BEAM_MAX_PLUGINS) return 0;
    beam_copy(c->disabled_plugins[c->disabled_plugin_count++],path,BEAM_MAX_PATH);
    return 1;
}

static int read_line(BPTR f, char *line, unsigned int cap)
{
    /* Reject overlong lines: never silently accept a truncated file path. */
    unsigned int len;
    if (!FGets(f,line,(LONG)cap)) return 0;
    len=(unsigned int)strlen(line);
    if (len && line[len-1]!='\n' && len+1>=cap) {
        char ch;
        while (FRead(f,&ch,1,1)==1 && ch!='\n') { }
        line[0]=0;
    }
    return 1;
}

static int plugin_setting_key_valid(const char *s)
{
    int n=0;
    while (s && *s) {
        int c=(unsigned char)*s++;
        if (!((c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='_')) return 0;
        if (++n>=32) return 0;
    }
    return n>0;
}

static int capability_name_valid(const char *s,unsigned int n)
{
    unsigned int i;
    if (!n || n>=20) return 0;
    for (i=0;i<n;i++)
        if (!((s[i]>='A'&&s[i]<='Z') || s[i]=='_')) return 0;
    return 1;
}

static int capabilities_valid(const char *s)
{
    const char *start=s;
    if (!s || !*s) return 1;
    while (1) {
        while (*s && *s!=',') ++s;
        if (!capability_name_valid(start,(unsigned int)(s-start))) return 0;
        if (!*s) return 1;
        ++s;start=s;
    }
}

int beam_capability_list_has(const char *list,const char *name)
{
    const char *s,*end;
    unsigned int n;
    if (!list || !name || !*name) return 0;
    n=(unsigned int)strlen(name);s=list;
    while (*s) {
        end=strchr(s,',');if (!end) end=s+strlen(s);
        if ((unsigned int)(end-s)==n) {
            unsigned int i;int same=1;
            for (i=0;i<n;i++) {
                int a=(unsigned char)s[i],b=(unsigned char)name[i];
                if (a>='a'&&a<='z') a-=32;
                if (b>='a'&&b<='z') b-=32;
                if (a!=b) {same=0;break;}
            }
            if (same) return 1;
        }
        s=*end?end+1:end;
    }
    return 0;
}
static int prefix_ci(const char *text,const char *prefix)
{
    while (*prefix) if (ascii_lower((unsigned char)*text++)!=
                         ascii_lower((unsigned char)*prefix++)) return 0;
    return 1;
}

int beam_is_plugindir_path(const char *spec)
{
    return spec && strlen(spec)>=10 && prefix_ci(spec,"PLUGINDIR:");
}

int beam_plugindir_resolve(const char *manifest_path,const char *spec,
                           char *out,unsigned int cap)
{
    const char *rel,*p,*cut=NULL;
    unsigned int dir_len,rel_len;
    if (!spec || !out || !cap) return 0;
    out[0]=0;
    if (!beam_is_plugindir_path(spec)) {
        if (strlen(spec)+1>cap) return 0;
        beam_copy(out,spec,cap);
        return 1;
    }
    rel=spec+10;
    if (!manifest_path || !*rel || strchr(rel,':') || rel[0]=='/' ||
        strstr(rel,"//") || strchr(rel,'"') || strchr(rel,'*') ||
        strchr(rel,'\n') || strchr(rel,'\r')) return 0;
    for (p=manifest_path;*p;p++) if (*p=='/' || *p==':') cut=p;
    if (!cut) return 0;
    /* Keep a trailing ':' ("Work:x.plugin" -> "Work:"); drop a '/'. */
    dir_len=(unsigned int)(cut-manifest_path)+(*cut==':'?1U:0U);
    rel_len=(unsigned int)strlen(rel);
    if (dir_len+1+rel_len+1>cap) return 0;
    memcpy(out,manifest_path,dir_len);
    if (*cut=='/') out[dir_len++]='/';
    memcpy(out+dir_len,rel,rel_len+1);
    return 1;
}

int beam_version_valid(const char *v)
{
    int digits=0;
    if (!v || !*v || strlen(v)>15) return 0;
    for (;*v;v++) {
        if (*v>='0' && *v<='9') ++digits;
        else if (*v=='.' && digits) digits=0;
        else return 0;
    }
    return digits>0;
}

int beam_version_compare(const char *a,const char *b)
{
    int va=beam_version_valid(a),vb=beam_version_valid(b);
    if (!va || !vb) return va-vb;
    while (*a || *b) {
        const char *ae=a,*be=b,*az,*bz;
        unsigned int an,bn;
        int order;
        while (*ae>='0' && *ae<='9') ++ae;
        while (*be>='0' && *be<='9') ++be;
        az=a; bz=b;
        while (az<ae && *az=='0') ++az;
        while (bz<be && *bz=='0') ++bz;
        an=(unsigned int)(ae-az); bn=(unsigned int)(be-bz);
        if (an!=bn) return an<bn?-1:1;
        order=an?memcmp(az,bz,an):0;
        if (order) return order<0?-1:1;
        a=*ae=='.'?ae+1:ae; b=*be=='.'?be+1:be;
    }
    return 0;
}

void beam_plugin_path_migrate(char *spec,unsigned int cap)
{
    char name[BEAM_MAX_NAME],out[BEAM_MAX_PATH];
    const char *rest;
    unsigned int n;
    if (!spec || !prefix_ci(spec,"PROGDIR:Plugins/")) return;
    rest=spec+16;
    if (strchr(rest,'/') || strchr(rest,':')) return; /* already a drawer */
    n=(unsigned int)strlen(rest);
    if (n<=7 || !beam_equal(rest+n-7,".plugin") || n-7>=sizeof(name)) return;
    memcpy(name,rest,n-7); name[n-7]=0;
    if (16+(n-7)+1+n+1>sizeof(out)) return;
    sprintf(out,"PROGDIR:Plugins/%s/%s",name,rest);
    if (strlen(out)+1<=cap) beam_copy(spec,out,cap);
}

int beam_plugin_has_capability(const struct BeamPlugin *p,const char *name)
{
    return p?beam_capability_list_has(p->capabilities,name):0;
}

/* SETTING=KEY|Label|TYPE|Default|choices,comma,separated
   TYPE is TEXT, NUMBER, BOOL or CHOICE. Choices are used only by CHOICE. */
static int plugin_setting_parse(struct BeamPluginSetting *d,char *value)
{
    char *parts[5],*p=value;
    int i,count=0;
    memset(d,0,sizeof(*d));
    while (count<5) {
        parts[count++]=p;
        p=strchr(p,'|');
        if (!p) break;
        *p++=0;
    }
    if (count<4 || strchr(parts[count-1],'|')) return 0;
    for (i=0;i<count;i++) parts[i]=beam_trim(parts[i]);
    if (!plugin_setting_key_valid(parts[0]) || !*parts[1] ||
        strlen(parts[1])>=sizeof(d->label) ||
        strlen(parts[3])>=sizeof(d->default_value)) return 0;
    if (beam_equal(parts[2],"TEXT")) d->type=BEAM_PLUGIN_SETTING_TEXT;
    else if (beam_equal(parts[2],"NUMBER")) d->type=BEAM_PLUGIN_SETTING_NUMBER;
    else if (beam_equal(parts[2],"BOOL")) d->type=BEAM_PLUGIN_SETTING_BOOL;
    else if (beam_equal(parts[2],"CHOICE")) d->type=BEAM_PLUGIN_SETTING_CHOICE;
    else return 0;
    if (d->type==BEAM_PLUGIN_SETTING_CHOICE &&
        (count<5 || !*parts[4] || strlen(parts[4])>=sizeof(d->choices))) return 0;
    if (d->type==BEAM_PLUGIN_SETTING_BOOL &&
        !(beam_equal(parts[3],"YES") || beam_equal(parts[3],"NO"))) return 0;
    beam_copy(d->key,parts[0],sizeof(d->key));
    beam_copy(d->label,parts[1],sizeof(d->label));
    beam_copy(d->default_value,parts[3],sizeof(d->default_value));
    if (count>=5) beam_copy(d->choices,parts[4],sizeof(d->choices));
    return 1;
}

int beam_config_read(struct BeamConfig *c, const char *file)
{
    BPTR f;
    char line[512];
    char *eq, *key, *value;
    int n;
    f=Open((STRPTR)file,MODE_OLDFILE);
    if (!f) return 0;
    /* Directory/plugin entries replace defaults, not append to them. */
    c->dir_count=0;
    c->excluded_dir_count=0;
    c->plugin_file_count=0;
    c->disabled_plugin_count=0;
    while (read_line(f,line,sizeof(line))) {
        key=beam_trim(line);
        if (!*key || *key=='#' || *key==';') continue;
        eq=strchr(key,'='); if (!eq) continue;
        *eq=0; value=beam_trim(eq+1); key=beam_trim(key);
        if (beam_equal(key,"HOTKEY")) beam_copy(c->hotkey,value,sizeof(c->hotkey));
        else if (beam_equal(key,"INSTALL_DIR")) beam_copy(c->install_dir,value,sizeof(c->install_dir));
        else if (beam_equal(key,"DIR") && c->dir_count<BEAM_MAX_DIRS && *value)
            beam_copy(c->dirs[c->dir_count++],value,BEAM_MAX_PATH);
        else if (beam_equal(key,"EXCLUDE_DIR") &&
                 c->excluded_dir_count<BEAM_MAX_EXCLUDED_DIRS && *value)
            beam_copy(c->excluded_dirs[c->excluded_dir_count++],value,BEAM_MAX_PATH);
        else if (beam_equal(key,"FILE_PATTERNS"))
            beam_copy(c->file_patterns,value,sizeof(c->file_patterns));
        else if (beam_equal(key,"PLUGIN") && c->plugin_file_count<BEAM_MAX_PLUGINS && *value) {
            beam_copy(c->plugin_files[c->plugin_file_count],value,BEAM_MAX_PATH);
            beam_plugin_path_migrate(c->plugin_files[c->plugin_file_count++],BEAM_MAX_PATH);
        }
        else if (beam_equal(key,"PLUGIN_DISABLED") && c->disabled_plugin_count<BEAM_MAX_PLUGINS && *value) {
            int i, duplicate=0;
            char migrated[BEAM_MAX_PATH];
            if (strlen(value)<BEAM_MAX_PATH) {
                beam_copy(migrated,value,sizeof(migrated));
                beam_plugin_path_migrate(migrated,sizeof(migrated));
                for (i=0;i<c->disabled_plugin_count;i++)
                    if (beam_equal(c->disabled_plugins[i],migrated)) duplicate=1;
                if (!duplicate)
                    beam_copy(c->disabled_plugins[c->disabled_plugin_count++],migrated,BEAM_MAX_PATH);
            }
        }
        else if (beam_equal(key,"PLUGIN_TIMEOUT")) {
            n=atoi(value); if (n>=5 && n<=120) c->plugin_timeout=n;
        }
        else if (beam_equal(key,"INDEX_CACHE")) c->index_cache=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"HISTORY")) c->history=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"SHELL_COMMANDS")) c->shell_commands=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"THEME")) c->theme=config_theme(value);
        else if (beam_equal(key,"CUSTOM_COLORS")) {
            unsigned long colors[BEAM_THEME_COLOR_COUNT];
            int i;
            if (beam_theme_parse_colors(value,colors))
                for (i=0;i<BEAM_THEME_COLOR_COUNT;i++) c->custom_colors[i]=colors[i];
        }
        else if (beam_equal(key,"ICON_PREVIEW")) c->icon_preview=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"SCAN_ICONS")) c->scan_icons=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"SCAN_EXECUTABLES")) c->scan_executables=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"INDEX_FILES")) c->index_files=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"INDEX_DRAWERS")) c->index_drawers=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"INDEX_GAMES")) c->index_games=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"WHDLOAD_COMMAND") && *value) beam_copy(c->whdload_command,value,sizeof(c->whdload_command));
        else if (beam_equal(key,"QUICK_LAUNCH")) beam_copy(c->quick_launch,value,sizeof(c->quick_launch));
        else if (beam_equal(key,"DOPUS_PORT") && *value) beam_copy(c->dopus_port,value,sizeof(c->dopus_port));
        else if (beam_equal(key,"RESULTS_MAX")) c->results_max=atoi(value);
        else if (beam_equal(key,"BROWSE_MAX")) c->browse_max=atoi(value);
        else if (beam_equal(key,"HIDDEN_MAX")) c->hidden_max=atoi(value);
        else if (beam_equal(key,"RECENT_DOCS")) c->recent_docs_max=atoi(value);
        else if (beam_equal(key,"INDEX_REXX")) c->index_rexx=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"INDEX_MODULES")) c->index_modules=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"BOOKMARK_FILES")) beam_copy(c->bookmark_files,value,sizeof(c->bookmark_files));
        else if (beam_equal(key,"MODULE_COMMAND") && *value) beam_copy(c->module_command,value,sizeof(c->module_command));
        else if (beam_equal(key,"MODULE_PORT")) beam_copy(c->module_port,value,sizeof(c->module_port));
        else if (beam_equal(key,"MODULE_AREXX")) beam_copy(c->module_arexx,value,sizeof(c->module_arexx));
        else if (beam_equal(key,"URL_COMMAND") && *value) beam_copy(c->url_command,value,sizeof(c->url_command));
        else if (beam_equal(key,"CATALOGUE_URL")) beam_copy(c->catalogue_url,value,sizeof(c->catalogue_url));
        else if (beam_equal(key,"DOWNLOAD_COMMAND") && *value) beam_copy(c->download_command,value,sizeof(c->download_command));
        else if (beam_equal(key,"PLUGIN_CACHE")) c->plugin_cache=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"OPEN_DRAWERS_IN_DOPUS")) c->dopus_drawers=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"WINDOW_WIDTH")) {
            n=atoi(value); if (n>=230 && n<=1200) c->window_width=n;
        } else if (beam_equal(key,"WINDOW_HEIGHT")) {
            n=atoi(value); if (n==0 || (n>=90 && n<=900)) c->window_height=n;
        } else if (beam_equal(key,"MAX_ITEMS")) {
            n=atoi(value); if (n>=16 && n<=BEAM_MAX_ITEMS_LIMIT) c->max_items=n;
        } else if (beam_equal(key,"MAX_DEPTH")) {
            n=atoi(value); if (n>=0 && n<=16) c->max_depth=n;
        } else if (beam_equal(key,"VISIBLE_RESULTS")) {
            n=atoi(value); if (n>=1 && n<=BEAM_MAX_VISIBLE) c->visible=n;
        } else if (beam_equal(key,"FONT_NAME"))
            beam_copy(c->font_name,value,sizeof(c->font_name));
        else if (beam_equal(key,"FONT_SIZE")) {
            n=atoi(value);if (n>=6 && n<=32) c->font_size=n;
        } else if (beam_equal(key,"ROW_SPACING")) {
            n=atoi(value);if (n>=2 && n<=16) c->row_spacing=n;
        } else if (beam_equal(key,"HIGH_CONTRAST")) c->high_contrast=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"LARGE_GADGETS")) c->large_gadgets=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"REDUCED_COLORS")) c->reduced_colors=beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1");
        else if (beam_equal(key,"COMPACT_MODE"))
            c->compact_mode=beam_equal(value,"YES")?BEAM_COMPACT_ALWAYS:
                            beam_equal(value,"NO")?BEAM_COMPACT_NEVER:BEAM_COMPACT_AUTO;
    }
    Close(f);
    return 1;
}

int beam_plugin_read(struct BeamPlugin *p, const char *file)
{
    BPTR f;
    char line[512]; char *eq, *key, *value;
    int version=0,malformed=0;
    memset(p,0,sizeof(*p));
    f=Open((STRPTR)file,MODE_OLDFILE);
    if (!f) return 0;
    while (read_line(f,line,sizeof(line))) {
        key=beam_trim(line);
        if (!*key || *key=='#' || *key==';') continue;
        eq=strchr(key,'='); if (!eq) continue;
        *eq=0; value=beam_trim(eq+1); key=beam_trim(key);
        if (beam_equal(key,"VERSION")) version=atoi(value);
        else if (beam_equal(key,"NAME")) {
            if (strlen(value)>=sizeof(p->name)) malformed=1;
            else beam_copy(p->name,value,sizeof(p->name));
        } else if (beam_equal(key,"COMMAND")) {
            if (strlen(value)>=sizeof(p->command)) malformed=1;
            else beam_copy(p->command,value,sizeof(p->command));
        } else if (beam_equal(key,"DESCRIPTION")) {
            if (strlen(value)>=sizeof(p->description)) malformed=1;
            else beam_copy(p->description,value,sizeof(p->description));
        } else if (beam_equal(key,"SETTINGS")) {
            if (strlen(value)>=sizeof(p->settings)) malformed=1;
            else beam_copy(p->settings,value,sizeof(p->settings));
        } else if (beam_equal(key,"PREFIX")) {
            if (!*value || strlen(value)>=sizeof(p->prefix) ||
                strchr(value,' ') || strchr(value,'\t')) malformed=1;
            else beam_copy(p->prefix,value,sizeof(p->prefix));
        } else if (beam_equal(key,"CATEGORY")) {
            if (strlen(value)>=sizeof(p->category)) malformed=1;
            else beam_copy(p->category,value,sizeof(p->category));
        } else if (beam_equal(key,"ICON")) {
            if (strlen(value)>=sizeof(p->icon) || strchr(value,'"') ||
                strchr(value,'*') || strchr(value,'\r') || strchr(value,'\n')) malformed=1;
            else beam_copy(p->icon,value,sizeof(p->icon));
        } else if (beam_equal(key,"EXAMPLE")) {
            if (strlen(value)>=sizeof(p->example)) malformed=1;
            else beam_copy(p->example,value,sizeof(p->example));
        } else if (beam_equal(key,"HELP")) {
            if (strlen(value)>=sizeof(p->help)) malformed=1;
            else beam_copy(p->help,value,sizeof(p->help));
        } else if (beam_equal(key,"CAPABILITIES")) {
            if (strlen(value)>=sizeof(p->capabilities) ||
                !capabilities_valid(value)) malformed=1;
            else beam_copy(p->capabilities,value,sizeof(p->capabilities));
        }
        else if (beam_equal(key,"SETTING")) {
            int i,duplicate=0;
            struct BeamPluginSetting d;
            if (p->setting_count>=BEAM_MAX_PLUGIN_SETTINGS ||
                !plugin_setting_parse(&d,value)) malformed=1;
            else {
                for (i=0;i<p->setting_count;i++)
                    if (beam_equal(p->setting_defs[i].key,d.key)) duplicate=1;
                if (duplicate) malformed=1;
                else p->setting_defs[p->setting_count++]=d;
            }
        }
        else if (beam_equal(key,"TRIGGER") && value[0] && !value[1]) p->trigger=value[0];
        else if (beam_equal(key,"ID")) {
            if (strlen(value)>=sizeof(p->id)) malformed=1;
            else beam_copy(p->id,value,sizeof(p->id));
        } else if (beam_equal(key,"PLUGIN_VERSION")) {
            if (!beam_version_valid(value)) malformed=1;
            else beam_copy(p->plugin_version,value,sizeof(p->plugin_version));
        } else if (beam_equal(key,"MIN_BEAM68")) {
            if (!beam_version_valid(value)) malformed=1;
            else beam_copy(p->min_beam68,value,sizeof(p->min_beam68));
        } else if (beam_equal(key,"AUTHOR")) {
            if (strlen(value)>=sizeof(p->author)) malformed=1;
            else beam_copy(p->author,value,sizeof(p->author));
        } else if (beam_equal(key,"LICENSE")) {
            if (strlen(value)>=sizeof(p->license)) malformed=1;
            else beam_copy(p->license,value,sizeof(p->license));
        } else if (beam_equal(key,"PREVIEW")) {
            p->preview=(unsigned char)(beam_equal(value,"YES") || beam_equal(value,"TRUE") || beam_equal(value,"1"));
        } else if (beam_equal(key,"GUIDE")) {
            if (strlen(value)>=sizeof(p->guide) || strchr(value,'"') || strchr(value,'*'))
                malformed=1;
            else beam_copy(p->guide,value,sizeof(p->guide));
        } else if (beam_equal(key,"URL")) {
            if (strlen(value)>=sizeof(p->url)) malformed=1;
            else beam_copy(p->url,value,sizeof(p->url));
        }
    }
    Close(f);
    /* A plugin that needs a newer Beam68 is not loaded at all. */
    if (p->min_beam68[0] && beam_version_compare(p->min_beam68,BEAM68_VERSION)>0)
        return 0;
    /* Restrict the command to a quoted path: no shell interpolation allowed. */
    if (malformed || !strchr(p->command,':') ||
        strchr(p->command,'"') || strchr(p->command,'\n') || strchr(p->command,'\r') ||
        strchr(p->command,'*') ||
        (p->setting_count && !p->settings[0]) ||
        (p->settings[0] && (strchr(p->settings,'"') || strchr(p->settings,'*') ||
            strchr(p->settings,'\n') || strchr(p->settings,'\r') ||
            !strchr(p->settings,':')))) return 0;
    p->protocol=(unsigned char)version;
    return version==1 && *p->name && *p->command &&
           p->trigger>='!' && p->trigger<='~';
}

/* ENV: wins over ENVARC:, then the shipped sample. Keep input deterministic:
 * the caller starts with beam_config_defaults(). */
int beam_config_read_active(struct BeamConfig *c)
{
    static struct BeamConfig candidate;
    static const char *paths[]={
        BEAM_CONFIG_SESSION,BEAM_CONFIG_SESSION ".bak",
        BEAM_CONFIG_ENV,BEAM_CONFIG_ENVARC,
        BEAM_CONFIG_ENVARC ".bak",BEAM_CONFIG_ENV ".bak",
        BEAM_CONFIG_DEFAULT,NULL
    };
    int i;
    if (!c) return 0;
    for (i=0;paths[i];i++) {
        beam_config_defaults(&candidate);
        if (beam_config_read(&candidate,paths[i]) && beam_config_valid(&candidate)) {
            *c=candidate;
            return 1;
        }
    }
    return 0;
}

static int safe_setting(const char *s)
{
    size_t i;
    if (!s || !*s) return 0;
    for (i=0;s[i];i++) if (s[i]=='\r' || s[i]=='\n') return 0;
    return 1;
}
int beam_config_valid(const struct BeamConfig *c)
{
    int i;
    if (!safe_setting(c->hotkey) || !safe_setting(c->install_dir) ||
        !strchr(c->install_dir,':') || strchr(c->install_dir,'"') || strchr(c->install_dir,'*') || c->dir_count<0 || c->dir_count>BEAM_MAX_DIRS ||
        c->plugin_file_count<0 || c->plugin_file_count>BEAM_MAX_PLUGINS ||
        c->excluded_dir_count<0 || c->excluded_dir_count>BEAM_MAX_EXCLUDED_DIRS ||
        c->disabled_plugin_count<0 || c->disabled_plugin_count>BEAM_MAX_PLUGINS ||
        c->max_depth<0 || c->max_depth>16 || c->max_items<16 ||
        c->max_items>BEAM_MAX_ITEMS_LIMIT || c->visible<1 ||
        c->visible>BEAM_MAX_VISIBLE || (c->history!=0 && c->history!=1) ||
        (c->index_cache!=0 && c->index_cache!=1) ||
        (c->shell_commands!=0 && c->shell_commands!=1) ||
        (c->index_files!=0 && c->index_files!=1) ||
        (c->index_drawers!=0 && c->index_drawers!=1) ||
        (c->index_games!=0 && c->index_games!=1) ||
        !safe_setting(c->whdload_command) || strchr(c->whdload_command,'"') ||
        strchr(c->quick_launch,'\n') || strchr(c->quick_launch,'\r') ||
        !safe_setting(c->dopus_port) || (c->dopus_drawers!=0 && c->dopus_drawers!=1) ||
        (c->plugin_cache!=0 && c->plugin_cache!=1) ||
        c->results_max<8 || c->results_max>64 || c->browse_max<16 || c->browse_max>512 ||
        c->hidden_max<1 || c->hidden_max>256 || c->recent_docs_max<1 || c->recent_docs_max>64 ||
        strchr(c->catalogue_url,'\n') || !safe_setting(c->download_command) ||
        (c->index_rexx!=0 && c->index_rexx!=1) || (c->index_modules!=0 && c->index_modules!=1) ||
        strchr(c->bookmark_files,'\n') || !safe_setting(c->module_command) ||
        !safe_setting(c->url_command) ||
        strchr(c->module_port,'\n') || strchr(c->module_arexx,'\n') ||
        strchr(c->whdload_command,'*') ||
        c->theme<BEAM_THEME_WORKBENCH || c->theme>=BEAM_THEME_COUNT ||
        c->plugin_timeout<5 || c->plugin_timeout>120 ||
        (c->icon_preview!=0 && c->icon_preview!=1) ||
        c->window_width<230 || c->window_width>1200 ||
        (c->window_height!=0 && (c->window_height<90 || c->window_height>900)) ||
        (c->file_patterns[0] && !safe_setting(c->file_patterns)) ||
        !safe_setting(c->font_name) ||
        c->font_size<6 || c->font_size>32 ||
        c->row_spacing<2 || c->row_spacing>16 ||
        (c->high_contrast!=0 && c->high_contrast!=1) ||
        (c->large_gadgets!=0 && c->large_gadgets!=1) ||
        (c->reduced_colors!=0 && c->reduced_colors!=1) ||
        c->compact_mode<BEAM_COMPACT_AUTO || c->compact_mode>BEAM_COMPACT_NEVER) return 0;
    for (i=0;i<BEAM_THEME_COLOR_COUNT;i++)
        if (c->custom_colors[i]>0xffffffUL) return 0;
    for (i=0;i<c->dir_count;i++) if (!safe_setting(c->dirs[i])) return 0;
    for (i=0;i<c->excluded_dir_count;i++)
        if (!safe_setting(c->excluded_dirs[i]) ||
            !strchr(c->excluded_dirs[i],':') || strchr(c->excluded_dirs[i],'*')) return 0;
    for (i=0;i<c->plugin_file_count;i++) if (!safe_setting(c->plugin_files[i])) return 0;
    for (i=0;i<c->disabled_plugin_count;i++)
        if (!safe_setting(c->disabled_plugins[i])) return 0;
    return 1;
}

/* Keep third-party keys/comments and manually written documentation.
 * Replaces only the fields this editor owns, and appends canonical values.
 * Lines beyond 511 bytes are not copied: malformed keys cannot propagate.
 * Writes to .new, renames previous file to .bak, installs new file; on
 * error restores .bak. A failed save must never truncate the old file. */
static int owned_key(char *line)
{
    char *s=beam_trim(line), *eq=strchr(s,'=');
    if (!eq || *s=='#' || *s==';') return 0;
    *eq=0; s=beam_trim(s);
    return beam_equal(s,"INSTALL_DIR") || beam_equal(s,"PLUGIN_DISABLED") || beam_equal(s,"HOTKEY") || beam_equal(s,"HISTORY") || beam_equal(s,"INDEX_CACHE") || beam_equal(s,"SHELL_COMMANDS") || beam_equal(s,"THEME") || beam_equal(s,"CUSTOM_COLORS") || beam_equal(s,"ICON_PREVIEW") || beam_equal(s,"PLUGIN_TIMEOUT") || beam_equal(s,"DIR") ||
           beam_equal(s,"PLUGIN") || beam_equal(s,"SCAN_ICONS") ||
           beam_equal(s,"SCAN_EXECUTABLES") || beam_equal(s,"MAX_DEPTH") ||
           beam_equal(s,"MAX_ITEMS") || beam_equal(s,"VISIBLE_RESULTS") ||
           beam_equal(s,"WINDOW_WIDTH") || beam_equal(s,"WINDOW_HEIGHT") ||
           beam_equal(s,"EXCLUDE_DIR") || beam_equal(s,"FILE_PATTERNS") ||
           beam_equal(s,"INDEX_FILES") || beam_equal(s,"INDEX_DRAWERS") ||
           beam_equal(s,"FONT_NAME") || beam_equal(s,"FONT_SIZE") ||
           beam_equal(s,"ROW_SPACING") || beam_equal(s,"HIGH_CONTRAST") ||
           beam_equal(s,"LARGE_GADGETS") || beam_equal(s,"REDUCED_COLORS") ||
           beam_equal(s,"COMPACT_MODE") || beam_equal(s,"INDEX_GAMES") ||
           beam_equal(s,"WHDLOAD_COMMAND") || beam_equal(s,"QUICK_LAUNCH") ||
           beam_equal(s,"DOPUS_PORT") || beam_equal(s,"OPEN_DRAWERS_IN_DOPUS") ||
           beam_equal(s,"PLUGIN_CACHE") || beam_equal(s,"RESULTS_MAX") ||
           beam_equal(s,"BROWSE_MAX") || beam_equal(s,"HIDDEN_MAX") ||
           beam_equal(s,"RECENT_DOCS") || beam_equal(s,"CATALOGUE_URL") ||
           beam_equal(s,"INDEX_REXX") || beam_equal(s,"INDEX_MODULES") ||
           beam_equal(s,"BOOKMARK_FILES") || beam_equal(s,"MODULE_COMMAND") ||
           beam_equal(s,"URL_COMMAND") || beam_equal(s,"MODULE_PORT") ||
           beam_equal(s,"MODULE_AREXX") ||
           beam_equal(s,"DOWNLOAD_COMMAND");
}
static int generated_header(char *line)
{
    return beam_equal(beam_trim(line),
        "# Beam68 settings (editable text; created by Beam68Prefs)");
}
static int emit(BPTR f, const char *s)
{
    return FPuts(f,(STRPTR)s)>=0;
}
static int emit_entry(BPTR f,const char *key,const char *value)
{
    return emit(f,key) && emit(f,"=") && emit(f,value) && emit(f,"\n");
}
static int emit_number(BPTR f,const char *key,int value)
{
    char b[32];
    sprintf(b,"%ld",(long)value);
    return emit_entry(f,key,b);
}
static int config_path_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);
    return 1;
}
int beam_config_write(struct BeamConfig *c, const char *path)
{
    char tmp[BEAM_MAX_PATH+8],bak[BEAM_MAX_PATH+8],original[512],copy[512];
    char custom_colors[48];
    BPTR input=0,output=0,dir;
    int i,ok=1,had_original=0;
    if (!beam_config_valid(c) || strlen(path)>=BEAM_MAX_PATH) return 0;
    if (beam_equal(path,BEAM_CONFIG_SESSION)) {
        dir=CreateDir((STRPTR)"T:Beam68"); if (dir) UnLock(dir);
    } else if (beam_equal(path,BEAM_CONFIG_ENV)) {
        dir=CreateDir((STRPTR)"ENV:Beam68"); if (dir) UnLock(dir);
    } else if (beam_equal(path,BEAM_CONFIG_ENVARC)) {
        dir=CreateDir((STRPTR)"ENVARC:Beam68"); if (dir) UnLock(dir);
    }
    sprintf(tmp,"%s.new",path); sprintf(bak,"%s.bak",path);
    /* Preserve miscellaneous keys from current destination, else active
       configuration. No binary or sidecar settings file is necessary. */
    input=Open((STRPTR)path,MODE_OLDFILE);
    if (!input) input=Open((STRPTR)BEAM_CONFIG_ENV,MODE_OLDFILE);
    if (!input) input=Open((STRPTR)BEAM_CONFIG_ENVARC,MODE_OLDFILE);
    if (!input) input=Open((STRPTR)BEAM_CONFIG_DEFAULT,MODE_OLDFILE);
    output=Open((STRPTR)tmp,MODE_NEWFILE);
    if (!output) { if (input) Close(input); return 0; }
    if (input) {
        while (read_line(input,original,sizeof(original))) {
            beam_copy(copy,original,sizeof(copy));
            if (!owned_key(copy)) {
                beam_copy(copy,original,sizeof(copy));
                if (!generated_header(copy) && *original && !emit(output,original)) {
                    ok=0; break;
                }
            }
        }
        Close(input);
    }
    if (ok) ok=emit(output,"\n# Beam68 settings (editable text; created by Beam68Prefs)\n");
    if (ok) ok=emit_entry(output,"HOTKEY",c->hotkey);
    if (ok) ok=emit_entry(output,"INSTALL_DIR",c->install_dir);
    if (ok) ok=emit_entry(output,"HISTORY",c->history?"YES":"NO");
    if (ok) ok=emit_entry(output,"INDEX_CACHE",c->index_cache?"YES":"NO");
    if (ok) ok=emit_entry(output,"SHELL_COMMANDS",c->shell_commands?"YES":"NO");
    if (ok) ok=emit_entry(output,"THEME",config_theme_name(c->theme));
    beam_theme_format_colors(c->custom_colors,custom_colors,sizeof(custom_colors));
    if (ok) ok=emit_entry(output,"CUSTOM_COLORS",custom_colors);
    if (ok) ok=emit_entry(output,"ICON_PREVIEW",c->icon_preview?"YES":"NO");
    if (ok) ok=emit_number(output,"PLUGIN_TIMEOUT",c->plugin_timeout);
    if (ok) ok=emit_entry(output,"SCAN_ICONS",c->scan_icons?"YES":"NO");
    if (ok) ok=emit_entry(output,"SCAN_EXECUTABLES",c->scan_executables?"YES":"NO");
    if (ok) ok=emit_entry(output,"INDEX_FILES",c->index_files?"YES":"NO");
    if (ok) ok=emit_entry(output,"INDEX_DRAWERS",c->index_drawers?"YES":"NO");
    if (ok) ok=emit_entry(output,"INDEX_GAMES",c->index_games?"YES":"NO");
    if (ok) ok=emit_entry(output,"WHDLOAD_COMMAND",c->whdload_command);
    if (ok && c->quick_launch[0]) ok=emit_entry(output,"QUICK_LAUNCH",c->quick_launch);
    if (ok) ok=emit_entry(output,"DOPUS_PORT",c->dopus_port);
    if (ok) ok=emit_entry(output,"OPEN_DRAWERS_IN_DOPUS",c->dopus_drawers?"YES":"NO");
    if (ok) ok=emit_entry(output,"PLUGIN_CACHE",c->plugin_cache?"YES":"NO");
    if (ok) ok=emit_number(output,"RESULTS_MAX",c->results_max);
    if (ok) ok=emit_number(output,"BROWSE_MAX",c->browse_max);
    if (ok) ok=emit_number(output,"HIDDEN_MAX",c->hidden_max);
    if (ok) ok=emit_number(output,"RECENT_DOCS",c->recent_docs_max);
    if (ok && c->catalogue_url[0]) ok=emit_entry(output,"CATALOGUE_URL",c->catalogue_url);
    if (ok) ok=emit_entry(output,"DOWNLOAD_COMMAND",c->download_command);
    if (ok) ok=emit_entry(output,"INDEX_REXX",c->index_rexx?"YES":"NO");
    if (ok) ok=emit_entry(output,"INDEX_MODULES",c->index_modules?"YES":"NO");
    if (ok && c->bookmark_files[0]) ok=emit_entry(output,"BOOKMARK_FILES",c->bookmark_files);
    if (ok) ok=emit_entry(output,"MODULE_COMMAND",c->module_command);
    if (ok) ok=emit_entry(output,"URL_COMMAND",c->url_command);
    if (ok && c->module_port[0]) ok=emit_entry(output,"MODULE_PORT",c->module_port);
    if (ok && c->module_arexx[0]) ok=emit_entry(output,"MODULE_AREXX",c->module_arexx);
    if (ok) ok=emit_entry(output,"FILE_PATTERNS",c->file_patterns);
    if (ok) ok=emit_number(output,"MAX_DEPTH",c->max_depth);
    if (ok) ok=emit_number(output,"MAX_ITEMS",c->max_items);
    if (ok) ok=emit_number(output,"VISIBLE_RESULTS",c->visible);
    if (ok) ok=emit_number(output,"WINDOW_WIDTH",c->window_width);
    if (ok) ok=emit_number(output,"WINDOW_HEIGHT",c->window_height);
    if (ok) ok=emit_entry(output,"FONT_NAME",c->font_name);
    if (ok) ok=emit_number(output,"FONT_SIZE",c->font_size);
    if (ok) ok=emit_number(output,"ROW_SPACING",c->row_spacing);
    if (ok) ok=emit_entry(output,"HIGH_CONTRAST",c->high_contrast?"YES":"NO");
    if (ok) ok=emit_entry(output,"LARGE_GADGETS",c->large_gadgets?"YES":"NO");
    if (ok) ok=emit_entry(output,"REDUCED_COLORS",c->reduced_colors?"YES":"NO");
    if (ok) ok=emit_entry(output,"COMPACT_MODE",
        c->compact_mode==BEAM_COMPACT_ALWAYS?"YES":
        c->compact_mode==BEAM_COMPACT_NEVER?"NO":"AUTO");
    for (i=0;ok && i<c->dir_count;i++) ok=emit_entry(output,"DIR",c->dirs[i]);
    for (i=0;ok && i<c->excluded_dir_count;i++)
        ok=emit_entry(output,"EXCLUDE_DIR",c->excluded_dirs[i]);
    for (i=0;ok && i<c->plugin_file_count;i++) ok=emit_entry(output,"PLUGIN",c->plugin_files[i]);
    /* Never emit stale disable entries for manifests no longer installed. */
    for (i=0;ok && i<c->plugin_file_count;i++)
        if (!beam_plugin_enabled(c,c->plugin_files[i]))
            ok=emit_entry(output,"PLUGIN_DISABLED",c->plugin_files[i]);
    if (Close(output)==0) ok=0;
    if (!ok) { DeleteFile((STRPTR)tmp); return 0; }
    /* Rename an existing file first; never delete it to make room for the
       updated version. The .bak is useful for manual recovery as well. */
    had_original=config_path_exists(path);
    if (had_original) {
        int slot;
        DeleteFile((STRPTR)bak);
        /* Some host-backed Amiga filesystems can leave a metadata sidecar
         * which makes a stale .bak name appear occupied after deletion.
         * Keep the validated .new file and choose a free backup name rather
         * than abandoning an otherwise safe settings update. */
        if (config_path_exists(bak)) {
            for (slot=1;slot<=99;slot++) {
                sprintf(bak,"%s.b%02ld",path,(long)slot);
                if (!config_path_exists(bak)) break;
            }
            if (slot>99) { DeleteFile((STRPTR)tmp); return 0; }
        }
        if (!Rename((STRPTR)path,(STRPTR)bak)) {
            DeleteFile((STRPTR)tmp); return 0;
        }
    }
    if (!Rename((STRPTR)tmp,(STRPTR)path)) {
        if (had_original) Rename((STRPTR)bak,(STRPTR)path);
        DeleteFile((STRPTR)tmp); return 0;
    }
    return 1;
}
