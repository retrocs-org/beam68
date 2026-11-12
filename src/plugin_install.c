/* Plugin package installer for Beam68Prefs.  AmigaOS 3.x, 68000-safe.
 * Large buffers are static: Preferences may run on a small stack. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "plugin_catalog.h"
#include "plugin_package.h"
#include "plugin_install.h"
#include "dos_compat.h"
#include "prefs_locale.h"

static char install_tmp[64];
#define INSTALL_TMP install_tmp
#define CMD_LEN (BEAM_MAX_PATH*2+64)

static struct BeamPlugin manifest_buf;   /* the package being installed */
static struct BeamPlugin other_buf;      /* installed plugins, compared */
static char cmd[CMD_LEN];
static char body[1024];

static int safe_text(const char *s)
{
    return s && *s && !strchr(s,'"') && !strchr(s,'*') &&
           !strchr(s,'\n') && !strchr(s,'\r') && !strchr(s,'`');
}

static int exists(const char *path,int *is_dir)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int ok=0;
    if (is_dir) *is_dir=0;
    lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib) {
        if (Examine(lock,fib)) {
            ok=1;
            if (is_dir) *is_dir=fib->fib_DirEntryType>0;
        }
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    return ok;
}

static int join(char *out,unsigned int cap,const char *dir,const char *name)
{
    if (strlen(dir)+1>cap) return 0;
    strcpy(out,dir);
    return AddPart((STRPTR)out,(STRPTR)name,(LONG)cap)!=0;
}

static int run(const char *command,const char *output)
{
    return beam_system(command,NULL,output,0)==0;
}

static void delete_tree(const char *path)
{
    if (!safe_text(path) || strlen(path)+40>sizeof(cmd)) return;
    if (!exists(path,NULL)) return;
    sprintf(cmd,"C:Delete \"%s\" ALL QUIET FORCE",path);
    run(cmd,NULL);
}

/* An AmigaOS executable, or for .rexx an ARexx script (comment first). */
static int is_hunk_executable(const char *path)
{
    UBYTE hdr[4];
    BPTR f=Open((STRPTR)path,MODE_OLDFILE);
    LONG got;
    unsigned int n=(unsigned int)strlen(path);
    if (!f) return 0;
    got=Read(f,hdr,4);
    Close(f);
    if (n>5 && beam_equal(path+n-5,".rexx")) return got>=2 && hdr[0]=='/' && hdr[1]=='*';
    return got==4 && hdr[0]==0 && hdr[1]==0 && hdr[2]==3 && hdr[3]==0xf3;
}

static int ask(struct Window *win,const char *title,const char *text,const char *buttons)
{
    struct EasyStruct es;
    APTR args[1];
    es.es_StructSize=sizeof(es);
    es.es_Flags=0;
    es.es_Title=(UBYTE *)title;
    es.es_TextFormat=(UBYTE *)"%s"; /* manifest text is never a format */
    es.es_GadgetFormat=(UBYTE *)buttons;
    args[0]=(APTR)text;
    return (int)EasyRequestArgs(win,&es,NULL,args);
}

static int has_suffix_ci(const char *s,const char *suffix)
{
    unsigned int n=(unsigned int)strlen(s),m=(unsigned int)strlen(suffix);
    return n>m && beam_equal(s+n-m,suffix);
}

