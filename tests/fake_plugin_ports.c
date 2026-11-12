#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <proto/exec.h>
#include "beam68plugin.h"
static int available=1,allocate=1,forbidden;
static struct Message *received;
static char port_token;
void *AllocVec(unsigned long size,unsigned long flags)
{ (void)flags;return allocate?calloc(1,size):NULL; }
void FreeVec(void *memory) { free(memory); }
void Forbid(void) { assert(!forbidden);forbidden=1; }
void Permit(void) { assert(forbidden);forbidden=0; }
struct MsgPort *FindPort(STRPTR name)
{ assert(forbidden);return available && !strcmp(name,BEAM68_REPLY_PORT)?(struct MsgPort *)&port_token:NULL; }
void PutMsg(struct MsgPort *port,struct Message *message)
{ assert(forbidden && port && !received);received=message; }
struct Beam68ReplyMessage *test_take_reply(void)
{ struct Message *m=received;received=NULL;assert(!forbidden);return (struct Beam68ReplyMessage *)m; }
void test_delivery(int port,int memory) { available=port;allocate=memory; }
