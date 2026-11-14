/* Portable text index cache, AmigaDOS API; no FPU.
 * A non-matching/truncated/tampered cache is discarded and rebuilt by caller.
 *
 * Schema 5: one line per item, "kind<TAB>root<TAB>path[<TAB>title]".  Only '%', control
 * bytes and DEL are %XX-escaped, so a typical line is about half the size of
 * the old all-hex schema 3.  The cache is disposable: it is written through
 * a .new file with committed .bak/.old recovery candidates.
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index_cache.h"
#define CACHE_MAGIC "BEAM68CACHE5"
#define CACHE_LINE (BEAM_MAX_PATH*6+40)
#define U32_MASK 0xffffffffUL

static unsigned long hash_byte(unsigned long h, unsigned int c)
{
    return (((h<<5)-h)+(unsigned long)c)&U32_MASK;
}
static unsigned long hash_string(unsigned long h,const char *s)
{
    while (*s) h=hash_byte(h,(unsigned char)*s++);
    return hash_byte(h,0); /* separator */
}
static unsigned long hash_number(unsigned long h,unsigned long n)
{
    int i;
    for (i=0;i<4;i++) { h=hash_byte(h,(unsigned int)(n&255UL)); n>>=8; }
    return h;
}
static unsigned long fingerprint(const struct BeamConfig *c)
{
    int i;
    unsigned long h=5381UL;
    h=hash_number(h,5UL);
    h=hash_number(h,(unsigned long)c->dir_count);
    h=hash_number(h,(unsigned long)c->max_items);
    h=hash_number(h,(unsigned long)c->max_depth);
    h=hash_number(h,(unsigned long)c->scan_icons);
    h=hash_number(h,(unsigned long)c->scan_executables);
    h=hash_number(h,(unsigned long)c->index_files);
    h=hash_number(h,(unsigned long)c->index_drawers);
    h=hash_number(h,(unsigned long)c->index_games);
    h=hash_number(h,(unsigned long)c->index_rexx);
    h=hash_number(h,(unsigned long)c->index_modules);
    h=hash_string(h,c->bookmark_files);
    h=hash_string(h,c->file_patterns);
    for (i=0;i<c->dir_count;i++) h=hash_string(h,c->dirs[i]);
    h=hash_number(h,(unsigned long)c->excluded_dir_count);
    for (i=0;i<c->excluded_dir_count;i++) h=hash_string(h,c->excluded_dirs[i]);
    return h;
}
static unsigned long hash_item(unsigned long h,const struct BeamItem *it)
{
    h=hash_byte(h,it->kind);
    h=hash_byte(h,it->root);
    h=hash_string(h,it->path);
    return it->title?hash_string(h,it->title):h;
}
static int hex_value(int c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='a' && c<='f') return c-'a'+10;
    if (c>='A' && c<='F') return c-'A'+10;
    return -1;
}
static void escape_path(const char *s,char *out)
{
    static const char digits[]="0123456789ABCDEF";
    while (*s) {
        unsigned int b=(unsigned char)*s++;
        if (b<32 || b==127 || b=='%') {
            *out++='%'; *out++=digits[b>>4]; *out++=digits[b&15];
        } else *out++=(char)b;
    }
    *out=0;
}
static int unescape_path(const char *s,char *out,unsigned int cap)
{
    unsigned int n=0;
    while (*s) {
        unsigned int c=(unsigned char)*s++;
        if (c=='%') {
            int hi=hex_value((unsigned char)s[0]);
            int lo=hi<0?-1:hex_value((unsigned char)s[1]);
            if (hi<0 || lo<0) return 0;
            c=(unsigned int)(hi*16+lo); s+=2;
            if (!c || c=='\r' || c=='\n') return 0;
        } else if (c<32 || c==127) return 0;
        if (n+1>=cap) return 0;
        out[n++]=(char)c;
    }
    out[n]=0;
    return n>0;
}
/* Require a complete line including a newline to reject damaged/truncated data. */
static int get_line(BPTR f,char *line,unsigned int cap)
{
    unsigned int n;
    if (!FGets(f,line,(LONG)cap)) return 0;
    n=(unsigned int)strlen(line);
    if (!n || line[n-1]!='\n') return 0;
    line[--n]=0;
    if (n && line[n-1]=='\r') line[n-1]=0;
    return 1;
}
static int parse_number(const char *s,int base,unsigned long *out)
{
    char *end;
    unsigned long value;
    if (!*s || *s=='-' || *s=='+' || *s==' ') return 0;
    value=strtoul(s,&end,base);
    if (*end || value>U32_MASK) return 0;
    *out=value;
    return 1;
}

