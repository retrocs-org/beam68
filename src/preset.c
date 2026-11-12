#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "preset.h"

#define PRESET_LINE 600
static char preset_names[BEAM_PRESET_MAX_FILES][64];

int beam_preset_name_ok(const char *name)
{
    const char *p;
    int slashes=0;
    if (!name || !*name || strlen(name)>=64 || *name=='.') return 0;
    if (strchr(name,'/')) {
        if (strncmp(name,"Plugins/",8) || !name[8]) return 0;
    }
    for (p=name;*p;p++) {
        int c=(unsigned char)*p;
        if (c=='/') { if (++slashes>1) return 0; continue; }
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))
            return 0;
    }
    return !strstr(name,"..") && !beam_equal(name,"Plugins");
}

static int join(char *out,unsigned int cap,const char *dir,const char *name)
{
    if (strlen(dir)+strlen(name)+2>cap) return 0;
    sprintf(out,"%s/%s",dir,name);
    return 1;
}

int beam_preset_export(const char *out_path,const char *dir,
                       const char *const *names,int count)
{
    static char line[PRESET_LINE],path[BEAM_MAX_PATH];
    BPTR out,in;
    int i,written=0,ok=1;
    out=Open((STRPTR)out_path,MODE_NEWFILE);
    if (!out) return -1;
    ok=FPuts(out,(STRPTR)"BEAM68PRESET 1\n")>=0;
    for (i=0;ok && i<count;i++) {
        if (!beam_preset_name_ok(names[i]) || !join(path,sizeof(path),dir,names[i])) continue;
        in=Open((STRPTR)path,MODE_OLDFILE);
        if (!in) continue;
        ok=FPuts(out,(STRPTR)"=FILE ")>=0 && FPuts(out,(STRPTR)names[i])>=0 &&
           FPuts(out,(STRPTR)"\n")>=0;
        while (ok && FGets(in,line,sizeof(line)-2)) {
            unsigned int n=(unsigned int)strlen(line);
            if (n>=sizeof(line)-3) { ok=0; break; }
            if (n && line[n-1]!='\n') { line[n++]='\n'; line[n]=0; }
            ok=FPuts(out,(STRPTR)"|")>=0 && FPuts(out,(STRPTR)line)>=0;
        }
        Close(in);
        if (ok) ok=FPuts(out,(STRPTR)"=END\n")>=0;
        if (ok) ++written;
    }
    if (!Close(out)) ok=0;
    if (!ok) { DeleteFile((STRPTR)out_path); return -1; }
    return written;
}

/* Two passes: validate everything, then write. */
static int import_pass(const char *in_path,const char *const *dirs,int dir_count,int write)
{
    static char line[PRESET_LINE],name[64],path[BEAM_MAX_PATH];
    BPTR in,out[4];
    int files=0,inside=0,d,ok=1;
    if (!in_path || !dirs || dir_count<1 || dir_count>4) return -1;
    in=Open((STRPTR)in_path,MODE_OLDFILE);
    if (!in) return -1;
    if (!FGets(in,line,sizeof(line)) || strcmp(beam_trim(line),"BEAM68PRESET 1")) { Close(in); return -1; }
    for (d=0;d<4;d++) out[d]=0;
    while (ok && FGets(in,line,sizeof(line))) {
        unsigned int length=(unsigned int)strlen(line);
        if (!length || line[length-1]!='\n') { ok=0; break; }
        if (!strncmp(line,"=FILE ",6)) {
            char *t=beam_trim(line+6);
            int previous;
            if (inside || !beam_preset_name_ok(t) || files>=BEAM_PRESET_MAX_FILES) { ok=0; break; }
            for (previous=0;previous<files;previous++) if (beam_equal(preset_names[previous],t)) { ok=0; break; }
            if (!ok) break;
            beam_copy(preset_names[files],t,sizeof(preset_names[0]));
            beam_copy(name,t,sizeof(name));
            inside=1;
            if (write) for (d=0;d<dir_count;d++) {
                if (strchr(name,'/')) {   /* make the Plugins drawer */
                    BPTR l;
                    if (join(path,sizeof(path),dirs[d],"Plugins")) {
                        l=CreateDir((STRPTR)path); if (l) UnLock(l);
                    }
                }
                if (!join(path,sizeof(path),dirs[d],name) ||
                    !(out[d]=Open((STRPTR)path,MODE_NEWFILE))) ok=0;
            }
        } else if ((!strcmp(line,"=END\n") || !strcmp(line,"=END\r\n"))) {
            if (!inside) { ok=0; break; }
            inside=0; ++files;
            for (d=0;d<dir_count;d++) if (out[d]) { if (!Close(out[d])) ok=0; out[d]=0; }
        } else if (line[0]=='|') {
            if (!inside) { ok=0; break; }
            if (write) for (d=0;d<dir_count;d++) if (out[d] && FPuts(out[d],(STRPTR)line+1)<0) ok=0;
        } else if (line[0]!='\n' && line[0]!='#') { ok=0; break; }
    }
    for (d=0;d<dir_count;d++) if (out[d]) Close(out[d]);
    Close(in);
    if (inside) ok=0;
    return ok?files:-1;
}

