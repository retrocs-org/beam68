/* Run only in an isolated test environment. Writes exclusively to a newly
 * reserved T:Beam68QAxx drawer; never reads or edits the active settings. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "preset.h"
#include "beam.h"
#include "search_rank.h"
void qa_fail_writes(int n);
void qa_fail_rename(int n);
static int put(const char *p,const char *text)
{
    BPTR f=Open((STRPTR)p,MODE_NEWFILE);int ok;
    if (!f) return 0;
    ok=FPuts(f,(STRPTR)text)>=0; if (!Close(f)) ok=0; return ok;
}
static int has(const char *p,const char *text)
{
    char buf[256];LONG n;BPTR f=Open((STRPTR)p,MODE_OLDFILE);
    if (!f) return 0;
    n=Read(f,buf,sizeof(buf)-1);Close(f);if(n<0)return 0;
    buf[n]=0;return strstr(buf,text)!=NULL;
}
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n",__LINE__,#x); return 20; } } while(0)
int main(void)
{
    char root[80],dir[100],preset[100],target[120];
    const char *dirs[1];BPTR lock=0;int slot,i,n,k;
    static const int sizes[]={128,1024,4096};
    struct DateStamp start,end;
    volatile unsigned long checksum=0;
    static struct BeamConfig fresh,reloaded;
    for(slot=0;slot<100;slot++) {
        sprintf(root,"T:Beam68QA%02d",slot);lock=CreateDir((STRPTR)root);
        if(lock)break;
    }
    CHECK(lock);UnLock(lock);
    printf("Native recovery checks; isolated files: %s\n",root);
    sprintf(dir,"%s/settings",root);lock=CreateDir((STRPTR)dir);CHECK(lock);UnLock(lock);
    dirs[0]=dir;sprintf(preset,"%s/input.preset",root);sprintf(target,"%s/Beam68.cfg",dir);
    CHECK(put(target,"ORIGINAL\n"));
    CHECK(put(preset,"BEAM68PRESET 1\n=FILE Beam68.cfg\n|REPLACEMENT\n=END\n"));
    qa_fail_writes(0);CHECK(beam_preset_import(preset,dirs,1)==-1);CHECK(has(target,"ORIGINAL\n"));
    puts("PASS: simulated disk-full write preserves original on native filesystem");
    qa_fail_rename(1);CHECK(beam_preset_import(preset,dirs,1)==-1);CHECK(has(target,"ORIGINAL\n"));
    puts("PASS: commit rename failure restores original on native filesystem");
    CHECK(put(preset,"BEAM68PRESET 1\n=FILE Beam68.cfg\n|INCOMPLETE\n"));
    CHECK(beam_preset_import(preset,dirs,1)==-1);CHECK(has(target,"ORIGINAL\n"));
    puts("PASS: interrupted/truncated preset rejected without active writes");
    CHECK(put(preset,"BEAM68PRESET 1\n=FILE Beam68.cfg\n|REPLACEMENT\n=END\n"));
    CHECK(beam_preset_import(preset,dirs,1)==1);CHECK(has(target,"REPLACEMENT\n"));
    puts("PASS: successful native commit");
    beam_config_defaults(&fresh);
    fresh.max_items=1234;
    CHECK(beam_config_write(&fresh,target));
    beam_config_defaults(&reloaded);
    CHECK(beam_config_read(&reloaded,target));
    CHECK(reloaded.max_items==1234 && beam_config_valid(&reloaded));
    CHECK(DeleteFile((STRPTR)target));
    CHECK(!Lock((STRPTR)target,ACCESS_READ));
    puts("PASS: isolated defaults, save, reload and settings removal");
    /* Reproducible CPU matcher baseline. Not a UI/disk/startup benchmark. */
    for(k=0;k<3;k++) {
        n=sizes[k];
        DateStamp(&start);
        for(i=0;i<n*20;i++) checksum+=(unsigned long)beam_search_rank_alias("scr","ScreenMode","display screen",-1,0);
        DateStamp(&end);
        printf("matcher items=%d repeats=20 elapsed_ticks=%ld (50Hz) checksum=%lu\n",n,
            (end.ds_Days-start.ds_Days)*4320000L+(end.ds_Minute-start.ds_Minute)*3000L+end.ds_Tick-start.ds_Tick,checksum);
    }
    puts("Native recovery and matcher baseline: PASS");
    return 0;
}
