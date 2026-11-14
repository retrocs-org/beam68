/* Native regression: returning from a swapped stack must restore the caller. */
#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack_guard.h"
static int calls;
static int entry(int argc,char **argv)
{
    volatile unsigned long local[256];
    int i;
    if(argc!=7 || !argv || argv[0][0]!='s') return 20;
    for(i=0;i<256;i++) local[i]=(unsigned long)i+argc;
    for(i=0;i<256;i++) if(local[i]!=(unsigned long)i+7) return 20;
    calls++;
    return 37;
}
int main(void)
{
    struct Task *task=FindTask(NULL);
    APTR lower=task->tc_SPLower,upper=task->tc_SPUpper;
    char *args[]={"stack",0};
    int i;
    for(i=0;i<32;i++) {
        if(beam_run_with_stack(65536UL,entry,7,args)!=37 ||
           task->tc_SPLower!=lower || task->tc_SPUpper!=upper) return 20;
    }
    if(beam_run_with_stack(1024UL,entry,7,args)!=37 || calls!=33) return 20;
    PutStr("Stack guard: swapped/direct return, arguments and restored bounds: PASS\n");
    return 0;
}
