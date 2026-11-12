#include <exec/types.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <devices/clipboard.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

static void put32(UBYTE *p,ULONG v)
{ p[0]=(UBYTE)(v>>24);p[1]=(UBYTE)(v>>16);p[2]=(UBYTE)(v>>8);p[3]=(UBYTE)v; }
static ULONG get32(const UBYTE *p)
{ return ((ULONG)p[0]<<24)|((ULONG)p[1]<<16)|((ULONG)p[2]<<8)|(ULONG)p[3]; }

/* clipboard.device may wait for a writer. Never let a launcher utility hang
 * indefinitely: abort a request after roughly two seconds. */
static int timed_io(struct IOClipReq *io)
{
    int ticks=0;
    SendIO((struct IORequest *)io);
    while (!CheckIO((struct IORequest *)io) && ticks++<100) Delay(1);
    if (!CheckIO((struct IORequest *)io)) AbortIO((struct IORequest *)io);
    WaitIO((struct IORequest *)io);
    return ticks<=100 && io->io_Error==0;
}

static int clipboard_write(const char *text)
{
    struct MsgPort *port;
    struct IOClipReq *io;
    UBYTE data[160];
    ULONG len=(ULONG)strlen(text),total=20+len+(len&1);
    int ok=0;
    if (total>sizeof(data)) return 0;
    memcpy(data,"FORM",4);put32(data+4,total-8);memcpy(data+8,"FTXTCHRS",8);
    put32(data+16,len);memcpy(data+20,text,len);if (len&1) data[20+len]=0;
    port=CreateMsgPort();
    if (!port) return 0;
    io=(struct IOClipReq *)CreateIORequest(port,sizeof(*io));
    if (io && OpenDevice("clipboard.device",0,(struct IORequest *)io,0)==0) {
        io->io_Command=CMD_WRITE;io->io_Data=data;io->io_Length=total;io->io_Offset=0;io->io_ClipID=0;
        ok=timed_io(io);
        io->io_Command=CMD_UPDATE;io->io_Data=NULL;io->io_Length=0;
        if (!timed_io(io)) ok=0;
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    return ok;
}

static int clipboard_read(char *out,unsigned int cap)
{
    struct MsgPort *port;
    struct IOClipReq *io;
    UBYTE header[12],chunk[8],data[128];
    ULONG form_size,len,n,remaining,got;
    int ok=0;
    if (!cap) return 0;
    out[0]=0;
    port=CreateMsgPort();
    if (!port) return 0;
    io=(struct IOClipReq *)CreateIORequest(port,sizeof(*io));
    if (io && OpenDevice("clipboard.device",0,(struct IORequest *)io,0)==0) {
        io->io_Command=CMD_READ;io->io_Data=header;io->io_Length=12;io->io_Offset=0;io->io_ClipID=0;
        if (timed_io(io) && io->io_Actual==12 && !memcmp(header,"FORM",4) && !memcmp(header+8,"FTXT",4)) {
            form_size=get32(header+4);remaining=form_size>=4?form_size-4:0;
            while (remaining>=8) {
                io->io_Command=CMD_READ;io->io_Data=chunk;io->io_Length=8;
                if (!timed_io(io) || io->io_Actual!=8) break;
                len=get32(chunk+4);remaining-=8;got=0;
                if (!memcmp(chunk,"CHRS",4)) {
                    n=len;if (n+1>cap) n=cap-1;if (n>sizeof(data)) n=sizeof(data);
                    io->io_Command=CMD_READ;io->io_Data=data;io->io_Length=n;
                    if (!n || (timed_io(io) && io->io_Actual==n)) {
                        memcpy(out,data,n);out[n]=0;ok=1;got=n;
                    }
                }
                while (got<len) {
                    n=len-got;if (n>sizeof(data)) n=sizeof(data);
                    io->io_Command=CMD_READ;io->io_Data=data;io->io_Length=n;
                    if (!timed_io(io) || io->io_Actual!=n) { got=len;break; }
                    got+=n;
                }
                if (len&1) { io->io_Command=CMD_READ;io->io_Data=data;io->io_Length=1;timed_io(io); }
                if (remaining<len+(len&1)) break;
                remaining-=len+(len&1);if (ok) break;
            }
        }
        /* clipboard.device considers a read finished only once the reader
           reads past the end (io_Actual==0).  Stopping early -- or sending
           CMD_UPDATE, which terminates a WRITE -- leaves the unit busy for
           the next writer. */
        do {
            io->io_Command=CMD_READ;io->io_Data=data;io->io_Length=sizeof(data);
            if (!timed_io(io)) break;
        } while (io->io_Actual>0);
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    return ok;
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],text[121];
    char *s,*path;
    BPTR file;
    unsigned int i;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (!*s || beam_ascii_equal(s,"show")) {
        if (!clipboard_read(text,sizeof(text))) return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_EMPTY));
        return beam_plugin_text(argc,argv,text);
    }
    if (beam_ascii_equal(s,"clear"))
        return clipboard_write("")?beam_plugin_text(argc,argv,plugin_str(PLMSG_CLIP_CLEARED)):beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_CLEAR_FAILED));
    if (beam_ascii_prefix(s,"set:")) {
        path=s+4;
        return clipboard_write(path)?beam_plugin_text(argc,argv,plugin_str(PLMSG_CLIP_SET)):beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_UPDATE_FAILED));
    }
    if (beam_ascii_equal(s,"upper") || beam_ascii_equal(s,"lower")) {
        int upper=beam_ascii_equal(s,"upper");
        if (!clipboard_read(text,sizeof(text))) return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_EMPTY));
        for (i=0;text[i];i++) {
            if (upper && text[i]>='a'&&text[i]<='z') text[i]-=32;
            if (!upper && text[i]>='A'&&text[i]<='Z') text[i]+=32;
        }
        return clipboard_write(text)?beam_plugin_text(argc,argv,text):beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_UPDATE_FAILED));
    }
    if (beam_ascii_prefix(s,"save:")) {
        path=beam_plugin_trim(s+5);
        if (!*path || !strchr(path,':') || strchr(path,'\n') || strchr(path,'\r')) return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_SAVE_USAGE));
        if (!clipboard_read(text,sizeof(text))) return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_EMPTY));
        file=Open(path,MODE_NEWFILE);if (!file) return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_CREATE_FAILED));
        i=(unsigned int)strlen(text);if (FWrite(file,text,1,i)!=(LONG)i) { Close(file);return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_WRITE_FAILED)); }
        Close(file);return beam_plugin_text(argc,argv,plugin_str(PLMSG_CLIP_SAVED));
    }
    return beam_plugin_error(argc,argv,plugin_str(PLMSG_CLIP_USAGE));
}
