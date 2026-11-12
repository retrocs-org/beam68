#include <string.h>
#include "beam.h"
#include "catalogue.h"
#include "plugin_package.h"

static int field(const char **p,char *out,unsigned int cap)
{
    unsigned int n=0;
    const char *s=*p;
    while (*s && *s!='|' && *s!='\n' && *s!='\r') {
        if ((unsigned char)*s<32) return 0;
        if (n+1>=cap) return 0;
        out[n++]=*s++;
    }
    out[n]=0;
    *p=(*s=='|')?s+1:s;
    return 1;
}

int beam_catalogue_parse_line(const char *line,struct BeamCatalogueEntry *e)
{
    char kind[8];
    const char *p=line;
    memset(e,0,sizeof(*e));
    if (!line || !*line || *line=='#' || *line==';') return 0;
    if (!field(&p,kind,sizeof(kind))) return 0;
    if (beam_equal(kind,"PLUGIN")) e->kind=BEAM_CAT_PLUGIN;
    else if (beam_equal(kind,"THEME")) e->kind=BEAM_CAT_THEME;
    else return 0;
    if (!field(&p,e->id,sizeof(e->id)) || !field(&p,e->name,sizeof(e->name)) ||
        !field(&p,e->version,sizeof(e->version)) || !field(&p,e->author,sizeof(e->author)) ||
        !field(&p,e->description,sizeof(e->description)) || !field(&p,e->url,sizeof(e->url)))
        return 0;
    if (!beam_package_id_valid(e->id) || !*e->name || !beam_version_valid(e->version) ||
        !*e->url || !beam_catalogue_extension(e->url)) return 0;
    if ((e->kind==BEAM_CAT_THEME)!=!strcmp(beam_catalogue_extension(e->url),".theme")) return 0;
    return 1;
}

const char *beam_catalogue_extension(const char *url)
{
    static const char *const ext[]={".lha",".lzh",".zip",".theme",NULL};
    unsigned int n=(unsigned int)strlen(url),i;
    for (i=0;ext[i];i++) {
        unsigned int m=(unsigned int)strlen(ext[i]);
        if (n>m && beam_equal(url+n-m,ext[i])) return ext[i];
    }
    return NULL;
}

static int clean(const char *s)
{
    for (;*s;s++) if ((unsigned char)*s<=32 || *s=='"' || *s=='*' || *s=='`') return 0;
    return 1;
}

int beam_catalogue_command(const char *t,const char *url,const char *file,
                           char *out,unsigned int cap)
{
    unsigned int n=0;
    if (!t || !url || !file || !*url || !clean(url) || !clean(file) || !cap) return 0;
    while (*t) {
        const char *insert=NULL;
        if (!strncmp(t,"{url}",5)) { insert=url; t+=5; }
        else if (!strncmp(t,"{file}",6)) { insert=file; t+=6; }
        if (insert) {
            unsigned int m=(unsigned int)strlen(insert);
            if (n+m+1>=cap) return 0;
            memcpy(out+n,insert,m); n+=m;
        } else {
            if (n+2>=cap) return 0;
            out[n++]=*t++;
        }
    }
    out[n]=0;
    return 1;
}
