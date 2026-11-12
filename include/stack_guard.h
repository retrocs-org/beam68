#ifndef BEAM68_STACK_GUARD_H
#define BEAM68_STACK_GUARD_H
/* Run entry(argc,argv) on a stack of at least `minimum` bytes.
 *
 * Workbench honours the icon's stack size, but a program started from a
 * Shell, with Run, or by another program through SystemTagList() gets the
 * Shell default (typically 4 KB).  A 68000 has no MMU, so an overflow
 * silently corrupts memory.  This swaps to a private stack when needed. */
int beam_run_with_stack(unsigned long minimum,
                        int (*entry)(int, char **), int argc, char **argv);
#endif
