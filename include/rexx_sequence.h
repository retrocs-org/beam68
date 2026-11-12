#ifndef BEAM68_REXX_SEQUENCE_H
#define BEAM68_REXX_SEQUENCE_H
#define REXX_MODULE 1
#define REXX_DOPUS_NEW 2
#define REXX_DOPUS_WAIT 3
#define REXX_DOPUS_SELECT 4
#define REXX_DOPUS_REFRESH 5
#define REXX_DOPUS_SCAN 6
/* Returns next stage, zero when complete, -1 for unsafe/oversized input. */
int beam_rexx_next(int stage,int ok,const char *handle,const char *drawer,
    const char *select,char *command,unsigned int cap);
int beam_rexx_handle_valid(const char *handle);
#endif
