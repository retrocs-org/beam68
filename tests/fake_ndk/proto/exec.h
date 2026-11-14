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
struct MsgPort *CreateMsgPort(void);
void DeleteMsgPort(struct MsgPort *port);
void AddPort(struct MsgPort *port);
void RemPort(struct MsgPort *port);
struct Message *GetMsg(struct MsgPort *port);
void ReplyMsg(struct Message *message);
void *FindTask(STRPTR name);
#endif
