/* Native AmigaOS plugin inspector used only by the separate Preferences app. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "beam.h"
#include "plugin_catalog.h"

static int is_progdir_path(const char *path)
{
    return path && strlen(path)>=8 &&
        (path[0]=='P' || path[0]=='p') &&
        (path[1]=='R' || path[1]=='r') &&
        (path[2]=='O' || path[2]=='o') &&
        (path[3]=='G' || path[3]=='g') &&
        (path[4]=='D' || path[4]=='d') &&
        (path[5]=='I' || path[5]=='i') &&
        (path[6]=='R' || path[6]=='r') && path[7]==':';
}

int beam_catalog_resolve(const struct BeamConfig *c, const char *path,
                         char *out, unsigned int cap)
{
    unsigned int base, tail, sep=0;
    if (!c || !path || !out || cap==0) return 0;
    out[0]=0;
    if (is_progdir_path(path)) {
        const char *suffix=path+8;
        base=(unsigned int)strlen(c->install_dir);
        tail=(unsigned int)strlen(suffix);
        if (!base || !strchr(c->install_dir,':')) return 0;
        if (tail && c->install_dir[base-1]!=':' && c->install_dir[base-1]!='/') sep=1;
        if (base+tail+sep+1>cap) return 0;
        memcpy(out,c->install_dir,base);
        if (sep) out[base++]='/';
        memcpy(out+base,suffix,tail+1);
    } else {
        tail=(unsigned int)strlen(path);
        if (tail+1>cap) return 0;
        memcpy(out,path,tail+1);
    }
    return out[0] && !strchr(out,'"') && !strchr(out,'*') &&
           !strchr(out,'\r') && !strchr(out,'\n');
}

/* A development copy keeps Beam68Prefs beside Beam68 and Plugins/, while an
 * installed copy puts Prefs in SYS:Prefs and points INSTALL_DIR at the
 * launcher drawer. Prefer INSTALL_DIR, but if that target does not exist,
 * allow the current process' PROGDIR: as a deliberate development fallback. */
static int existing_manifest_path(const struct BeamConfig *c,const char *spec,
                                  char *out,unsigned int cap,int *used_local)
{
    BPTR lock;
    *used_local=0;
    if (!beam_catalog_resolve(c,spec,out,cap)) return -1;
    lock=Lock((STRPTR)out,ACCESS_READ);
    if (lock) { UnLock(lock); return 1; }
    if (!is_progdir_path(spec) || beam_equal(out,spec) || strlen(spec)+1>cap)
        return 0;
    lock=Lock((STRPTR)spec,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);
    beam_copy(out,spec,cap);
    *used_local=1;
    return 1;
}

int beam_catalog_inspect(const struct BeamConfig *c, int index,
                         unsigned char claimed[128], struct BeamPlugin *plugin)
{
    char manifest[BEAM_MAX_PATH],executable[BEAM_MAX_PATH];
    BPTR lock;
    int disabled,local_manifest=0,manifest_found;
    unsigned char trigger;
    if (!c || !claimed || !plugin || index<0 || index>=c->plugin_file_count) return BEAM_PLUGIN_MANIFEST_ERROR;
    memset(plugin,0,sizeof(*plugin));
    disabled=!beam_plugin_enabled(c,c->plugin_files[index]);
    manifest_found=existing_manifest_path(c,c->plugin_files[index],manifest,
                                          sizeof(manifest),&local_manifest);
    if (manifest_found<0)
        return disabled?BEAM_PLUGIN_DISABLED:BEAM_PLUGIN_PATH_ERROR;
    if (!manifest_found)
        return disabled?BEAM_PLUGIN_DISABLED:BEAM_PLUGIN_MANIFEST_ERROR;
    if (!beam_plugin_read(plugin,manifest))
        return disabled?BEAM_PLUGIN_DISABLED:BEAM_PLUGIN_MANIFEST_ERROR;
    if (disabled) return BEAM_PLUGIN_DISABLED;
    if (beam_is_plugindir_path(plugin->command)) {
        /* Package form: relative to the manifest's own drawer. */
        if (!beam_plugindir_resolve(manifest,plugin->command,executable,sizeof(executable)))
            return BEAM_PLUGIN_PATH_ERROR;
    } else if (local_manifest && is_progdir_path(plugin->command))
        beam_copy(executable,plugin->command,sizeof(executable));
    else if (!beam_catalog_resolve(c,plugin->command,executable,sizeof(executable)))
            return BEAM_PLUGIN_PATH_ERROR;
    lock=Lock((STRPTR)executable,ACCESS_READ);
    if (!lock) return BEAM_PLUGIN_EXECUTABLE_MISSING;
    UnLock(lock);
    trigger=(unsigned char)plugin->trigger;
    if (trigger>=128 || claimed[trigger]) return BEAM_PLUGIN_CONFLICT;
    claimed[trigger]=1;
    return BEAM_PLUGIN_OK;
}

