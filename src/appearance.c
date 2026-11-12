#include "appearance.h"
#include "beam.h"
#include <stdio.h>

/* Catppuccin-inspired values. Components are repeated across 32 bits as
 * required by graphics.library's ObtainBestPenA(). Workbench uses DrawInfo
 * pens and therefore has no fixed RGB definition here. */
struct ThemeRGB { unsigned char r,g,b; };
static const struct ThemeRGB theme_colors[4][BEAM_THEME_ROLE_COUNT]={
    {{76,79,105},{156,160,176},{114,135,253},{239,241,245},{239,241,245}},
    {{198,208,245},{98,104,128},{140,170,238},{48,52,70},{48,52,70}},
    {{202,211,245},{91,96,120},{138,173,244},{36,39,58},{36,39,58}},
    {{205,214,244},{88,91,112},{137,180,250},{30,30,46},{30,30,46}}
};

static unsigned long spread(unsigned char value)
{
    unsigned long v=value;
    return (v<<24)|(v<<16)|(v<<8)|v;
}

static void copy_bounded(char *dest,const char *src,unsigned int capacity)
{
    unsigned int i=0;
    if (!dest || !capacity) return;
    while (src[i] && i+1<capacity) { dest[i]=src[i]; ++i; }
    dest[i]=0;
}

static int theme_name_equal(const char *a,const char *b)
{
    int ca,cb;
    while (*a && *b) {
        ca=(unsigned char)*a++; cb=(unsigned char)*b++;
        if (ca>='A' && ca<='Z') ca+=32;
        if (cb>='A' && cb<='Z') cb+=32;
        if (ca!=cb) return 0;
    }
    return !*a && !*b;
}

const char *beam_theme_name(int theme)
{
    static const char *names[BEAM_THEME_COUNT]={
        "Workbench","Latte","Frappe","Macchiato","Mocha","Custom"
    };
    return (theme>=0 && theme<BEAM_THEME_COUNT)?names[theme]:names[0];
}

const char *beam_theme_description(int theme)
{
    static const char *descriptions[BEAM_THEME_COUNT]={
        "Workbench system colors",
        "Light - lavender-blue accent",
        "Muted dark - soft blue accent",
        "Balanced dark - clear blue accent",
        "Deep dark - bright blue accent",
        "Editable text, shadow, accent, accent text and background"
    };
    return (theme>=0 && theme<BEAM_THEME_COUNT)?descriptions[theme]:descriptions[0];
}

int beam_theme_from_name(const char *name)
{
    int i;
    if (!name) return BEAM_THEME_WORKBENCH;
    for (i=0;i<BEAM_THEME_COUNT;i++) if (theme_name_equal(name,beam_theme_name(i)))
        return i;
    return BEAM_THEME_WORKBENCH;
}

int beam_theme_color(int theme, int role, unsigned long *red,
                     unsigned long *green, unsigned long *blue)
{
    const struct ThemeRGB *c;
    if (theme<=BEAM_THEME_WORKBENCH || theme>BEAM_THEME_MOCHA ||
        role<0 || role>=BEAM_THEME_ROLE_COUNT || !red || !green || !blue)
        return 0;
    c=&theme_colors[theme-1][role];
    *red=spread(c->r); *green=spread(c->g); *blue=spread(c->b);
    return 1;
}

int beam_theme_color_custom(int theme, int role,
                            const unsigned long *custom_colors,
                            unsigned long *red, unsigned long *green,
                            unsigned long *blue)
{
    unsigned long color;
    if (theme!=BEAM_THEME_CUSTOM)
        return beam_theme_color(theme,role,red,green,blue);
    if (!custom_colors || role<0 || role>=BEAM_THEME_ROLE_COUNT ||
        !red || !green || !blue) return 0;
    color=custom_colors[role];
    *red=spread((unsigned char)((color>>16)&255));
    *green=spread((unsigned char)((color>>8)&255));
    *blue=spread((unsigned char)(color&255));
    return 1;
}

static int hex_value(int c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}

int beam_theme_parse_colors(const char *text, unsigned long *colors)
{
    int role,digit,value;
    unsigned long color,parsed[BEAM_THEME_ROLE_COUNT];
    const char *p=text;
    if (!p || !colors) return 0;
    for (role=0;role<BEAM_THEME_ROLE_COUNT;role++) {
        while (*p==' ' || *p=='\t') ++p;
        if (p[0]=='#') ++p;
        color=0;
        for (digit=0;digit<6;digit++) {
            value=hex_value((unsigned char)*p++);
            if (value<0) return 0;
            color=(color<<4)|(unsigned long)value;
        }
        parsed[role]=color;
        while (*p==' ' || *p=='\t') ++p;
        if (role+1<BEAM_THEME_ROLE_COUNT) {
            if (*p!=',') return 0;
            ++p;
        }
    }
    while (*p==' ' || *p=='\t') ++p;
    if (*p) return 0;
    for (role=0;role<BEAM_THEME_ROLE_COUNT;role++) colors[role]=parsed[role];
    return 1;
}

void beam_theme_format_colors(const unsigned long *colors, char *out,
                              unsigned int capacity)
{
    char buffer[48];
    if (!out || !capacity) return;
    out[0]=0;
    if (!colors) return;
    sprintf(buffer,"%06lX,%06lX,%06lX,%06lX,%06lX",
        colors[0]&0xffffffUL,colors[1]&0xffffffUL,
        colors[2]&0xffffffUL,colors[3]&0xffffffUL,
        colors[4]&0xffffffUL);
    copy_bounded(out,buffer,capacity);
}

void beam_palette_init(struct BeamPalette *out,
                       const unsigned short *pens, int count)
{
    if (!out) return;
    out->text=1;
    out->shadow=1;
    out->fill=1;
    out->fill_text=0;
    out->background=0;
    out->highlight=1;
    if (!pens || count<=BEAM_PEN_HIGHLIGHTTEXT) return;
    out->text=pens[BEAM_PEN_TEXT];
    out->shadow=pens[BEAM_PEN_SHADOW];
    out->fill=pens[BEAM_PEN_FILL];
    out->fill_text=pens[BEAM_PEN_FILLTEXT];
    out->background=pens[BEAM_PEN_BACKGROUND];
    out->highlight=pens[BEAM_PEN_HIGHLIGHTTEXT];
}

int beam_preview_reserved(int enabled, int inner_width, int available_height)
{
    if (!enabled || inner_width<BEAM_PREVIEW_MIN_WIDTH ||
        available_height<BEAM_PREVIEW_MIN_HEIGHT) return 0;
    return BEAM_PREVIEW_BOX;
}

int beam_preview_fits(int width, int height)
{
    return width>0 && height>0 &&
        width<=BEAM_PREVIEW_MAX_IMAGE && height<=BEAM_PREVIEW_MAX_IMAGE;
}
