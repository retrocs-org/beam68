#include <string.h>
#include "hotkey_capture.h"

static const char *key_name(unsigned raw_code)
{
    static const char *function_keys[] = {
        "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10"
    };
    switch (raw_code) {
    case 0x40: return "space";
    case 0x41: return "backspace";
    case 0x42: return "tab";
    case 0x43: return "enter";
    case 0x44: return "return";
    case 0x45: return "esc";
    case 0x46: return "del";
    case 0x4c: return "up";
    case 0x4d: return "down";
    case 0x4e: return "right";
    case 0x4f: return "left";
    case 0x5f: return "help";
    default:
        if (raw_code>=0x50 && raw_code<=0x59)
            return function_keys[raw_code-0x50];
        return NULL;
    }
}

int beam_hotkey_is_modifier(unsigned raw_code)
{
    return raw_code>=0x60 && raw_code<=0x67;
}

static int append(char *out, unsigned capacity, const char *text)
{
    unsigned used=(unsigned)strlen(out), length=(unsigned)strlen(text);
    if (used+length+1>capacity) return 0;
    memcpy(out+used,text,length+1);
    return 1;
}

int beam_hotkey_format(char *out, unsigned capacity, unsigned raw_code,
                       unsigned modifiers, int mapped_char)
{
    const char *name=key_name(raw_code);
    char ansi[2];
    if (!out || !capacity || raw_code>=0x80 || beam_hotkey_is_modifier(raw_code))
        return 0;
    if (!name) {
        if (mapped_char>='A' && mapped_char<='Z') mapped_char+='a'-'A';
        if (mapped_char<33 || mapped_char>126) return 0;
        ansi[0]=(char)mapped_char; ansi[1]=0; name=ansi;
    }
    out[0]=0;
    if (!append(out,capacity,"rawkey ")) return 0;
    if ((modifiers&BEAM_HOTKEY_SHIFT) && !append(out,capacity,"shift ")) return 0;
    if ((modifiers&BEAM_HOTKEY_CONTROL) && !append(out,capacity,"control ")) return 0;
    if ((modifiers&BEAM_HOTKEY_ALT) && !append(out,capacity,"alt ")) return 0;
    if ((modifiers&BEAM_HOTKEY_LCOMMAND) && !append(out,capacity,"lcommand ")) return 0;
    if ((modifiers&BEAM_HOTKEY_RCOMMAND) && !append(out,capacity,"rcommand ")) return 0;
    if (!append(out,capacity,name)) return 0;
    return 1;
}

static int token_equal(const char *a,unsigned n,const char *b)
{
    unsigned i;
    for (i=0;i<n && b[i];i++) {
        int x=(unsigned char)a[i],y=(unsigned char)b[i];
        if (x>='A'&&x<='Z') x+=32;
        if (y>='A'&&y<='Z') y+=32;
        if (x!=y) return 0;
    }
    return i==n && !b[i];
}
int beam_hotkey_has_duplicates(const char *descriptor)
{
    const char *p=descriptor,*start;
    unsigned n,seen=0;
    int keys=0,first=1,bit;
    if (!p) return 1;
    while (*p) {
        while (*p==' '||*p=='\t') p++;
        if (!*p) break;
        start=p;while (*p && *p!=' ' && *p!='\t') p++;
        n=(unsigned)(p-start);
        if (first) {
            if (!token_equal(start,n,"rawkey")) return 1;
            first=0;continue;
        }
        bit=token_equal(start,n,"shift")?1:token_equal(start,n,"control")?2:
            token_equal(start,n,"alt")?4:token_equal(start,n,"lcommand")?8:
            token_equal(start,n,"rcommand")?16:0;
        if (bit) {
            if (seen&(unsigned)bit) return 1;
            seen|=(unsigned)bit;
        } else if (++keys>1) return 1;
    }
    return first || keys!=1;
}
