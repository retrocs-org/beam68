#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "search_rank.h"
#include "matcher.h"

/* The mask reject must never drop an item the full ranker would accept. */
static void mask_sound(const char *q,const char *name,const char *alias)
{
    struct BeamQueryMasks m;
    unsigned long mask=beam_text_mask(name)|(alias?beam_text_mask(alias):0UL);
    beam_query_masks_build(q,&m);
    if (beam_search_rank_alias(q,name,alias,-1,0)>0) assert(!beam_query_masks_reject(&m,mask));
}
int main(void)
{
    assert(beam_search_rank("","Deluxe Paint",-1,0)==0);
    assert(beam_search_rank("","Deluxe Paint",0,0)>0);
    assert(beam_search_rank("","File Browser",0,0)>
           beam_search_rank("","Deluxe Paint",4,100));
    assert(beam_search_rank("zz","Deluxe Paint",0,200)==0);
    assert(beam_search_rank("dp","Deluxe Paint",-1,0)>0);
    assert(beam_search_rank("dp","Deluxe Paint",0,0)>
           beam_search_rank("dp","Deluxe Paint",-1,0));
    assert(beam_search_rank("ed","Ed",-1,0)>
           beam_search_rank("ed","Text Editor",0,144));
    assert(beam_search_rank("de","Deluxe Paint",-1,0)>
           beam_search_rank("dp","Deluxe Paint",0,144));
    assert(beam_search_rank_alias("terminal","Shell","terminal cli console",-1,0)>0);
    assert(beam_search_rank_alias("cli","Shell","terminal cli console",-1,0)>0);
    assert(beam_search_rank_alias("shell","Shell","terminal",-1,0)>
           beam_search_rank_alias("terminal","Shell","terminal",-1,0));
    assert(beam_search_rank_alias("paint","Shell","terminal cli",-1,0)==0);
    assert(beam_search_rank_alias("paint deluxe","Deluxe Paint","graphics editor",-1,0)>0);
    assert(beam_search_rank_alias("shell console","Shell","terminal cli console",-1,0)>0);
    assert(beam_search_rank_alias("shell graphics","Shell","terminal cli console",-1,0)==0);
    /* Loose scattered terms must not beat a real substring hit. */
    assert(beam_search_rank_alias("pt dx","Deluxe Paint",0,-1,0)<
           beam_search_rank_alias("paint","Deluxe Paint",0,-1,0));
    {
        static const char *names[]={"Deluxe Paint","ScreenMode","Shell","File Browser","Ed","WorkBench","MultiView","Calculator",0};
        static const char *queries[]={"dp","srceen","sxreen","scren","shell","fb","ed","wb","mv","calc","paint deluxe","sh co","xyz","de pa","mlti","multivew","c4lc",0};
        int i,j;
        for (i=0;names[i];i++) for (j=0;queries[j];j++) {
            mask_sound(queries[j],names[i],0);
            mask_sound(queries[j],names[i],"terminal cli console graphics editor");
        }
        {
            struct BeamQueryMasks m;
            beam_query_masks_build("xyz",&m);
            assert(beam_query_masks_reject(&m,beam_text_mask("Deluxe Paint")));
            beam_query_masks_build("srceen",&m);
            assert(!beam_query_masks_reject(&m,beam_text_mask("ScreenMode")));
        }
    }
    assert(beam_match_marks("multi","MultiView")==0x1FUL);
    assert(beam_match_marks("view","MultiView")==(0xFUL<<5));
    assert(beam_match_marks("dp","Deluxe Paint")==((1UL<<0)|(1UL<<7)));
    assert(beam_match_marks("scren","ScreenMode")!=0);
    assert(beam_match_marks("xyz","ScreenMode")==0);
    assert(beam_query_marks("paint lux","Deluxe Paint")==((0x1FUL<<7)|(0x7UL<<2)));
    /* Exact/prefix real names outrank every possible alias or term score. */
    assert(beam_search_rank_alias("scr","ScreenMode","screen display settings",-1,0)==
           beam_search_rank_alias("scr","ScreenMode",0,-1,0));
    assert(beam_search_rank_alias("screenmode","ScreenMode","screenmode",0,100)==
           beam_search_rank_alias("screenmode","ScreenMode",0,0,100));
    puts("Search rank: names, aliases, pins, history and blank-query order: PASS");
    return 0;
}