int beam_catalog_discover_bundled(struct BeamConfig *c)
{
    /* ~17 KB together: static, never on a (possibly 4 KB) Shell stack. */
    static struct BeamConfig bundled;
    static struct BeamPlugin plugin;
    char manifest[BEAM_MAX_PATH];
    int i,j,found,local_manifest,added=0;
    if (!c) return 0;
    beam_config_defaults(&bundled);
    for (i=0;i<bundled.plugin_file_count &&
         c->plugin_file_count<BEAM_MAX_PLUGINS;i++) {
        found=0;
        for (j=0;j<c->plugin_file_count;j++)
            if (beam_equal(c->plugin_files[j],bundled.plugin_files[i])) {
                found=1;break;
            }
        if (found) continue;
        if (existing_manifest_path(c,bundled.plugin_files[i],manifest,
                                   sizeof(manifest),&local_manifest)<=0 ||
            !beam_plugin_read(&plugin,manifest)) continue;
        beam_copy(c->plugin_files[c->plugin_file_count],bundled.plugin_files[i],
                  BEAM_MAX_PATH);
        c->plugin_file_count++;
        /* Discovery is intentionally visible but opt-in. */
        if (!beam_plugin_set_enabled(c,bundled.plugin_files[i],0)) {
            c->plugin_file_count--;
            continue;
        }
        added++;
    }
    return added;
}

int beam_catalog_merge_discovered(struct BeamConfig *c,
                                  const char (*specs)[BEAM_MAX_PATH],int count)
{
    int i,j,added=0;
    if (!c || !specs) return 0;
    for (i=0;i<count && c->plugin_file_count<BEAM_MAX_PLUGINS;i++) {
        int found=0;
        if (!specs[i][0]) continue;
        for (j=0;j<c->plugin_file_count;j++)
            if (beam_equal(c->plugin_files[j],specs[i])) { found=1; break; }
        if (found) continue;
        beam_copy(c->plugin_files[c->plugin_file_count],specs[i],BEAM_MAX_PATH);
        c->plugin_file_count++;
        /* Discovery is visible but opt-in: never silently grant access. */
        if (!beam_plugin_set_enabled(c,specs[i],0)) {
            c->plugin_file_count--;
            continue;
        }
        added++;
    }
    return added;
}

/* Topaz is fixed-width: pad and bound every field so columns never drift. */
void beam_catalog_format_row(char *out,unsigned int cap,unsigned int columns,
                            const char *priority,const char *name,const char *version,
                            const char *state,const char *key)
{
    char row[96];int width=(int)columns-22;
    if(width<8)width=8;
    if(width>40)width=40;
    sprintf(row,"%-3.3s %-*.*s %-7.7s %-5.5s %-3.3s",priority?priority:"-",width,width,name?name:"-",
            version && *version?version:"-",state?state:"-",key?key:"-");
    beam_copy(out,row,cap);
}

/* Stable identities plus canonical paths protect even broken/legacy bundled
 * manifests. Names or capabilities alone must not decide removability. */
