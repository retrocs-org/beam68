#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include "stack_guard.h"

/* Everything used between the two StackSwap() calls is static: once the
 * stack pointer moves, a compiler-generated stack slot of this function
 * would refer to the wrong stack. */
static struct StackSwapStruct guard_swap;
static APTR guard_memory;
static int (*guard_entry)(int, char **);
static int guard_argc;
static char **guard_argv;
static int guard_result;

static unsigned long current_stack(void)
{
    struct Process *me=(struct Process *)FindTask(NULL);
    unsigned long size=(unsigned long)me->pr_Task.tc_SPUpper-
                       (unsigned long)me->pr_Task.tc_SPLower;
    if (me->pr_Task.tc_Node.ln_Type==NT_PROCESS && me->pr_CLI) {
        struct CommandLineInterface *cli=
            (struct CommandLineInterface *)BADDR(me->pr_CLI);
        /* A Shell command runs on cli_DefaultStack longwords. */
        unsigned long shell=(unsigned long)cli->cli_DefaultStack*4UL;
        if (shell && shell<size) size=shell;
    }
    return size;
}

int beam_run_with_stack(unsigned long minimum,
                        int (*entry)(int, char **), int argc, char **argv)
{
    if (current_stack()>=minimum) return entry(argc,argv);
    guard_memory=AllocVec(minimum,MEMF_ANY);
    if (!guard_memory) return entry(argc,argv);
    guard_entry=entry; guard_argc=argc; guard_argv=argv;
    guard_swap.stk_Lower=guard_memory;
    guard_swap.stk_Upper=(ULONG)guard_memory+minimum;
    guard_swap.stk_Pointer=(APTR)guard_swap.stk_Upper;
    StackSwap(&guard_swap);
    guard_result=guard_entry(guard_argc,guard_argv);
    StackSwap(&guard_swap);
    FreeVec(guard_memory);
    guard_memory=NULL;
    return guard_result;
}
