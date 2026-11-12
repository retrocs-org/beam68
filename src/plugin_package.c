#include <string.h>
#include "plugin_package.h"

static int lower(int c) { return (c>='A' && c<='Z')?c+32:c; }

static int has_suffix(const char *s,const char *suffix)
{
    unsigned int n=(unsigned int)strlen(s),m=(unsigned int)strlen(suffix),i;
    if (n<=m) return 0;
    for (i=0;i<m;i++) if (lower((unsigned char)s[n-m+i])!=suffix[i]) return 0;
    return 1;
}

int beam_package_archive_kind(const char *path)
{
    if (!path) return BEAM_ARCHIVE_NONE;
    if (has_suffix(path,".lha") || has_suffix(path,".lzh")) return BEAM_ARCHIVE_LHA;
    if (has_suffix(path,".zip")) return BEAM_ARCHIVE_ZIP;
    return BEAM_ARCHIVE_NONE;
}

static int name_chars(const char *s,unsigned int max)
{
    unsigned int n=0;
    if (!s || !*s || *s=='.' || *s=='-') return 0;
    for (;*s;s++) {
        int c=(unsigned char)*s;
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
              c=='_'||c=='-'||c=='.')) return 0;
        if (++n>max) return 0;
    }
    return 1;
}

int beam_package_id_valid(const char *id) { return name_chars(id,63); }
int beam_package_drawer_name_valid(const char *name) { return name_chars(name,30); }

static int set_err(char *err,unsigned int cap,const char *text)
{
    if (err && cap) beam_copy(err,text,cap);
    return 0;
}

static int settings_ok(const char *s)
{
    static const char prefix[]="ENVARC:Beam68/Plugins/";
    unsigned int n=(unsigned int)strlen(prefix),i;
    if (!*s) return 1;
    if (strlen(s)<=n) return 0;
    for (i=0;i<n;i++) if (lower((unsigned char)s[i])!=lower((unsigned char)prefix[i])) return 0;
    return !strchr(s+n,'/') && !strchr(s+n,':') && s[n]!='.';
}

/* English texts for beam_package_manifest_problem() codes (tools, tests);
 * Beam68Prefs shows translated versions of the same messages. */
static const char *const problem_text[]={
    "",
    "No manifest",
    "Manifest needs a valid ID= (letters, digits, . _ -)",
    "Manifest needs PLUGIN_VERSION=",
    "Plugin needs a newer Beam68",
    "COMMAND must be PLUGINDIR:<file> inside the package",
    "ICON must be PLUGINDIR:<file> inside the package",
    "GUIDE must be PLUGINDIR:<file> inside the package",
    "SETTINGS must be ENVARC:Beam68/Plugins/<file>"
};

int beam_package_manifest_problem(const struct BeamPlugin *p)
{
    char probe[BEAM_MAX_PATH];
    if (!p) return BEAM_PKG_NO_MANIFEST;
    if (!beam_package_id_valid(p->id)) return BEAM_PKG_BAD_ID;
    if (!beam_version_valid(p->plugin_version)) return BEAM_PKG_NO_VERSION;
    if (p->min_beam68[0] && (!beam_version_valid(p->min_beam68) || beam_version_compare(p->min_beam68,BEAM68_VERSION)>0))
        return BEAM_PKG_TOO_NEW;
    if (!beam_is_plugindir_path(p->command) ||
        !beam_plugindir_resolve("X:P/x.plugin",p->command,probe,sizeof(probe)))
        return BEAM_PKG_BAD_COMMAND;
    /* PROGDIR:Beam68 (the launcher's own icon) is always present. */
    if (p->icon[0] && !beam_equal(p->icon,"PROGDIR:Beam68") &&
        (!beam_is_plugindir_path(p->icon) ||
         !beam_plugindir_resolve("X:P/x.plugin",p->icon,probe,sizeof(probe))))
        return BEAM_PKG_BAD_ICON;
    if (p->guide[0] && (!beam_is_plugindir_path(p->guide) ||
        !beam_plugindir_resolve("X:P/x.plugin",p->guide,probe,sizeof(probe))))
        return BEAM_PKG_BAD_GUIDE;
    if (!settings_ok(p->settings)) return BEAM_PKG_BAD_SETTINGS;
    return BEAM_PKG_OK;
}

int beam_package_manifest_check(const struct BeamPlugin *p,char *err,unsigned int cap)
{
    int problem=beam_package_manifest_problem(p);
    if (err && cap) err[0]=0;
    if (problem==BEAM_PKG_OK) return 1;
    return set_err(err,cap,problem_text[problem]);
}

int beam_package_zip_name_safe(const char *name)
{
    const char *p;
    if (!name || !*name || name[0]=='/' || name[0]=='\\') return 0;
    if (strlen(name)>=BEAM_MAX_PATH || strchr(name,':') || strchr(name,'\\') || strstr(name,"//")) return 0;
    for (p=name;*p;p++) if ((unsigned char)*p<32 || (unsigned char)*p==127 || strchr("\"*`",*p)) return 0;
    for (p=name;*p;) {
        const char *end=strchr(p,'/');
        unsigned int n=end?(unsigned int)(end-p):(unsigned int)strlen(p);
        if (n==2 && p[0]=='.' && p[1]=='.') return 0;
        if (n==1 && p[0]=='.') return 0;
        if (!end) break;
        p=end+1;
    }
    return 1;
}

int beam_package_lha_line_safe(const char *line)
{
    if (!line) return 0;
    return !strstr(line,"../") && !strstr(line,"..\\") && !strstr(line,"/..") &&
           !strstr(line," /") && !strstr(line,"\t/");
}

/* ------------------------------------------------------------ LHA headers */

