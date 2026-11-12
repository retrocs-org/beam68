/* Host-only AmigaDOS file-I/O shim for actual config.c integration tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fake_ndk/proto/dos.h"

static int fail_writes=-1,fail_rename=-1;
void beam_test_fail_writes(int after){fail_writes=after;}
void beam_test_fail_rename(int after){fail_rename=after;}

static const char *pathmap(const char *name)
{
    static char result[4][512];
    static int which;
    const char *root="build/testsettings/";
    char *out=result[(which++)&3];
    const char *colon=strchr(name,':');
    if (colon) {
        size_t n=(size_t)(colon-name);
        snprintf(out,512,"%s%.*s/%s",root,(int)n,name,colon+1);
    } else snprintf(out,512,"%s%s",root,name);
    return out;
}
BPTR Open(STRPTR name,LONG mode)
{ return fopen(pathmap(name),mode==MODE_NEWFILE?"wb":"rb"); }
char *FGets(BPTR f,char *line,LONG n) { return fgets(line,(int)n,f); }
LONG FRead(BPTR f,void *buf,LONG size,LONG count)
{ return (LONG)fread(buf,(size_t)size,(size_t)count,f); }
LONG FPuts(BPTR f,STRPTR s) { if (fail_writes==0) {fail_writes=-1;return -1;} if (fail_writes>0)--fail_writes;return fputs(s,f)==EOF?-1:0; }
LONG Close(BPTR f) { return fclose(f)==0?-1:0; }
BPTR CreateDir(STRPTR name)
{ return mkdir(pathmap(name),0777)==0?(BPTR)1:NULL; }
BPTR Lock(STRPTR name,LONG mode)
{
    FILE *f;
    (void)mode;
    f=fopen(pathmap(name),"rb");
    return f;
}
void UnLock(BPTR f) { if (f!=(BPTR)1 && f) fclose(f); }
LONG Rename(STRPTR a,STRPTR b)
{
    char a2[512],b2[512];
    if (fail_rename==0) {fail_rename=-1;return 0;}
    if (fail_rename>0) --fail_rename;
    strcpy(a2,pathmap(a)); strcpy(b2,pathmap(b));
    return rename(a2,b2)==0?-1:0;
}
LONG DeleteFile(STRPTR name) { return remove(pathmap(name))==0?-1:0; }
