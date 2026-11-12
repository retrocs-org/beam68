#include <string.h>
#include "whdload.h"

static unsigned long be32(const unsigned char *p)
{ return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3]; }
static unsigned int be16(const unsigned char *p) { return ((unsigned int)p[0]<<8)|p[1]; }

/* Offset of the first hunk's contents in a HUNK executable, or 0. */
static unsigned long first_hunk_data(const unsigned char *d,unsigned long len)
{
    unsigned long pos=4,n,first,last,type;
    if (len<32 || be32(d)!=0x3F3UL) return 0;
    for (;;) {                                   /* resident library names */
        if (pos+4>len) return 0;
        n=be32(d+pos); pos+=4;
        if (!n) break;
        if (n>64) return 0;
        pos+=n*4;
    }
    if (pos+12>len) return 0;
    first=be32(d+pos+4); last=be32(d+pos+8); pos+=12;
    if (last<first || last-first>255) return 0;
    pos+=(last-first+1)*4;                       /* hunk sizes */
    if (pos+8>len) return 0;
    type=be32(d+pos)&0x3FFFFFFFUL;
    if (type!=0x3E9UL && type!=0x3EAUL) return 0; /* HUNK_CODE / HUNK_DATA */
    return pos+8;
}

static void copy_text(const unsigned char *d,unsigned long len,unsigned long at,
                      char *out,unsigned int cap)
{
    unsigned int n=0;
    out[0]=0;
    if (!at || at>=len) return;
    while (at<len && d[at] && n+1<cap) {
        unsigned char c=d[at++];
        /* Slave texts use 10 for line breaks and -1 as a separator. */
        if (c<32 || c==127 || c==0xFF) c=' ';
        if (c==' ' && (!n || out[n-1]==' ')) continue;
        out[n++]=(char)c;
    }
    while (n && out[n-1]==' ') --n;
    out[n]=0;
}

int beam_slave_info(const unsigned char *d,unsigned long len,struct BeamSlaveInfo *info)
{
    unsigned long base=first_hunk_data(d,len),s;
    memset(info,0,sizeof(*info));
    if (!base || base+44>len || memcmp(d+base+4,"WHDLOADS",8)) return 0;
    s=len-base;
    info->version=be16(d+base+12);
    if (info->version>=10) {
        copy_text(d+base,s,be16(d+base+36),info->name,sizeof(info->name));
        copy_text(d+base,s,be16(d+base+38),info->copyright,sizeof(info->copyright));
    }
    if (info->version>=16 && base+50<=len && be32(d+base+44) && be16(d+base+48)!=0xFFFFU)
        copy_text(d+base,s,be16(d+base+42),info->kick,sizeof(info->kick));
    return 1;
}

void beam_slave_title(const struct BeamSlaveInfo *info,char *out,unsigned int cap)
{
    unsigned int n;
    out[0]=0;
    if (!info->name[0] || cap<2) return;
    strncpy(out,info->name,cap-1); out[cap-1]=0;
    n=(unsigned int)strlen(out);
    if (info->copyright[0] && n+strlen(info->copyright)+4<cap) {
        strcat(out," (");
        strcat(out,info->copyright);
        strcat(out,")");
    }
}
