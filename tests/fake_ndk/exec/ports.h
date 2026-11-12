#ifndef BEAM_FAKE_EXEC_PORTS_H
#define BEAM_FAKE_EXEC_PORTS_H
/* Host-test stand-in for the NDK's struct Message. */
struct Node { struct Node *ln_Succ, *ln_Pred; unsigned char ln_Type; signed char ln_Pri; char *ln_Name; };
struct Message { struct Node mn_Node; void *mn_ReplyPort; unsigned short mn_Length; };
#endif
