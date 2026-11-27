/* Synchronous native regressions; parent and child both have guarded stacks. */
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include "stack_guard.h"
static int run_qa(int argc,char **argv){(void)argc;(void)argv;return (int)SystemTags("Builds:Beam68LauncherUIQA >AutoRun:cache-polish.log",NP_StackSize,65536UL,TAG_DONE);}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,run_qa,argc,argv);}
