#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "appearance.h"

int main(void)
{
    struct BeamPalette p;
    unsigned long r,g,b,colors[BEAM_THEME_ROLE_COUNT],unchanged[BEAM_THEME_ROLE_COUNT];
    char formatted[48];
    unsigned short custom[9]={3,4,7,8,9,12,6,5,15};
    beam_palette_init(&p,NULL,0);
    assert(p.background==0 && p.text==1 && p.fill_text==0);
    beam_palette_init(&p,custom,9);
    assert(p.background==5 && p.text==7);
    assert(p.fill==12 && p.fill_text==6 && p.highlight==15);
    beam_palette_init(&p,custom,3); /* insufficient DrawInfo pens */
    assert(p.background==0 && p.fill==1);
    assert(beam_preview_reserved(0,640,120)==0);
    assert(beam_preview_reserved(1,339,120)==0);
    assert(beam_preview_reserved(1,500,57)==0);
    assert(beam_preview_reserved(1,340,58)==BEAM_PREVIEW_BOX);
    assert(beam_preview_fits(48,48));
    assert(!beam_preview_fits(49,48));
    assert(!beam_preview_fits(32,49));
    assert(!beam_preview_fits(-1,32));
    assert(!strcmp(beam_theme_name(BEAM_THEME_WORKBENCH),"Workbench"));
    assert(!strcmp(beam_theme_description(BEAM_THEME_WORKBENCH),"Workbench system colors"));
    assert(strstr(beam_theme_description(BEAM_THEME_MOCHA),"bright blue")!=NULL);
    assert(beam_theme_from_name("mocha")==BEAM_THEME_MOCHA);
    assert(beam_theme_from_name("custom")==BEAM_THEME_CUSTOM);
    assert(beam_theme_from_name("unknown")==BEAM_THEME_WORKBENCH);
    assert(beam_theme_color(BEAM_THEME_LATTE,BEAM_THEME_ROLE_FILL,&r,&g,&b));
    assert(r==0x72727272UL && g==0x87878787UL && b==0xfdfdfdfdUL);
    assert(!beam_theme_color(BEAM_THEME_WORKBENCH,BEAM_THEME_ROLE_FILL,&r,&g,&b));
    assert(beam_theme_parse_colors("#112233,445566,778899,AABBCC,DDEEFF",colors));
    assert(colors[0]==0x112233UL && colors[4]==0xddeeffUL);
    assert(beam_theme_color_custom(BEAM_THEME_CUSTOM,BEAM_THEME_ROLE_FILL,
                                   colors,&r,&g,&b));
    assert(r==0x77777777UL && g==0x88888888UL && b==0x99999999UL);
    beam_theme_format_colors(colors,formatted,sizeof(formatted));
    assert(!strcmp(formatted,"112233,445566,778899,AABBCC,DDEEFF"));
    memcpy(unchanged,colors,sizeof(colors));
    assert(!beam_theme_parse_colors("112233,bad",colors));
    assert(!memcmp(unchanged,colors,sizeof(colors)));
    puts("appearance tests passed");
    return 0;
}
