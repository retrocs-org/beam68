#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "beam.h"
#include "appearance.h"
#include "theme_file.h"

static int safe_path(const char *path)
{
    return path && *path && strlen(path)<BEAM_MAX_PATH && strchr(path,':') &&
        !strpbrk(path,"\r\n\"*");
}

int beam_theme_file_load(const char *path,unsigned long *colors)
{
    BPTR f;char line[128];int version=0,got=0;
    unsigned long parsed[BEAM_THEME_COLOR_COUNT];
    if (!safe_path(path) || !colors) return 0;
    f=Open((STRPTR)path,MODE_OLDFILE);if (!f) return 0;
    while (FGets(f,line,sizeof(line))) {
        char *s=beam_trim(line),*eq;
        if (!*s || *s=='#' || *s==';') continue;
        eq=strchr(s,'=');if (!eq) continue;*eq++=0;
        s=beam_trim(s);eq=beam_trim(eq);
        if (beam_equal(s,"BEAM68THEME")) version=atoi(eq);
        else if (beam_equal(s,"COLORS")) got=beam_theme_parse_colors(eq,parsed);
    }
    Close(f);
    if (version!=1 || !got) return 0;
    memcpy(colors,parsed,sizeof(parsed));return 1;
}

int beam_theme_file_save(const char *path,const unsigned long *colors)
{
    BPTR f,lock;char temp[BEAM_MAX_PATH+8],bak[BEAM_MAX_PATH+8],text[48];
    unsigned long checked[BEAM_THEME_COLOR_COUNT];
    int old,ok=1;
    if (!safe_path(path) || !colors || strlen(path)+5>=sizeof(temp)) return 0;
    beam_theme_format_colors(colors,text,sizeof(text));
    if (!beam_theme_parse_colors(text,checked)) return 0;
    sprintf(temp,"%s.new",path);sprintf(bak,"%s.bak",path);
    f=Open((STRPTR)temp,MODE_NEWFILE);if (!f) return 0;
    if (FPuts(f,(STRPTR)"# Beam68 0.1 portable theme\nBEAM68THEME=1\nCOLORS=")<0 ||
        FPuts(f,(STRPTR)text)<0 || FPuts(f,(STRPTR)"\n")<0) ok=0;
    if (Close(f)==0) ok=0;
    if (!ok) {DeleteFile((STRPTR)temp);return 0;}
    lock=Lock((STRPTR)path,ACCESS_READ);old=lock!=0;if(lock)UnLock(lock);
    if (old) {DeleteFile((STRPTR)bak);if(!Rename((STRPTR)path,(STRPTR)bak)){DeleteFile((STRPTR)temp);return 0;}}
    if (!Rename((STRPTR)temp,(STRPTR)path)) {
        if(old)Rename((STRPTR)bak,(STRPTR)path);
        DeleteFile((STRPTR)temp);return 0;
    }
    return 1;
}
