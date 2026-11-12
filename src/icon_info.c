#include <string.h>
#include "icon_info.h"

static unsigned long be32(const unsigned char *p)
{ return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3]; }
static unsigned int be16(const unsigned char *p)
{ return ((unsigned int)p[0]<<8)|p[1]; }

/* Skip one struct Image (20 bytes) and its plane data. */
static int skip_image(const unsigned char *d,unsigned long len,unsigned long *pos)
{
    unsigned long width,height,depth,size;
    if (*pos+20>len) return 0;
    width=be16(d+*pos+4); height=be16(d+*pos+6); depth=be16(d+*pos+8);
    size=be32(d+*pos+10)?((width+15UL)/16UL)*2UL*height*depth:0UL;
    if (width>1024 || height>1024 || depth>8) return 0;
    *pos+=20+size;
    return *pos<=len;
}

/* Offset just after the images, or 0. */
static unsigned long after_images(const unsigned char *d,unsigned long len)
{
    unsigned long pos=BEAM_ICON_HEADER_BYTES;
    if (be32(d+66)) pos+=56;
    if (be32(d+22) && !skip_image(d,len,&pos)) return 0;
    if (be32(d+26) && !skip_image(d,len,&pos)) return 0;
    return pos;
}

static int lower_c(int c) { return (c>='A' && c<='Z')?c+32:c; }

int beam_icon_tooltype(const unsigned char *d,unsigned long len,const char *key,
                       char *value,unsigned int cap)
{
    unsigned long pos,count,i,n,k;
    if (value && cap) value[0]=0;
    if (!d || len<BEAM_ICON_HEADER_BYTES || be16(d)!=0xE310U || !key || !value || !cap) return 0;
    pos=after_images(d,len);
    if (!pos) return 0;
    if (be32(d+50)) {                              /* skip the Default Tool */
        if (pos+4>len) return 0;
        n=be32(d+pos); pos+=4+n;
    }
    if (!be32(d+54) || pos+4>len) return 0;        /* no tool types */
    count=be32(d+pos)/4; pos+=4;
    if (count) --count;
    k=strlen(key);
    for (i=0;i<count && pos+4<=len;i++) {
        n=be32(d+pos); pos+=4;
        if (!n || pos+n>len) return 0;
        if (n>k+1 && d[pos+k]=='=') {
            unsigned long j;
            for (j=0;j<k && lower_c(d[pos+j])==lower_c((unsigned char)key[j]);j++) { }
            if (j==k) {
                unsigned long m=n-k-2;               /* without '=' and NUL */
                if (m+1>cap) m=cap-1;
                memcpy(value,d+pos+k+1,m); value[m]=0;
                return 1;
            }
        }
        pos+=n;
    }
    return 0;
}

int beam_icon_info(const unsigned char *d,unsigned long len,char *tool,unsigned int cap)
{
    unsigned long pos=BEAM_ICON_HEADER_BYTES,n;
    int type;
    if (tool && cap) tool[0]=0;
    if (!d || len<BEAM_ICON_HEADER_BYTES || be16(d)!=0xE310U) return 0;
    type=d[48];
    if (type<1 || type>8) return 0;
    if (be32(d+66)) pos+=56;                     /* do_DrawerData */
    if (be32(d+22) && !skip_image(d,len,&pos)) return type; /* GadgetRender */
    if (be32(d+26) && !skip_image(d,len,&pos)) return type; /* SelectRender */
    if (!be32(d+50) || pos+4>len) return type;   /* no do_DefaultTool */
    n=be32(d+pos); pos+=4;
    if (!n || n>255 || pos+n>len || !tool || !cap) return type;
    if (n>cap) n=cap;
    memcpy(tool,d+pos,n);
    tool[n-1]=0;
    return type;
}

static int lower(int c) { return (c>='A' && c<='Z')?c+32:c; }

int beam_icon_is_whdload_tool(const char *tool)
{
    const char *name=tool,*p;
    static const char want[]="whdload";
    unsigned int i;
    if (!tool) return 0;
    for (p=tool;*p;p++) if (*p=='/' || *p==':') name=p+1;
    for (i=0;want[i];i++) if (lower((unsigned char)name[i])!=want[i]) return 0;
    return name[i]==0;
}
