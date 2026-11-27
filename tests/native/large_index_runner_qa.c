/* Synthetic large-index workloads run sequentially in one disposable guest. */
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <stdio.h>
#include "stack_guard.h"
static int run_qa(int argc,char **argv)
{
    const char *commands[]={"Builds:Beam68PerformanceQA 2000 >AutoRun:bench-2000.log","Builds:Beam68PerformanceQA 10000 >AutoRun:bench-10000.log","Builds:Beam68PerformanceQA 50000 >AutoRun:bench-50000.log"};
    FILE *proof;LONG code;int i;(void)argc;(void)argv;
    proof=fopen("AutoRun:benchmark-proof.txt","w");if(!proof)return 20;
    for(i=0;i<3;i++) {
        fprintf(proof,"START %s\n",commands[i]);fflush(proof);
        code=SystemTags((STRPTR)commands[i],NP_StackSize,65536UL,TAG_DONE);
        fprintf(proof,"EXIT %ld\n",code);fflush(proof);
        if(code){fclose(proof);return 20;}
    }
    fputs("PASS: all synthetic workloads completed.\n",proof);fclose(proof);return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,run_qa,argc,argv);}