static int cache_load_into(const struct BeamConfig *c,struct BeamIndex *x,const char *path)
{
    BPTR f;
    static char line[CACHE_LINE];
    char item_path[BEAM_MAX_PATH],*sep;
    static char item_title[BEAM_MAX_PATH];
    unsigned long fp,total,sum=5381UL,read_sum,root;
    int i,ok=0,resource_failure=0;
    beam_index_clear(x);
    if (!c->index_cache || !path || strlen(path)>=BEAM_MAX_PATH+12U) return 0;
    f=Open((STRPTR)path,MODE_OLDFILE);
    if (!f) return 0;
    if (!get_line(f,line,sizeof(line)) ||
        strncmp(line,CACHE_MAGIC "\t",strlen(CACHE_MAGIC)+1)) goto out;
    sep=strchr(line+strlen(CACHE_MAGIC)+1,'\t');
    if (!sep) goto out;
    *sep++=0;
    if (!parse_number(line+strlen(CACHE_MAGIC)+1,16,&fp) ||
        !parse_number(sep,10,&total) || fp!=fingerprint(c) ||
        total>(unsigned long)c->max_items ||
        total>(unsigned long)x->capacity) goto out;
    for (i=0;i<(int)total;i++) {
        char *root_text,*path_text;
        int kind,index,created;
        unsigned long kind_value;
        if (!get_line(f,line,sizeof(line)) || !(sep=strchr(line,'\t'))) goto out;
        *sep=0;
        if (!parse_number(line,10,&kind_value) || kind_value<1 ||
            kind_value>BEAM_ITEM_KIND_MAX || kind_value==BEAM_ITEM_ASSIGN) goto out;
        kind=(int)kind_value;
        root_text=sep+1;
        sep=strchr(root_text,'\t');
        if (!sep) goto out;
        *sep++=0; path_text=sep;
        item_title[0]=0;
        sep=strchr(path_text,'\t');
        if (sep) {
            *sep++=0;
            if (!unescape_path(sep,item_title,sizeof(item_title))) goto out;
        }
        if (!parse_number(root_text,10,&root) ||
            (root!=BEAM_ROOT_NONE && root!=BEAM_ROOT_SOURCES &&
             root>=(unsigned long)c->dir_count) ||
            ((kind==BEAM_ITEM_VOLUME)!=(root==BEAM_ROOT_NONE)) ||
            !unescape_path(path_text,item_path,sizeof(item_path)) ||
            strchr(item_path,'"') || strchr(item_path,'*')) goto out;
        if (kind==BEAM_ITEM_SNIPPET && !item_title[0]) goto out;
        index=beam_index_add(x,item_path,kind,(int)root,&created);
        if (index<0 || !created) {
            if (index==-1) resource_failure=1;
            goto out;
        }
        if (item_title[0] && !beam_index_set_title(x,index,item_title)) {
            resource_failure=1;goto out;
        }
        sum=hash_item(sum,&x->items[index]);
    }
    if (!get_line(f,line,sizeof(line)) || strncmp(line,"END\t",4) ||
        !parse_number(line+4,16,&read_sum) || read_sum!=sum) goto out;
    /* Strictly reject surplus data after END. */
    { char extra[2]; if (FGets(f,extra,sizeof(extra))) goto out; }
    ok=1;
out:
    Close(f);
    if (!ok) beam_index_clear(x);
    return resource_failure?-1:ok;
}

/* A fully validated candidate can repair interrupted rename state. If any
 * rename fails, leave the candidate intact and keep the in-memory recovery. */
static void cache_promote(const char *path,const char *candidate)
{
    char temporary[BEAM_MAX_PATH+12],old[BEAM_MAX_PATH+12];
    BPTR lock;
    int moved=0;
    sprintf(temporary,"%s.new",path);sprintf(old,"%s.old",path);
    if (!strcmp(path,candidate)) {
        char backup[BEAM_MAX_PATH+12];
        DeleteFile((STRPTR)temporary);
        sprintf(backup,"%s.bak",path);
        lock=Lock((STRPTR)backup,ACCESS_READ);
        if (lock) { UnLock(lock);DeleteFile((STRPTR)old); }
        else Rename((STRPTR)old,(STRPTR)backup);
        return;
    }
    lock=Lock((STRPTR)path,ACCESS_READ);
    if (lock) {
        UnLock(lock);
        lock=Lock((STRPTR)temporary,ACCESS_READ);
        if (lock) {
            UnLock(lock);
            if (!DeleteFile((STRPTR)temporary)) return;
        }
        if (!Rename((STRPTR)path,(STRPTR)temporary)) return;
        moved=1;
    }
    if (!Rename((STRPTR)candidate,(STRPTR)path)) {
        if (moved) Rename((STRPTR)temporary,(STRPTR)path);
        return;
    }
    DeleteFile((STRPTR)temporary);DeleteFile((STRPTR)old);
}

