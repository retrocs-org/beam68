#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "whdload.h"

static unsigned char d[512];
static void p32(unsigned long at,unsigned long v)
{ d[at]=(unsigned char)(v>>24); d[at+1]=(unsigned char)(v>>16); d[at+2]=(unsigned char)(v>>8); d[at+3]=(unsigned char)v; }
static void p16(unsigned long at,unsigned int v) { d[at]=(unsigned char)(v>>8); d[at+1]=(unsigned char)v; }

/* A HUNK file whose code hunk starts with a version-17 slave structure. */
static unsigned long make(unsigned int version,int kick)
{
    unsigned long b=36;
    memset(d,0,sizeof(d));
    p32(0,0x3F3); p32(4,0); p32(8,1); p32(12,0); p32(16,0); p32(20,100);
    p32(24,0x3E9); p32(28,100); /* header ends at 32: code data at 32 */
    b=32;
    p32(b,0x70FF4E75); memcpy(d+b+4,"WHDLOADS",8); p16(b+12,version);
    p16(b+36,60); p16(b+38,90); p16(b+40,120);
    strcpy((char *)d+b+60,"Turrican II\nThe Final Fight");
    strcpy((char *)d+b+90,"1991 Factor 5");
    strcpy((char *)d+b+120,"installed by Mr.Larmer");
    if (kick) { p16(b+42,150); p32(b+44,262144); p16(b+48,0x1234);
                strcpy((char *)d+b+150,"kick34005.A500"); }
    return b+200;
}

int main(void)
{
    struct BeamSlaveInfo s;
    char title[128];
    unsigned long n=make(17,1);
    assert(beam_slave_info(d,n,&s) && s.version==17);
    assert(!strcmp(s.name,"Turrican II The Final Fight"));
    assert(!strcmp(s.copyright,"1991 Factor 5"));
    assert(!strcmp(s.kick,"kick34005.A500"));
    beam_slave_title(&s,title,sizeof(title));
    assert(!strcmp(title,"Turrican II The Final Fight (1991 Factor 5)"));
    n=make(17,0);
    assert(beam_slave_info(d,n,&s) && !s.kick[0]);
    n=make(8,1);                     /* too old for texts */
    assert(beam_slave_info(d,n,&s) && !s.name[0] && !s.kick[0]);
    beam_slave_title(&s,title,sizeof(title)); assert(!title[0]);
    n=make(17,1); memcpy(d+36,"WHDLOADX",8);
    assert(!beam_slave_info(d,n,&s));
    n=make(17,1); d[3]=0;            /* not a HUNK executable */
    assert(!beam_slave_info(d,n,&s));
    n=make(17,1);
    assert(!beam_slave_info(d,40,&s)); /* truncated */
    puts("WHDLoad slaves: header, name, copyright, Kickstart: PASS");
    return 0;
}
