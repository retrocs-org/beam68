#include <exec/types.h>
#include <exec/io.h>
#include <devices/clipboard.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "clip.h"

static void put32(UBYTE *p,ULONG v)
{ p[0]=(UBYTE)(v>>24);p[1]=(UBYTE)(v>>16);p[2]=(UBYTE)(v>>8);p[3]=(UBYTE)v; }
static ULONG get32(const UBYTE *p)
{ return ((ULONG)p[0]<<24)|((ULONG)p[1]<<16)|((ULONG)p[2]<<8)|(ULONG)p[3]; }

static int timed_io(struct IOClipReq *io)
{
    int ticks=0;
    SendIO((struct IORequest *)io);
    while (!CheckIO((struct IORequest *)io) && ticks++<100) Delay(1);
    if (!CheckIO((struct IORequest *)io)) AbortIO((struct IORequest *)io);
    WaitIO((struct IORequest *)io);
    return ticks<=100 && io->io_Error==0;
}

static struct IOClipReq *clip_open(struct MsgPort **port)
{
    struct IOClipReq *io;
    *port=CreateMsgPort();
    if (!*port) return NULL;
    io=(struct IOClipReq *)CreateIORequest(*port,sizeof(*io));
    if (io && OpenDevice((STRPTR)"clipboard.device",0,(struct IORequest *)io,0)==0) return io;
    if (io) DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(*port);
    return NULL;
}

static void clip_close(struct IOClipReq *io,struct MsgPort *port)
{
    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
}

int beam_clip_write(const char *text)
{
    static UBYTE data[300];
    struct MsgPort *port;
    struct IOClipReq *io;
    ULONG len=(ULONG)strlen(text),total=20+len+(len&1);
    int ok;
    if (total>sizeof(data) || !(io=clip_open(&port))) return 0;
    memcpy(data,"FORM",4); put32(data+4,total-8); memcpy(data+8,"FTXTCHRS",8);
    put32(data+16,len); memcpy(data+20,text,len); if (len&1) data[20+len]=0;
    io->io_Command=CMD_WRITE; io->io_Data=(STRPTR)data; io->io_Length=total;
    io->io_Offset=0; io->io_ClipID=0;
    ok=timed_io(io);
    io->io_Command=CMD_UPDATE; io->io_Data=NULL; io->io_Length=0;
    if (!timed_io(io)) ok=0;
    clip_close(io,port);
    return ok;
}

int beam_clip_read(char *out,unsigned int cap)
{
    static UBYTE buf[256];
    struct MsgPort *port;
    struct IOClipReq *io;
    UBYTE head[12],chunk[8];
    ULONG remaining,len,n;
    int ok=0;
    if (!cap) return 0;
    out[0]=0;
    if (!(io=clip_open(&port))) return 0;
    io->io_Command=CMD_READ; io->io_Data=(STRPTR)head; io->io_Length=12;
    io->io_Offset=0; io->io_ClipID=0;
    if (timed_io(io) && io->io_Actual==12 && !memcmp(head,"FORM",4) && !memcmp(head+8,"FTXT",4)) {
        remaining=get32(head+4)>=4?get32(head+4)-4:0;
        while (!ok && remaining>=8) {
            io->io_Command=CMD_READ; io->io_Data=(STRPTR)chunk; io->io_Length=8;
            if (!timed_io(io) || io->io_Actual!=8) break;
            len=get32(chunk+4); remaining-=8;
            if (!memcmp(chunk,"CHRS",4)) {
                n=len<cap-1?len:cap-1;
                if (n>sizeof(buf)) n=sizeof(buf);
                io->io_Command=CMD_READ; io->io_Data=(STRPTR)buf; io->io_Length=n;
                if (n && timed_io(io) && io->io_Actual==n) {
                    memcpy(out,buf,n); out[n]=0; ok=1;
                }
            }
            if (!ok) {
                /* skip this chunk's data (and pad byte) */
                ULONG skip=len+(len&1);
                while (skip) {
                    n=skip>sizeof(buf)?sizeof(buf):skip;
                    io->io_Command=CMD_READ; io->io_Data=(STRPTR)buf; io->io_Length=n;
                    if (!timed_io(io) || io->io_Actual==0) { skip=0; remaining=0; break; }
                    skip-=io->io_Actual;
                }
                remaining=remaining>=len+(len&1)?remaining-len-(len&1):0;
            }
        }
    }
    /* Read past the end so clipboard.device releases the clip. */
    do {
        io->io_Command=CMD_READ; io->io_Data=(STRPTR)buf; io->io_Length=sizeof(buf);
        if (!timed_io(io)) break;
    } while (io->io_Actual>0);
    clip_close(io,port);
    return ok;
}