static int cache_load_recover(const struct BeamConfig *c,struct BeamIndex *x,const char *path)
{
    char candidate[BEAM_MAX_PATH+12];
    int result=cache_load_into(c,x,path);
    if (result<0) return 0; /* memory pressure is not proof of corruption */
    if (result) { cache_promote(path,path);return 1; }
    /* .old protects the latest primary while an existing .bak is retained. */
    sprintf(candidate,"%s.old",path);
    result=cache_load_into(c,x,candidate);
    if (result<0) return 0;
    if (result) { cache_promote(path,candidate);return 1; }
    sprintf(candidate,"%s.bak",path);
    result=cache_load_into(c,x,candidate);
    if (result>0) { cache_promote(path,candidate);return 1; }
    return 0;
}

int beam_cache_load(const struct BeamConfig *c,struct BeamIndex *x,const char *path)
{
    struct BeamIndex next,old;
    if (!x || !x->items || !x->pool || !x->hash || !c || !c->index_cache || !path || strlen(path)>=BEAM_MAX_PATH) return 0;
    /* Startup has no live entries to protect. Reload commits only a fully
       validated index; corruption or low memory leaves current results intact. */
    if (!x->count) return cache_load_recover(c,x,path);
    if (!beam_index_init(&next,x->capacity)) return 0;
    if (!cache_load_recover(c,&next,path)) { beam_index_free(&next);return 0; }
    old=*x;*x=next;beam_index_free(&old);
    return 1;
}

static int put_line(BPTR f,const char *s)
{
    return FPuts(f,(STRPTR)s)>=0;
}
int beam_cache_save(const struct BeamConfig *c,const struct BeamIndex *x,
                    const char *path)
{
    BPTR f,lock;
    int i,ok=1,had_original=0,retained_backup=0;
    char tmp[BEAM_MAX_PATH+12],backup[BEAM_MAX_PATH+12],old[BEAM_MAX_PATH+12];
    char header[96];
    static char line[CACHE_LINE],escaped[BEAM_MAX_PATH*3];
    unsigned long checksum=5381UL;
    if (!c || !x || !c->index_cache || !path || x->full || x->count<0 || x->count>c->max_items ||
        strlen(path)>=BEAM_MAX_PATH) return 0;
    sprintf(tmp,"%s.new",path);
    sprintf(backup,"%s.bak",path);sprintf(old,"%s.old",path);
    /* Never overwrite unresolved recovery state before a new commit. */
    lock=Lock((STRPTR)old,ACCESS_READ);
    if (lock) { UnLock(lock);return 0; }
    f=Open((STRPTR)tmp,MODE_NEWFILE);
    if (!f) return 0;
    sprintf(header,CACHE_MAGIC "\t%08lX\t%ld\n",fingerprint(c),(long)x->count);
    ok=put_line(f,header);
    for (i=0;ok && i<x->count;i++) {
        const struct BeamItem *it=&x->items[i];
        if (it->kind<BEAM_ITEM_WORKBENCH || it->kind>BEAM_ITEM_KIND_MAX ||
            it->kind==BEAM_ITEM_ASSIGN ||
            !it->path || !it->path[0] ||
            strchr(it->path,'"') || strchr(it->path,'*')) { ok=0; break; }
        escape_path(it->path,escaped);
        sprintf(line,"%d\t%u\t%s",(int)it->kind,(unsigned int)it->root,escaped);
        if (it->title) {
            escape_path(it->title,escaped);
            strcat(line,"\t"); strcat(line,escaped);
        }
        strcat(line,"\n");
        ok=put_line(f,line);
        checksum=hash_item(checksum,it);
    }
    if (ok) {
        sprintf(header,"END\t%08lX\n",checksum);
        ok=put_line(f,header);
    }
    if (Close(f)==0) ok=0;
    if (!ok) { DeleteFile((STRPTR)tmp); return 0; }
    lock=Lock((STRPTR)path,ACCESS_READ);
    if (lock) {
        UnLock(lock);
        lock=Lock((STRPTR)backup,ACCESS_READ);
        if (lock) { UnLock(lock);retained_backup=1; }
        if (!Rename((STRPTR)path,(STRPTR)(retained_backup?old:backup))) {
            DeleteFile((STRPTR)tmp);return 0;
        }
        had_original=1;
    }
    if (!Rename((STRPTR)tmp,(STRPTR)path)) {
        if (had_original) Rename((STRPTR)(retained_backup?old:backup),(STRPTR)path);
        DeleteFile((STRPTR)tmp);return 0;
    }
    /* The new primary is committed before retiring an older backup. */
    if (had_original && retained_backup) {
        lock=Lock((STRPTR)backup,ACCESS_READ);
        if (lock) {
            UnLock(lock);
            if (!DeleteFile((STRPTR)backup)) return 1;
        }
        Rename((STRPTR)old,(STRPTR)backup);
    }
    return 1;
}
