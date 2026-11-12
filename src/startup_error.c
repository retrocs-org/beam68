/* Visible failures from both Shell and Workbench startup. */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include "startup_error.h"
extern struct IntuitionBase *IntuitionBase;
void beam_startup_error(const char *app,const char *reason)
{
    if (Output()) {
        PutStr((STRPTR)app); PutStr((STRPTR)": ");
        PutStr((STRPTR)reason); PutStr((STRPTR)"\n");
    } else if (IntuitionBase) {
        struct EasyStruct es;
        ULONG args[1];
        char wrapped[384];
        unsigned int i=0,column=0,last_space=0;
        while (reason[i] && i+1<sizeof(wrapped)) {
            wrapped[i]=reason[i];
            if (wrapped[i]==' ') last_space=i;
            if (++column>=48 && last_space) {
                wrapped[last_space]='\n'; column=i-last_space;last_space=0;
            }
            ++i;
        }
        wrapped[i]=0;
        es.es_StructSize=sizeof(es); es.es_Flags=0;
        es.es_Title=(STRPTR)app; es.es_TextFormat=(STRPTR)"%s";
        es.es_GadgetFormat=(STRPTR)"OK"; args[0]=(ULONG)wrapped;
        EasyRequestArgs(NULL,&es,NULL,args);
    }
}