static unsigned long le16(const unsigned char *p) { return (unsigned long)p[0]|((unsigned long)p[1]<<8); }
static unsigned long le32(const unsigned char *p)
{ return le16(p)|(le16(p+2)<<16); }

/* Append LHA path text: 0xFF and '\\' separate directories. */
static int add_name(char *out,unsigned int cap,unsigned int *n,
                    const unsigned char *s,unsigned long len,int as_dir)
{
    unsigned long i;
    for (i=0;i<len;i++) {
        int c=s[i];
        if (c==0xFF || c=='\\') c='/';
        if (!c || *n+2>=cap) return 0;
        out[(*n)++]=(char)c;
    }
    if (as_dir && *n && out[*n-1]!='/') out[(*n)++]='/';
    out[*n]=0;
    return 1;
}

/* Walk level 1/2 extended headers: type 1 = file name, 2 = directory. */
static int ext_headers(const unsigned char *buf,unsigned long avail,unsigned long pos,
                       unsigned long next,char *dir,char *file,unsigned int cap,
                       unsigned long *total_ext)
{
    unsigned int dn=0,fn=0;
    *total_ext=0;
    while (next) {
        if (next<3 || pos+next>avail) return pos+next>avail?-2:-1;
        if (buf[pos]==0x50) {
            unsigned long mode;
            if (next!=5) return -1;
            mode=le16(buf+pos+1)&0170000UL;
            if (mode && mode!=0100000UL && mode!=0040000UL) return -1;
        }
        if (buf[pos]==1) { fn=0; if (!add_name(file,cap,&fn,buf+pos+1,next-3,0)) return -1; }
        else if (buf[pos]==2) { dn=0; if (!add_name(dir,cap,&dn,buf+pos+1,next-3,1)) return -1; }
        *total_ext+=next;
        pos+=next;
        if (pos+2>avail) return -2;
        next=le16(buf+pos-2);
    }
    return 1;
}

long beam_lha_header(const unsigned char *buf,unsigned long avail,
                     char *name,unsigned int cap,unsigned long *skip)
{
    static char dir[BEAM_MAX_PATH],file[BEAM_MAX_PATH];
    unsigned long size,packed,total_ext=0;
    unsigned int n=0,level;
    long rc;
    if (!buf || !name || !cap || !skip) return -1;
    name[0]=0; dir[0]=0; file[0]=0; *skip=0;
    if (avail<1) return -2;
    if (buf[0]==0) return 0;
    if (avail<22) return -2;
    if (buf[2]!='-' || buf[6]!='-' || buf[3]!='l') return -1;
    packed=le32(buf+7);
    if (le32(buf+11)>16UL*1024UL*1024UL) return -1;
    level=buf[20];
    if (level==0 || level==1) {
        unsigned long namelen=buf[21];
        size=(unsigned long)buf[0]+2UL;
        if (size<(level==1?27UL:24UL)+namelen || size>BEAM_LHA_HEADER_MAX) return -1;
        if (size>avail) return -2;
        /* Level-0 OS-specific trailers are not interpreted here. Refuse
           them instead of allowing a hidden link type through to LhA. */
        if (level==0 && size!=24+namelen) return -1;
        if (!add_name(file,sizeof(file),&n,buf+22,namelen,0)) return -1;
        if (level==1) {
            /* base header ends with: CRC(2) OS(1) next-ext-size(2) */
            rc=ext_headers(buf,avail,size,le16(buf+size-2),dir,file,sizeof(file),&total_ext);
            if (rc<0) return rc;
            if (total_ext>packed) return -1;
            packed-=total_ext;  /* level 1 counts ext headers as data */
            size+=total_ext;
        }
    } else if (level==2) {
        size=le16(buf);
        if (size<26 || size>BEAM_LHA_HEADER_MAX) return -1;
        if (size>avail) return -2;
        rc=ext_headers(buf,avail,26,le16(buf+24),dir,file,sizeof(file),&total_ext);
        if (rc<0) return rc;
        if (26+total_ext!=size) return -1;
    } else return -1;
    n=0;
    if (!add_name(name,cap,&n,(const unsigned char *)dir,strlen(dir),0) ||
        !add_name(name,cap,&n,(const unsigned char *)file,strlen(file),0) || !n) return -1;
    *skip=packed;
    return (long)size;
}

long beam_zip_header(const unsigned char *buf,unsigned long avail,char *name,
                     unsigned int cap,struct BeamZipEntry *entry)
{
    unsigned long n,mode,attrs,total;
    if (!buf || !name || !entry || !cap) return -1;
    if (avail<46) return -2;
    if (memcmp(buf,"PK\001\002",4)) return -1;
    n=le16(buf+28); attrs=le32(buf+38); mode=(attrs>>16)&0170000UL;
    if (!n || n>=cap || n>=BEAM_MAX_PATH || le16(buf+34) ||
        (mode && mode!=0100000UL && mode!=0040000UL) || (attrs&0x400UL)) return -1;
    if (avail<46+n) return -2;
    memcpy(name,buf+46,(size_t)n); name[n]=0;
    if (strlen(name)!=n || !beam_package_zip_name_safe(name)) return -1;
    entry->flags=(unsigned int)le16(buf+8); entry->method=(unsigned int)le16(buf+10);
    entry->crc=le32(buf+16); entry->packed=le32(buf+20);
    entry->unpacked=le32(buf+24); entry->offset=le32(buf+42);
    if ((entry->flags&1U) || (entry->method!=0 && entry->method!=8) ||
        entry->unpacked>16UL*1024UL*1024UL || entry->packed>16UL*1024UL*1024UL ||
        entry->offset>0x7fffffffUL) return -1;
    total=46UL+n+le16(buf+30)+le16(buf+32);
    return (long)total;
}