/* All files are written into newly reserved directories before changing any
 * active setting. Originals remain in previous/ until the whole commit works. */
static int path_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock); return 1;
}
static int make_dir(const char *path)
{
    BPTR lock=CreateDir((STRPTR)path);
    if (lock) { UnLock(lock); return 1; }
    return path_exists(path);
}
int beam_preset_import(const char *in_path,const char *const *dirs,int dir_count)
{
    static char stage[4][BEAM_MAX_PATH],incoming[4][BEAM_MAX_PATH],previous[4][BEAM_MAX_PATH];
    static unsigned char moved[BEAM_PRESET_MAX_FILES][4];
    const char *write_dirs[4];
    char target[BEAM_MAX_PATH],from[BEAM_MAX_PATH],backup[BEAM_MAX_PATH],leaf[32];
    int files,d,i,slot,reserved=0,ok=0,rollback_failed=0;
    if (!in_path || !dirs || dir_count<1 || dir_count>4) return -1;
    for (d=0;d<dir_count;d++) if (!dirs[d] || !*dirs[d]) return -1;
    files=import_pass(in_path,dirs,dir_count,0);
    if (files<=0) return files<0?-1:0;
    memset(stage,0,sizeof(stage)); memset(incoming,0,sizeof(incoming)); memset(previous,0,sizeof(previous)); memset(moved,0,sizeof(moved));
    for (d=0;d<dir_count;d++) {
        BPTR lock=0;
        for (slot=0;slot<100;slot++) {
            sprintf(leaf,"B68Preset%02d",slot);
            if (!join(target,sizeof(target),dirs[d],leaf)) goto cleanup;
            lock=CreateDir((STRPTR)target);
            if (lock) break;
        }
        if (!lock) goto cleanup;
        UnLock(lock); beam_copy(stage[d],target,sizeof(stage[d])); reserved++;
        if (!join(incoming[d],sizeof(incoming[d]),stage[d],"incoming") ||
            !join(previous[d],sizeof(previous[d]),stage[d],"previous") ||
            !make_dir(incoming[d]) || !make_dir(previous[d])) goto cleanup;
        if (!join(target,sizeof(target),previous[d],"Plugins") || !make_dir(target)) goto cleanup;
        write_dirs[d]=incoming[d];
    }
    if (import_pass(in_path,write_dirs,dir_count,1)!=files) goto cleanup;
    /* Validate every destination and backup path before moving originals. */
    for (i=0;i<files;i++) for (d=0;d<dir_count;d++) {
        if (!join(target,sizeof(target),dirs[d],preset_names[i]) ||
            !join(from,sizeof(from),incoming[d],preset_names[i]) ||
            !join(backup,sizeof(backup),previous[d],preset_names[i])) goto cleanup;
        if (strchr(preset_names[i],'/')) {
            if (!join(target,sizeof(target),dirs[d],"Plugins") || !make_dir(target)) goto cleanup;
        }
    }
    for (i=0;i<files;i++) for (d=0;d<dir_count;d++) {
        join(target,sizeof(target),dirs[d],preset_names[i]);
        join(from,sizeof(from),incoming[d],preset_names[i]);
        join(backup,sizeof(backup),previous[d],preset_names[i]);
        if (path_exists(target)) {
            if (!Rename((STRPTR)target,(STRPTR)backup)) goto rollback;
            moved[i][d]=1;
        }
        if (!Rename((STRPTR)from,(STRPTR)target)) goto rollback;
        moved[i][d]|=2;
    }
    ok=1; goto cleanup;
rollback:
    for (i=files-1;i>=0;i--) for (d=dir_count-1;d>=0;d--) {
        join(target,sizeof(target),dirs[d],preset_names[i]);
        join(backup,sizeof(backup),previous[d],preset_names[i]);
        if ((moved[i][d]&2) && !DeleteFile((STRPTR)target)) rollback_failed=1;
        if ((moved[i][d]&1) && !Rename((STRPTR)backup,(STRPTR)target)) rollback_failed=1;
    }
cleanup:
    /* A failed rollback keeps all recovery data in B68PresetNN/previous/. */
    if (!rollback_failed) for (d=0;d<reserved;d++) {
        for (i=0;i<files;i++) {
            if (incoming[d][0] && join(from,sizeof(from),incoming[d],preset_names[i])) DeleteFile((STRPTR)from);
            if (previous[d][0] && join(backup,sizeof(backup),previous[d],preset_names[i])) DeleteFile((STRPTR)backup);
        }
        if (incoming[d][0] && join(target,sizeof(target),incoming[d],"Plugins")) DeleteFile((STRPTR)target);
        if (previous[d][0] && join(target,sizeof(target),previous[d],"Plugins")) DeleteFile((STRPTR)target);
        if (incoming[d][0]) DeleteFile((STRPTR)incoming[d]);
        if (previous[d][0]) DeleteFile((STRPTR)previous[d]);
        DeleteFile((STRPTR)stage[d]);
    }
    return ok?files:-1;
}
