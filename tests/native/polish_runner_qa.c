/* Native synchronous runner: Execute's script handoff cannot prove child exit. */
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include "stack_guard.h"
static int run_qa(int argc,char **argv)
{
 FILE *proof;LONG code;(void)argc;(void)argv;
 proof=fopen("AutoRun:polish-proof.txt","wb");if(!proof)return 20;
 fputs("Native runner started.\n",proof);fflush(proof);
 code=SystemTagList("Builds:Beam68QueryStateQA >AutoRun:query-state.log",NULL);
 fprintf(proof,"Query-state exit: %ld\n",code);fflush(proof);
 if(code){fclose(proof);return 20;}
 code=SystemTagList("Builds:Beam68AminetCompletionQA >AutoRun:aminet-completion.log",NULL);
 fprintf(proof,"Aminet-completion exit: %ld\n",code);fflush(proof);
 if(code){fclose(proof);return 20;}
 code=SystemTagList("Builds:Beam68AmiHASSQA >AutoRun:amihass-behavior.log",NULL);
 fprintf(proof,"AmiHASS-behavior exit: %ld\n",code);fflush(proof);
 if(code){fclose(proof);return 20;}
 code=SystemTagList("Builds:Beam68AmiHASSTransportQA >AutoRun:amihass-transport.log",NULL);
 fprintf(proof,"AmiHASS-transport exit: %ld\n",code);fflush(proof);
 if(code){fclose(proof);return 20;}
 fputs("PASS: all four native programs completed successfully.\n",proof);fclose(proof);return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(32768UL,run_qa,argc,argv);}
