#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "history.h"
static void write_file(const char *path,const char *data)
{
    FILE *f=fopen(path,"wb"); assert(f);
    assert(fputs(data,f)>=0); assert(!fclose(f));
}
int main(void)
{
    static struct BeamHistory h,r;
    unsigned int i;
    if (system("mkdir -p build/testsettings/ENVARC/Beam68")!=0) { fputs("test setup command failed\n",stderr); return 1; }
    beam_history_reset(&h);
    assert(!beam_history_bonus(&h,"SYS:Tools/Ed"));
    beam_history_record(&h,"SYS:Tools/Ed");
    beam_history_record(&h,"sys:tools/ed");
    assert(beam_history_bonus(&h,"SYS:Tools/Ed")==12);
    assert(beam_history_recent_position(&h,"SYS:Tools/Ed")==0);
    assert(h.count==1 && h.pending==2);
    assert(beam_history_save(&h,BEAM_HISTORY_PATH));
    assert(h.pending==0);
    assert(beam_history_load(&r,BEAM_HISTORY_PATH));
    assert(beam_history_bonus(&r,"sys:tools/ED")==12);
    beam_history_record(&r,"SYS:Utilities/Multi Tool");
    assert(beam_history_recent_position(&r,"SYS:Utilities/Multi Tool")==0);
    assert(beam_history_recent_position(&r,"SYS:Tools/Ed")==1);
    assert(beam_history_save(&r,BEAM_HISTORY_PATH));
    assert(beam_history_load(&h,BEAM_HISTORY_PATH));
    assert(h.count==2);
    assert(beam_history_bonus(&h,"SYS:Utilities/Multi Tool")==6);
    write_file("build/testsettings/ENVARC/Beam68/History.txt",
               "9\tSYS:Tools/Ed\ninvalid\tAnything\n2\t\n-1\tBad\n1\tSYS:Prefs/Workbench\n");
    assert(beam_history_load(&h,BEAM_HISTORY_PATH));
    assert(h.count==2 && beam_history_bonus(&h,"SYS:Tools/Ed")==54);
    for(i=0;i<100;i++) {
        char path[60];
        sprintf(path,"SYS:Other/App%u",i);
        beam_history_record(&h,path);
    }
    assert(h.count==BEAM_HISTORY_CAP);
    for(i=0;i<50;i++) beam_history_record(&h,"SYS:Tools/Ed");
    assert(beam_history_bonus(&h,"SYS:Tools/Ed")<=144);
    assert(beam_history_save(&h,BEAM_HISTORY_PATH));
    assert(beam_history_load(&r,BEAM_HISTORY_PATH));
    assert(r.count==BEAM_HISTORY_CAP);
    puts("Bounded text history, load/save, case folding, saturation: PASS");
    return 0;
}
