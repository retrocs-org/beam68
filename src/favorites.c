/* Editable pinned-application list. Deliberately compact for 68000 machines. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "favorites.h"

static int equal_path(const char *a,const char *b)
{
    int aa,bb;
    while (*a && *b) {
        aa=(unsigned char)*a++; bb=(unsigned char)*b++;
        if (aa>='A' && aa<='Z') aa+=32;
        if (bb>='A' && bb<='Z') bb+=32;
        if (aa!=bb) return 0;
    }
    return !*a && !*b;
}
static int safe_path(const char *s)
{
    size_t n;
    if (!s || !(n=strlen(s)) || n>=BEAM_MAX_PATH) return 0;
    if (s[0]=='#' || s[0]==';') return 0;
    return !strpbrk(s,"\r\n\t\"*|");
}
static int safe_group(const char *s)
{
    size_t n=0;
    if (!s || !*s) return 0;
    while (s[n]) {
        int c=(unsigned char)s[n++];
        if (n>=BEAM_FAVORITE_GROUP_LEN ||
            !((c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
              (c>='0'&&c<='9') || c==' ' || c=='-' || c=='_')) return 0;
    }
    return 1;
}
static int find_path(const struct BeamFavorites *f,const char *path)
{
    unsigned int i;
    for (i=0;i<f->count;i++) if (equal_path(f->paths[i],path)) return (int)i;
    return -1;
}
int beam_favorites_position(const struct BeamFavorites *f,const char *path)
{ return path?find_path(f,path):-1; }
void beam_favorites_clear(struct BeamFavorites *f)
{ memset(f,0,sizeof(*f)); }
int beam_favorites_contains(const struct BeamFavorites *f,const char *path)
{ return path && find_path(f,path)>=0; }
int beam_favorites_add(struct BeamFavorites *f,const char *path)
{ return beam_favorites_add_group(f,path,BEAM_FAVORITE_DEFAULT_GROUP); }
int beam_favorites_add_group(struct BeamFavorites *f,const char *path,
                             const char *group)
{
    int existing;
    if (!safe_path(path)) return 0;
    if (!group || !*group) group=BEAM_FAVORITE_DEFAULT_GROUP;
    if (!safe_group(group)) return 0;
    existing=find_path(f,path);
    if (existing>=0) {
        strcpy(f->groups[existing],group);
        return 1;
    }
    if (f->count>=BEAM_FAVORITES_CAP) return 0;
    strcpy(f->paths[f->count],path);
    strcpy(f->groups[f->count],group);
    ++f->count;
    return 1;
}
int beam_favorites_set_group(struct BeamFavorites *f,const char *path,
                             const char *group)
{
    int i=find_path(f,path);
    if (i<0 || !safe_group(group)) return 0;
    strcpy(f->groups[i],group);return 1;
}
const char *beam_favorites_group(const struct BeamFavorites *f,const char *path)
{
    int i=path?find_path(f,path):-1;
    return i<0?NULL:f->groups[i];
}
int beam_favorites_in_group(const struct BeamFavorites *f,const char *path,
                            const char *group)
{
    const char *found=beam_favorites_group(f,path);
    return found && group && equal_path(found,group);
}
int beam_favorites_remove(struct BeamFavorites *f,const char *path)
{
    int i=find_path(f,path);
    if (i<0) return 0;
    for (;(unsigned int)i+1<f->count;i++)
    {
        strcpy(f->paths[i],f->paths[i+1]);
        strcpy(f->groups[i],f->groups[i+1]);
    }
    --f->count;
    return 1;
}
int beam_favorites_toggle(struct BeamFavorites *f,const char *path)
{
    if (!safe_path(path)) return -1;
    if (beam_favorites_remove(f,path)) return 0;
    return beam_favorites_add(f,path)?1:-1;
}
int beam_favorites_load(struct BeamFavorites *f,const char *filename)
{
    BPTR in;
    char line[BEAM_MAX_PATH+4];
    beam_favorites_clear(f);
    in=Open((STRPTR)filename,MODE_OLDFILE);
    if (!in) return 0;
    while (FGets(in,line,sizeof(line))) {
        size_t n=strlen(line);
        if (!n) continue;
        if (line[n-1]!='\n' && n+1>=sizeof(line)) {
            char c;
            while (FRead(in,&c,1,1)==1 && c!='\n') { }
            continue;
        }
        while (n && (line[n-1]=='\n' || line[n-1]=='\r')) line[--n]=0;
        if (!n || line[0]=='#' || line[0]==';') continue;
        {
            char *separator=strchr(line,'|');
            if (separator) {
                *separator++=0;
                beam_favorites_add_group(f,separator,line);
            } else beam_favorites_add(f,line); /* legacy ungrouped line */
        }
    }
    Close(in);
    return 1;
}
int beam_favorites_load_active(struct BeamFavorites *f)
{
    if (beam_favorites_load(f,BEAM_FAVS_SESSION)) return 1;
    if (beam_favorites_load(f,BEAM_FAVS_ENV)) return 1;
    if (beam_favorites_load(f,BEAM_FAVS_ENVARC)) return 1;
    beam_favorites_clear(f);
    return 0;
}
int beam_favorites_save(const struct BeamFavorites *f,const char *filename)
{
    char temp[BEAM_MAX_PATH+8],bak[BEAM_MAX_PATH+8];
    BPTR out,lock,dir,in;
    unsigned int i;
    int ok=1,had_old;
    char line[512];
    if (!filename || strlen(filename)>=BEAM_MAX_PATH || f->count>BEAM_FAVORITES_CAP) return 0;
    for (i=0;i<f->count;i++)
        if (!safe_path(f->paths[i]) || !safe_group(f->groups[i])) return 0;
    if (!strcmp(filename,BEAM_FAVS_SESSION)) {
        dir=CreateDir((STRPTR)"T:Beam68"); if (dir) UnLock(dir);
    } else if (!strcmp(filename,BEAM_FAVS_ENV)) {
        dir=CreateDir((STRPTR)"ENV:Beam68"); if (dir) UnLock(dir);
    } else if (!strcmp(filename,BEAM_FAVS_ENVARC)) {
        dir=CreateDir((STRPTR)"ENVARC:Beam68"); if (dir) UnLock(dir);
    }
    sprintf(temp,"%s.new",filename);
    sprintf(bak,"%s.bak",filename);
    out=Open((STRPTR)temp,MODE_NEWFILE);
    if (!out) return 0;
    if (FPuts(out,(STRPTR)"# Beam68 0.1 favorites; Group|full application path\n")<0) ok=0;
    /* Keep manually written comment and blank lines from this destination.
     * All app paths are regenerated below, so unpinned entries disappear. */
    in=Open((STRPTR)filename,MODE_OLDFILE);
    if (in) {
        while (ok && FGets(in,line,sizeof(line))) {
            size_t n=strlen(line);
            if (n && line[n-1]!='\n' && n+1>=sizeof(line)) {
                char c;
                while (FRead(in,&c,1,1)==1 && c!='\n') { }
                continue;
            }
            if (!strcmp(line,"# Beam68 0.1 favorites; one full application path per line\n") ||
                !strcmp(line,"# Beam68 0.1 favorites; Group|full application path\n")) continue;
            if (line[0]=='#' || line[0]==';' || line[0]=='\n') {
                if (FPuts(out,(STRPTR)line)<0) ok=0;
                if (ok && n && line[n-1]!='\n' && FPuts(out,(STRPTR)"\n")<0) ok=0;
            }
        }
        Close(in);
    }
    for (i=0;ok && i<f->count;i++) {
        if (!safe_group(f->groups[i]) || FPuts(out,(STRPTR)f->groups[i])<0 ||
            FPuts(out,(STRPTR)"|")<0 || FPuts(out,(STRPTR)f->paths[i])<0 ||
            FPuts(out,(STRPTR)"\n")<0) ok=0;
    }
    if (Close(out)==0) ok=0;
    if (!ok) { DeleteFile((STRPTR)temp); return 0; }
    lock=Lock((STRPTR)filename,ACCESS_READ);
    had_old=lock!=0;
    if (lock) UnLock(lock);
    if (had_old) {
        DeleteFile((STRPTR)bak);
        if (!Rename((STRPTR)filename,(STRPTR)bak)) {
            DeleteFile((STRPTR)temp); return 0;
        }
    }
    if (!Rename((STRPTR)temp,(STRPTR)filename)) {
        if (had_old) Rename((STRPTR)bak,(STRPTR)filename);
        DeleteFile((STRPTR)temp); return 0;
    }
    return 1;
}
