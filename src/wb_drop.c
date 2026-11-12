#include <string.h>
#include "beam.h"
#include "wb_drop.h"

static unsigned char folded(unsigned char c)
{
    return (c>='A' && c<='Z')?(unsigned char)(c+('a'-'A')):c;
}
static int has_plugin_suffix(const char *path)
{
    static const char suffix[]=".plugin";
    unsigned int length=(unsigned int)strlen(path),i;
    if (length<=7) return 0;
    for (i=0;i<7;i++) if (folded((unsigned char)path[length-7+i])!=
                         (unsigned char)suffix[i]) return 0;
    return 1;
}
static int has_suffix_ci(const char *path,const char *suffix)
{
    unsigned int length=(unsigned int)strlen(path),n=(unsigned int)strlen(suffix),i;
    if (length<=n) return 0;
    for (i=0;i<n;i++) if (folded((unsigned char)path[length-n+i])!=
                         (unsigned char)suffix[i]) return 0;
    return 1;
}
static int has_theme_suffix(const char *path)
{
    static const char suffix[]=".theme";
    unsigned int length=(unsigned int)strlen(path),i;
    if (length<=6) return 0;
    for (i=0;i<6;i++) if (folded((unsigned char)path[length-6+i])!=
                         (unsigned char)suffix[i]) return 0;
    return 1;
}
int beam_drop_candidate(const char *path, int is_directory, int target,
                        char *output, unsigned int capacity)
{
    unsigned int length;
    if (!path || !output || capacity==0) return 0;
    output[0]=0;
    length=(unsigned int)strlen(path);
    if (!length || length>=BEAM_MAX_PATH || length>=capacity ||
        !strchr(path,':') || strchr(path,'"') || strchr(path,'*') ||
        strchr(path,'\n') || strchr(path,'\r')) return 0;
    if (target==BEAM_DROP_DIRECTORY) {
        if (!is_directory) return 0;
    } else if (target==BEAM_DROP_PLUGIN) {
        /* A .plugin manifest, a package drawer, or a package archive. */
        if (!is_directory && !has_plugin_suffix(path) &&
            !has_suffix_ci(path,".lha") && !has_suffix_ci(path,".lzh") &&
            !has_suffix_ci(path,".zip")) return 0;
    } else if (target==BEAM_DROP_FAVORITE) {
        if (is_directory) return 0;
    } else if (target==BEAM_DROP_THEME) {
        if (is_directory || !has_theme_suffix(path)) return 0;
    } else return 0;
    memcpy(output,path,length+1);
    return 1;
}
