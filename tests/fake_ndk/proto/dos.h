#ifndef BEAM_FAKE_PROTO_DOS_H
#define BEAM_FAKE_PROTO_DOS_H
#include <dos/dos.h>
BPTR Open(STRPTR name,LONG mode);
char *FGets(BPTR file,char *line,LONG len);
LONG FRead(BPTR file,void *buf,LONG size,LONG count);
LONG FPuts(BPTR file,STRPTR str);
LONG Seek(BPTR file,LONG offset,LONG mode);
LONG Close(BPTR file);
BPTR CreateDir(STRPTR name);
BPTR Lock(STRPTR name,LONG mode);
void UnLock(BPTR lock);
LONG Rename(STRPTR oldname,STRPTR newname);
LONG DeleteFile(STRPTR name);
void DateStamp(struct DateStamp *stamp);
void Delay(unsigned long ticks);
void *AllocDosObject(ULONG type,void *tags);
void FreeDosObject(ULONG type,void *object);
LONG Examine(BPTR lock,struct FileInfoBlock *fib);
LONG ExNext(BPTR lock,struct FileInfoBlock *fib);
LONG AddPart(STRPTR path,STRPTR part,LONG size);
LONG IoErr(void);
#endif
