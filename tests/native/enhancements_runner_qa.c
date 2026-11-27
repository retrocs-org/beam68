/* Synchronous isolated runner; every child must finish before the next starts. */
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include "stack_guard.h"
static int run_qa(int argc,char **argv)
{
    static const char *commands[]={
        "Builds:Beam68LauncherUIQA >AutoRun:enhanced-launcher.log",
        "Builds:Beam68/Beam68Prefs LAYOUTQA 200 >AutoRun:enhanced-prefs200.log",
        "Builds:Beam68/Beam68Prefs LAYOUTQA 256 >AutoRun:enhanced-prefs256.log",
        "Builds:Beam68/Beam68Prefs LAYOUTQA 400 >AutoRun:enhanced-prefs400.log",
        "Builds:Beam68/Beam68Prefs SETTINGSQA >AutoRun:enhanced-settings.log",
        "Builds:PerformanceBefore >AutoRun:performance-before1.log",
        "Builds:PerformanceAfter >AutoRun:performance-after1.log",
        "Builds:PerformanceAfter >AutoRun:performance-after2.log",
        "Builds:PerformanceBefore >AutoRun:performance-before2.log"
    };
    FILE *proof;int i;LONG code;(void)argc;(void)argv;
    proof=fopen("AutoRun:enhancements-proof.txt","wb");if(!proof)return 20;
    for(i=0;i<9;i++) {
        fprintf(proof,"START %d: %s\n",i+1,commands[i]);fflush(proof);
        code=SystemTagList((STRPTR)commands[i],NULL);
        fprintf(proof,"EXIT %ld\n",code);fflush(proof);
        if(code){fclose(proof);return 20;}
    }
    fputs("PASS: all nine synchronous checks completed.\n",proof);fclose(proof);return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(32768UL,run_qa,argc,argv);}
