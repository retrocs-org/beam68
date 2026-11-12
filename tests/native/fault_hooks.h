/* Included after the NDK in test-only compilation, so its inline macros
 * cannot bypass the injected failure functions. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#undef FPuts
#undef Rename
LONG qa_FPuts(BPTR file,CONST_STRPTR text);
LONG qa_Rename(CONST_STRPTR from,CONST_STRPTR to);
#define FPuts qa_FPuts
#define Rename qa_Rename
