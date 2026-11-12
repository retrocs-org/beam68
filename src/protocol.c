#include "plugin_protocol.h"

static int from_hex(char c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    return -1;
}

int beam_decode_hex(const char *src, char *dst, unsigned int size)
{
    unsigned int i=0;
    int a, b;
    if (!src || !dst || size==0) return 0;
    if (src[0]=='-' && !src[1]) { dst[0]=0; return 1; }
    while (*src) {
        a=from_hex(*src++);
        if (a<0 || !*src) return 0;
        b=from_hex(*src++);
        if (b<0 || !((a<<4)|b)) return 0;
        if (i+1 >= size) return 0;
        dst[i++]=(char)((a<<4)|b);
    }
    dst[i]=0;
    return 1;
}