int beam_catalog_is_bundled(const struct BeamConfig *c,int index,const struct BeamPlugin *p)
{
    static const char *ids[]={"org.beam68.calc","org.beam68.clock","org.beam68.memory",
        "org.beam68.filefinder","org.beam68.systemcommands","org.beam68.dostools",
        "org.beam68.taskmanager","org.beam68.clipboardtools","org.beam68.dateutilities",
        "org.beam68.windowswitcher","org.beam68.developertools","org.beam68.arexxrunner",
        "org.beam68.musiccontrols","org.beam68.aminetbrowser","org.beam68.musiclibrary","org.beam68.whdload","org.beam68.dopus","org.beam68.wikipedia"};
    static const char *drawers[]={"Calc","Clock","Memory","FileFinder","SystemCommands",
        "DosTools","TaskManager","ClipboardTools","DateUtilities","WindowSwitcher",
        "DeveloperTools","ARexxRunner","MusicControls","AminetBrowser","MusicLibrary","WHDLoad","DOpus","Wikipedia"};
    char actual[BEAM_MAX_PATH],expected[BEAM_MAX_PATH],spec[BEAM_MAX_PATH];
    unsigned int i;
    if(!c || index<0 || index>=c->plugin_file_count)return 0;
    for(i=0;i<sizeof(ids)/sizeof(ids[0]);i++)if(p && beam_equal(p->id,ids[i]))return 1;
    if(!beam_catalog_resolve(c,c->plugin_files[index],actual,sizeof(actual)))return 0;
    for(i=0;i<sizeof(drawers)/sizeof(drawers[0]);i++) {
        sprintf(spec,"PROGDIR:Plugins/%s/%s.plugin",drawers[i],drawers[i]);
        if(beam_catalog_resolve(c,spec,expected,sizeof(expected)) && beam_equal(actual,expected))return 1;
        sprintf(spec,"PROGDIR:Plugins/%s.plugin",drawers[i]);
        if(beam_catalog_resolve(c,spec,expected,sizeof(expected)) && beam_equal(actual,expected))return 1;
    }
    return 0;
}

int beam_catalog_can_preview(const struct BeamPlugin *p)
{ return p && p->preview && !beam_plugin_has_capability(p,"SYSTEM"); }
int beam_catalog_has_settings(const struct BeamPlugin *p)
{ return p && p->settings[0]; }

/* Validate a complete dialog before writing any of its plugin-owned settings. */
int beam_catalog_setting_value(const struct BeamPluginSetting *d,const char *value,char *out,unsigned int cap)
{
    char copy[128],*p,*end;const char *s;
    if(!d || !value || !out || !cap || strlen(value)>=64)return 0;
    beam_copy(copy,value,sizeof(copy));p=beam_trim(copy);s=p;
    if(strchr(p,'\n') || strchr(p,'\r'))return 0;
    if(d->type==BEAM_PLUGIN_SETTING_BOOL) {
        if(beam_equal(p,"YES") || beam_equal(p,"TRUE") || !strcmp(p,"1"))s="YES";
        else if(beam_equal(p,"NO") || beam_equal(p,"FALSE") || !strcmp(p,"0"))s="NO";
        else return 0;
    } else if(d->type==BEAM_PLUGIN_SETTING_NUMBER) {
        if(*p=='-' || *p=='+')p++;
        if(!*p)return 0;
        while(*p)if(*p<'0' || *p>'9')return 0;else p++;
    } else if(d->type==BEAM_PLUGIN_SETTING_CHOICE) {
        beam_copy(copy,d->choices,sizeof(copy));p=copy;s=NULL;
        while(*p) {
            end=strchr(p,',');if(end)*end=0;p=beam_trim(p);
            if(beam_equal(p,value)){s=p;break;}
            if(!end)break;
            p=end+1;
        }
        if(!s)return 0;
    } else if(d->type!=BEAM_PLUGIN_SETTING_TEXT)return 0;
    if(strlen(s)>=cap)return 0;
    beam_copy(out,s,cap);return 1;
}
