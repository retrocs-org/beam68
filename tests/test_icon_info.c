#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "icon_info.h"

static unsigned char buf[4096];
static unsigned long put32(unsigned long at,unsigned long v)
{ buf[at]=(unsigned char)(v>>24); buf[at+1]=(unsigned char)(v>>16);
  buf[at+2]=(unsigned char)(v>>8); buf[at+3]=(unsigned char)v; return at+4; }
static void put16(unsigned long at,unsigned int v)
{ buf[at]=(unsigned char)(v>>8); buf[at+1]=(unsigned char)v; }

/* A project icon with two 20x10x2 images and a Default Tool. */
static unsigned long make(const char *tool,int type,int selected)
{
    unsigned long pos=78,n;
    memset(buf,0,sizeof(buf));
    put16(0,0xE310); put16(2,1);
    put32(22,1); if (selected) put32(26,1);
    buf[48]=(unsigned char)type;
    if (tool) put32(50,1);
    put16(pos+4,20); put16(pos+6,10); put16(pos+8,2); put32(pos+10,1);
    pos+=20+((20+15)/16)*2*10*2;
    if (selected) { put16(pos+4,20); put16(pos+6,10); put16(pos+8,2); put32(pos+10,1);
                    pos+=20+((20+15)/16)*2*10*2; }
    if (tool) { n=strlen(tool)+1; pos=put32(pos,n); memcpy(buf+pos,tool,n); pos+=n; }
    return pos;
}

int main(void)
{
    char tool[64];
    FILE *f;
    unsigned long n;
    n=make("WHDLoad",4,1);
    assert(beam_icon_info(buf,n,tool,sizeof(tool))==4 && !strcmp(tool,"WHDLoad"));
    assert(beam_icon_is_whdload_tool(tool));
    n=make("C:WHDLoad",4,0);
    assert(beam_icon_info(buf,n,tool,sizeof(tool))==4 && beam_icon_is_whdload_tool(tool));
    n=make("SYS:Utilities/MultiView",4,1);
    assert(beam_icon_info(buf,n,tool,sizeof(tool))==4 && !beam_icon_is_whdload_tool(tool));
    assert(!beam_icon_is_whdload_tool("WHDLoadX") && !beam_icon_is_whdload_tool("NoWHDLoad"));
    n=make(NULL,3,1);
    assert(beam_icon_info(buf,n,tool,sizeof(tool))==3 && !tool[0]);
    /* Truncated data: the type is still known, the tool is not. */
    n=make("WHDLoad",4,1);
    assert(beam_icon_info(buf,n-4,tool,sizeof(tool))==4 && !tool[0]);
    buf[0]=0; assert(beam_icon_info(buf,n,tool,sizeof(tool))==0);
    /* The shipped installer icon is a real project icon with a tool. */
    f=fopen("Install.info","rb");
    assert(f); n=(unsigned long)fread(buf,1,sizeof(buf),f); fclose(f);
    assert(beam_icon_info(buf,n,tool,sizeof(tool))==4 && !strcmp(tool,"Installer"));
    assert(beam_icon_tooltype(buf,n,"script",tool,sizeof(tool)) && !strcmp(tool,"Install"));
    assert(beam_icon_tooltype(buf,n,"APPNAME",tool,sizeof(tool)) && !strcmp(tool,"Beam68"));
    assert(!beam_icon_tooltype(buf,n,"SLAVE",tool,sizeof(tool)) && !tool[0]);
    assert(!beam_icon_tooltype(buf,n-3,"DEFUSER",tool,sizeof(tool)));
    puts("Icon reader: types, images, Default Tool, WHDLoad detection: PASS");
    return 0;
}
