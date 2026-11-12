#ifndef BEAM_FAKE_PROTO_EXEC_H
#define BEAM_FAKE_PROTO_EXEC_H
#include <exec/types.h>
#include <exec/ports.h>
#define NT_MESSAGE 5
void *AllocVec(unsigned long size,unsigned long flags);
void FreeVec(void *memory);
void Forbid(void);
void Permit(void);
struct MsgPort *FindPort(STRPTR name);
void PutMsg(struct MsgPort *port,struct Message *message);
#endif