/* Exactly one *.plugin directly inside dir; its file name goes to name. */
static int find_manifest(const char *dir,char *name,unsigned int cap)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int count=0;
    lock=Lock((STRPTR)dir,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib)) {
        while (ExNext(lock,fib)) {
            if (fib->fib_DirEntryType<0 &&
                has_suffix_ci((const char *)fib->fib_FileName,".plugin")) {
                if (!count && strlen((const char *)fib->fib_FileName)<cap)
                    strcpy(name,(const char *)fib->fib_FileName);
                ++count;
            }
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
    return count==1;
}

/* The chosen drawer itself, or its single sub-drawer, holds the plugin. */
static int locate_package(const char *root,char *drawer,char *manifest_name)
{
    static char sub[BEAM_MAX_PATH];
    BPTR lock;
    struct FileInfoBlock *fib;
    int found=0;
    if (find_manifest(root,manifest_name,BEAM_MAX_NAME)) {
        strcpy(drawer,root);
        return 1;
    }
    lock=Lock((STRPTR)root,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib)) {
        while (ExNext(lock,fib)) {
            if (fib->fib_DirEntryType!=ST_USERDIR) continue;
            if (!join(sub,sizeof(sub),root,(const char *)fib->fib_FileName)) continue;
            if (find_manifest(sub,manifest_name,BEAM_MAX_NAME)) {
                if (!found) strcpy(drawer,sub);
                ++found;
            }
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
    return found==1;
}

static int find_tool(const char *const *names,char *out,unsigned int cap)
{
    int i;
    for (i=0;names[i];i++) {
        if (exists(names[i],NULL) && strlen(names[i])<cap) {
            strcpy(out,names[i]);
            return 1;
        }
    }
    return 0;
}

/* LHA: read the directory natively (headers only, no decompression), so
 * every entry name gets the same strict check as ZIP names. */
static int lha_list_safe(const char *archive,char *msg,unsigned int cap)
{
    static UBYTE buf[BEAM_LHA_HEADER_MAX];
    char name[BEAM_MAX_PATH];
    unsigned long skip;
    LONG pos=0,got,length;
    long rc;
    int entries=0;
    BPTR f=Open((STRPTR)archive,MODE_OLDFILE);
    if (!f) { beam_copy(msg,pstr(PMSG_INST_OPEN_FAILED),cap); return 0; }
    if (Seek(f,0,OFFSET_END)<0 || (length=Seek(f,0,OFFSET_CURRENT))<1) { Close(f); return 0; }
    for (;;) {
        if (Seek(f,pos,OFFSET_BEGINNING)<0) { rc=-1; break; }
        got=Read(f,buf,sizeof(buf));
        if (got<=0) { rc=-1; break; }
        rc=beam_lha_header(buf,(unsigned long)got,name,sizeof(name),&skip);
        if (rc<=0) break;
        if (!beam_package_zip_name_safe(name)) { rc=-3; break; }
        if ((unsigned long)rc>(unsigned long)(length-pos) || skip>(unsigned long)(length-pos)-(unsigned long)rc) { rc=-1; break; }
        pos+=(LONG)rc+(LONG)skip;
        if (++entries>4096) { rc=-1; break; }
    }
    Close(f);
    if (rc==-3) { beam_copy(msg,pstr(PMSG_INST_UNSAFE),cap); return 0; }
    if (rc<0 || !entries) { beam_copy(msg,pstr(PMSG_INST_NOT_LHA),cap); return 0; }
    return 1;
}

static unsigned long zip16(const UBYTE *p) { return (unsigned long)p[0]|((unsigned long)p[1]<<8); }
static unsigned long zip32(const UBYTE *p) { return zip16(p)|(zip16(p+2)<<16); }
static int zip_list_safe(const char *archive)
{
    static UBYTE tail[1024],header[46+BEAM_MAX_PATH],local[30];
    char name[BEAM_MAX_PATH],local_name[BEAM_MAX_PATH];
    struct BeamZipEntry entry;
    BPTR f=Open((STRPTR)archive,MODE_OLDFILE);
    LONG length,read_size,i;
    unsigned long central,central_size,cursor,end,entries,index,total=0;
    int ok=0;
    if (!f) return 0;
    if (Seek(f,0,OFFSET_END)<0 || (length=Seek(f,0,OFFSET_CURRENT))<22) goto done_zip;
    read_size=length<(LONG)sizeof(tail)?length:(LONG)sizeof(tail);
    if (Seek(f,length-read_size,OFFSET_BEGINNING)<0 || Read(f,tail,read_size)!=read_size) goto done_zip;
    for (i=read_size-22;i>=0;i--) if (!memcmp(tail+i,"PK\005\006",4) &&
        zip16(tail+i+20)==(unsigned long)(read_size-i-22)) break;
    if (i<0 || zip16(tail+i+4) || zip16(tail+i+6) || zip16(tail+i+8)!=zip16(tail+i+10)) goto done_zip;
    entries=zip16(tail+i+10); central_size=zip32(tail+i+12); central=zip32(tail+i+16);
    end=(unsigned long)(length-read_size+i);
    if (!entries || entries>4096 || central>end || central_size!=end-central) goto done_zip;
    cursor=central;
    for (index=0;index<entries;index++) {
        unsigned long n,data_start,local_offset;
        long bytes;
        if (cursor>end || end-cursor<46 || Seek(f,(LONG)cursor,OFFSET_BEGINNING)<0 || Read(f,header,46)!=46) goto done_zip;
        n=zip16(header+28);
        if (!n || n>=BEAM_MAX_PATH || end-cursor-46<n || Read(f,header+46,(LONG)n)!=(LONG)n) goto done_zip;
        bytes=beam_zip_header(header,46+n,name,sizeof(name),&entry);
        if (bytes<0 || (unsigned long)bytes>end-cursor || entry.unpacked>64UL*1024UL*1024UL-total) goto done_zip;
        total+=entry.unpacked; local_offset=entry.offset;
        if (central<30 || local_offset>central-30 || Seek(f,(LONG)local_offset,OFFSET_BEGINNING)<0 || Read(f,local,30)!=30 ||
            memcmp(local,"PK\003\004",4) || zip16(local+26)!=n || zip16(local+6)!=entry.flags || zip16(local+8)!=entry.method) goto done_zip;
        data_start=local_offset+30+n+zip16(local+28);
        if (data_start>central || entry.packed>central-data_start || Read(f,local_name,(LONG)n)!=(LONG)n || memcmp(name,local_name,(size_t)n)) goto done_zip;
        cursor+=(unsigned long)bytes;
    }
    ok=cursor==end;
done_zip:
    Close(f); return ok;
}

/* A scratch directory belongs to this attempt only after atomic CreateDir.
 * Existing scratch/staging/backup objects are never deleted to make room. */
static int reserve_install(void)
{
    char candidate[64]; int i; BPTR lock;
    install_tmp[0]=0;
    for (i=0;i<100;i++) {
        sprintf(candidate,"T:Beam68-I%08lx%02x",(unsigned long)FindTask(NULL),i);
        lock=CreateDir((STRPTR)candidate);
        if (lock) { UnLock(lock); strcpy(install_tmp,candidate); return 1; }
    }
    return 0;
}
static int unpack(const char *archive,int kind,char *msg,unsigned int cap)
{
    static const char *const lha_tools[]={"C:LhA",NULL};
    static const char *const zip_tools[]={"C:UnZip",NULL};
    char tool[64];
    install_tmp[0]=0;
    if (!find_tool(kind==BEAM_ARCHIVE_ZIP?zip_tools:lha_tools,tool,sizeof(tool))) {
        beam_copy(msg,kind==BEAM_ARCHIVE_ZIP?pstr(PMSG_INST_NO_UNZIP):pstr(PMSG_INST_NO_LHA),cap); return 0;
    }
    if (!safe_text(archive) || strlen(archive)+100>sizeof(cmd)) {
        beam_copy(msg,pstr(PMSG_INST_BAD_ARCHIVE_PATH),cap); return 0;
    }
    if (kind==BEAM_ARCHIVE_ZIP) {
        if (!zip_list_safe(archive)) {
            beam_copy(msg,"Unsafe or unsupported ZIP: check paths, links, sizes and comments (maximum 1002 bytes). Unpack manually if needed.",cap); return 0;
        }
    } else if (!lha_list_safe(archive,msg,cap)) return 0;
    if (!reserve_install()) { beam_copy(msg,"Cannot reserve a private temporary drawer; existing files were preserved.",cap); return 0; }
    if (kind==BEAM_ARCHIVE_ZIP) sprintf(cmd,"%s -qq -o \"%s\" -d \"%s\"",tool,archive,install_tmp);
    else sprintf(cmd,"%s x \"%s\" \"%s/\"",tool,archive,install_tmp);
    if (!run(cmd,NULL)) { beam_copy(msg,pstr(PMSG_INST_UNPACK_FAILED),cap); return 0; }
    return 1;
}

int beam_plugin_install_candidate(const char *path)
{
    int is_dir=0;
    if (beam_package_archive_kind(path)!=BEAM_ARCHIVE_NONE) return 1;
    return exists(path,&is_dir) && is_dir;
}

int beam_plugin_package_dir(const struct BeamConfig *c,const char *spec,
                            char *out,unsigned int cap)
{
    char drawer[BEAM_MAX_PATH],rel[BEAM_MAX_PATH];
    const char *rest,*slash;
    unsigned int n;
    if (!spec || strlen(spec)<17) return 0;
    beam_copy(rel,spec,17); /* "PROGDIR:Plugins/" */
    if (!beam_equal(rel,"PROGDIR:Plugins/")) return 0;
    rest=spec+16;
    slash=strchr(rest,'/');
    if (!slash || strchr(slash+1,'/') || strchr(rest,':')) return 0;
    n=(unsigned int)(slash-rest);
    if (!n || n>=sizeof(drawer)) return 0;
    memcpy(drawer,rest,n); drawer[n]=0;
    if (!beam_package_drawer_name_valid(drawer)) return 0;
    sprintf(rel,"PROGDIR:Plugins/%.30s",drawer);
    return beam_catalog_resolve(c,rel,out,cap);
}

int beam_plugin_delete_package(const char *package_dir)
{
    char info[BEAM_MAX_PATH+8];
    if (!safe_text(package_dir)) return 0;
    delete_tree(package_dir);
    if (strlen(package_dir)+6<sizeof(info)) {
        sprintf(info,"%s.info",package_dir);
        DeleteFile((STRPTR)info);
    }
    return !exists(package_dir,NULL);
}

int beam_plugin_scan_installed(const struct BeamConfig *c,
                               char (*specs)[BEAM_MAX_PATH],int max)
{
    static char root[BEAM_MAX_PATH],sub[BEAM_MAX_PATH];
    char name[BEAM_MAX_NAME];
    BPTR lock;
    struct FileInfoBlock *fib;
    int count=0;
    if (!beam_catalog_resolve(c,"PROGDIR:Plugins",root,sizeof(root))) return 0;
    lock=Lock((STRPTR)root,ACCESS_READ);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib)) {
        while (count<max && ExNext(lock,fib)) {
            const char *d=(const char *)fib->fib_FileName;
            if (fib->fib_DirEntryType!=ST_USERDIR ||
                !beam_package_drawer_name_valid(d) ||
                !join(sub,sizeof(sub),root,d) ||
                !find_manifest(sub,name,sizeof(name)) ||
                16+strlen(d)+1+strlen(name)+1>BEAM_MAX_PATH) continue;
            sprintf(specs[count++],"PROGDIR:Plugins/%s/%s",d,name);
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
    return count;
}

/* Index of the configured plugin with this ID, or -1. */
static int installed_with_id(const struct BeamConfig *c,const char *id,char *version)
{
    char path[BEAM_MAX_PATH];
    int i;
    for (i=0;i<c->plugin_file_count;i++) {
        if (!beam_catalog_resolve(c,c->plugin_files[i],path,sizeof(path)) ||
            !beam_plugin_read(&other_buf,path)) continue;
        if (other_buf.id[0] && beam_equal(other_buf.id,id)) {
            beam_copy(version,other_buf.plugin_version,16);
            return i;
        }
    }
    return -1;
}

/* Another enabled plugin already uses this trigger or prefix. */
static int input_conflict(const struct BeamConfig *c,const struct BeamPlugin *p,
                          int skip,char *who,unsigned int cap)
{
    char path[BEAM_MAX_PATH];
    int i;
    for (i=0;i<c->plugin_file_count;i++) {
        if (i==skip || !beam_plugin_enabled(c,c->plugin_files[i]) ||
            !beam_catalog_resolve(c,c->plugin_files[i],path,sizeof(path)) ||
            !beam_plugin_read(&other_buf,path)) continue;
        if (other_buf.trigger==p->trigger ||
            (p->prefix[0] && other_buf.prefix[0] && beam_equal(p->prefix,other_buf.prefix))) {
            beam_copy(who,other_buf.name,cap);
            return 1;
        }
    }
    return 0;
}

int beam_plugin_install(struct Window *win,struct BeamConfig *c,
                        const char *source,char *msg,unsigned int cap)
{
    static char root[BEAM_MAX_PATH],drawer[BEAM_MAX_PATH],manifest[BEAM_MAX_PATH];
    static char exe[BEAM_MAX_PATH],plugins_dir[BEAM_MAX_PATH],target[BEAM_MAX_PATH];
    static char staging[BEAM_MAX_PATH+8],old[BEAM_MAX_PATH+8],spec[BEAM_MAX_PATH];
    char manifest_name[BEAM_MAX_NAME],drawer_name[BEAM_MAX_NAME];
    char old_version[16],conflict_with[BEAM_MAX_NAME];
    const char *base;
    int kind,is_dir=0,existing,conflict,ok=0,replaced=0,cmp=1,i;
    BPTR lock;
    if (!msg || !cap) return 0;
    msg[0]=0;
    if (!c || !source || !*source) { beam_copy(msg,pstr(PMSG_INST_NOTHING),cap); return 0; }
    kind=beam_package_archive_kind(source);
    if (kind!=BEAM_ARCHIVE_NONE) {
        if (!unpack(source,kind,msg,cap)) { if (install_tmp[0]) delete_tree(install_tmp); return 0; }
        beam_copy(root,INSTALL_TMP,sizeof(root));
    } else {
        if (!exists(source,&is_dir) || !is_dir) {
            beam_copy(msg,pstr(PMSG_INST_CHOOSE),cap); return 0;
        }
        lock=Lock((STRPTR)source,ACCESS_READ);
        if (!lock || !NameFromLock(lock,root,sizeof(root))) {
            if (lock) UnLock(lock);
            beam_copy(msg,pstr(PMSG_INST_DRAWER_LONG),cap); return 0;
        }
        UnLock(lock);
    }
    if (!locate_package(root,drawer,manifest_name)) {
        beam_copy(msg,pstr(PMSG_INST_NO_PACKAGE),cap);
        goto done;
    }
    if (!join(manifest,sizeof(manifest),drawer,manifest_name) ||
        !beam_plugin_read(&manifest_buf,manifest)) {
        beam_copy(msg,pstr(PMSG_INST_BAD_MANIFEST),cap);
        goto done;
    }
    {
        static const int texts[]={0,PMSG_PKG_NO_MANIFEST,PMSG_PKG_BAD_ID,PMSG_PKG_NO_VERSION,
            PMSG_PKG_TOO_NEW,PMSG_PKG_BAD_COMMAND,PMSG_PKG_BAD_ICON,PMSG_PKG_BAD_GUIDE,
            PMSG_PKG_BAD_SETTINGS};
        int problem=beam_package_manifest_problem(&manifest_buf);
        if (problem!=BEAM_PKG_OK) { beam_copy(msg,pstr(texts[problem]),cap); goto done; }
    }
    if (!beam_plugindir_resolve(manifest,manifest_buf.command,exe,sizeof(exe)) ||
        !is_hunk_executable(exe)) {
        beam_copy(msg,pstr(PMSG_INST_BAD_EXE),cap);
        goto done;
    }
    /* The installed drawer is named after the package drawer, or after the
       manifest when the archive has no top-level drawer. */
    base=(const char *)FilePart((STRPTR)drawer);
    if (beam_equal(drawer,INSTALL_TMP) || !beam_package_drawer_name_valid(base)) {
        unsigned int n=(unsigned int)strlen(manifest_name)-7;
        memcpy(drawer_name,manifest_name,n); drawer_name[n]=0;
    } else beam_copy(drawer_name,base,sizeof(drawer_name));
    if (!beam_package_drawer_name_valid(drawer_name)) {
        beam_copy(msg,pstr(PMSG_INST_BAD_DRAWER),cap); goto done;
    }
    if (!beam_catalog_resolve(c,"PROGDIR:Plugins",plugins_dir,sizeof(plugins_dir)) ||
        !join(target,sizeof(target),plugins_dir,drawer_name) ||
        strlen(target)+5>sizeof(staging)) {
        beam_copy(msg,pstr(PMSG_INST_CHECK_DIR),cap); goto done;
    }
    if (16+strlen(drawer_name)+1+strlen(manifest_name)+1>sizeof(spec)) { beam_copy(msg,pstr(PMSG_INST_PATH_LONG),cap); goto done; }
    sprintf(spec,"PROGDIR:Plugins/%.30s/%.63s",drawer_name,manifest_name);
    existing=installed_with_id(c,manifest_buf.id,old_version);
    /* A drawer of this name must belong to the same plugin ID. */
    if (exists(target,NULL)) {
        static char tmp_name[BEAM_MAX_NAME],tmp_path[BEAM_MAX_PATH];
        if (!find_manifest(target,tmp_name,sizeof(tmp_name)) ||
            !join(tmp_path,sizeof(tmp_path),target,tmp_name) ||
            !beam_plugin_read(&other_buf,tmp_path) ||
            !beam_equal(other_buf.id,manifest_buf.id)) {
            sprintf(body,pstr(PMSG_INST_DRAWER_TAKEN),drawer_name);
            beam_copy(msg,body,cap); goto done;
        }
    }
    if (existing<0 && c->plugin_file_count>=BEAM_MAX_PLUGINS) {
        beam_copy(msg,pstr(PMSG_INST_FULL),cap); goto done;
    }
    if (existing>=0 && !beam_equal(c->plugin_files[existing],spec)) {
        beam_copy(msg,"This plugin ID is installed in another drawer. Remove that package explicitly before moving it.",cap); goto done;
    }
    conflict=input_conflict(c,&manifest_buf,existing,conflict_with,sizeof(conflict_with));
    if (existing>=0) cmp=beam_version_compare(manifest_buf.plugin_version,old_version);
    sprintf(body,pstr(PMSG_INST_CONSENT),
        manifest_buf.name,manifest_buf.plugin_version,
        manifest_buf.author[0]?manifest_buf.author:pstr(PMSG_INST_UNKNOWN_AUTHOR),
        manifest_buf.id,manifest_buf.license[0]?manifest_buf.license:pstr(PMSG_INST_NOT_STATED),
        manifest_buf.capabilities[0]?manifest_buf.capabilities:pstr(PMSG_INST_NONE_DECLARED),
        beam_plugin_has_capability(&manifest_buf,"SYSTEM")?
            pstr(PMSG_INST_SYSTEM_WARNING):"",
        conflict?pstr(PMSG_INST_CONFLICT):"",conflict?conflict_with:"",
        conflict?pstr(PMSG_INST_CONFLICT_END):"",
        existing<0?"":cmp>0?pstr(PMSG_INST_UPGRADE):cmp==0?
            pstr(PMSG_INST_REINSTALL):pstr(PMSG_INST_DOWNGRADE),
        existing<0?"":old_version,existing<0?"":".",
        target);
    if (ask(win,pstr(PMSG_INST_TITLE),body,pstr(PMSG_INST_BUTTONS))!=1) {
        beam_copy(msg,pstr(PMSG_INST_CANCELLED),cap); goto done;
    }
    /* Copy into <Name>.new first, then swap it into place. */
    lock=CreateDir((STRPTR)plugins_dir);
    if (lock) UnLock(lock);
    lock=0;
    for (i=0;i<100;i++) {
        char leaf[16]; sprintf(leaf,"B68n%02x",i);
        if (!join(staging,sizeof(staging),plugins_dir,leaf)) break;
        if (beam_equal(staging,target)) continue;
        lock=CreateDir((STRPTR)staging);
        if (lock) break;
    }
    if (i==100 || !lock) { beam_copy(msg,pstr(PMSG_INST_CREATE_FAILED),cap); goto done; }
    UnLock(lock);
    if (!safe_text(drawer) || strlen(drawer)+strlen(staging)+40>sizeof(cmd)) {
        beam_copy(msg,pstr(PMSG_INST_PATH_LONG),cap); delete_tree(staging); goto done;
    }
    sprintf(cmd,"C:Copy \"%s/#?\" TO \"%s\" ALL CLONE QUIET",drawer,staging);
    if (!run(cmd,NULL) || !find_manifest(staging,manifest_name,sizeof(manifest_name))) {
        delete_tree(staging);
        beam_copy(msg,pstr(PMSG_INST_COPY_FAILED),cap); goto done;
    }
    if (exists(target,NULL)) {
        int moved=0;
        for (i=0;i<100;i++) {
            char leaf[16]; sprintf(leaf,"B68o%02x",i);
            if (!join(old,sizeof(old),plugins_dir,leaf)) break;
            if (beam_equal(old,target) || exists(old,NULL)) continue;
            if (Rename((STRPTR)target,(STRPTR)old)) { moved=1; break; }
        }
        if (!moved) {
            delete_tree(staging);
            beam_copy(msg,pstr(PMSG_INST_IN_USE),cap); goto done;
        }
        replaced=1;
    }
    if (!Rename((STRPTR)staging,(STRPTR)target)) {
        int restored=!replaced || Rename((STRPTR)old,(STRPTR)target);
        delete_tree(staging);
        if (restored) beam_copy(msg,pstr(PMSG_INST_MOVE_FAILED),cap);
        else {
            sprintf(body,"Update failed. The original remains in Plugins/%s; restore it before retrying.",(const char *)FilePart((STRPTR)old));
            beam_copy(msg,body,cap);
        }
        goto done;
    }
    if (replaced) delete_tree(old);
    /* Configuration: replace the old entry for this ID, or append. */
    if (existing>=0) {
        if (!beam_equal(c->plugin_files[existing],spec)) {
            beam_plugin_forget_disabled(c,c->plugin_files[existing]);
            beam_copy(c->plugin_files[existing],spec,BEAM_MAX_PATH);
        }
    } else {
        for (i=0;i<c->plugin_file_count;i++)
            if (beam_equal(c->plugin_files[i],spec)) break;
        if (i==c->plugin_file_count)
            beam_copy(c->plugin_files[c->plugin_file_count++],spec,BEAM_MAX_PATH);
    }
    beam_plugin_set_enabled(c,spec,!conflict);
    sprintf(body,pstr(PMSG_INST_DONE),manifest_buf.name,
            manifest_buf.plugin_version,conflict?pstr(PMSG_INST_DISABLED_NOTE):"");
    beam_copy(msg,body,cap);
    ok=1;
done:
    if (kind!=BEAM_ARCHIVE_NONE) delete_tree(INSTALL_TMP);
    return ok;
}
