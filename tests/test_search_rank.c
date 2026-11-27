#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "search_rank.h"
#include "matcher.h"
#include "beam.h"

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
    {struct BeamItem items[3];int result[3]={2,0,1};
     memset(items,0,sizeof(items));items[0].path="SYS:A";items[1].path="SYS:B";items[2].path="SYS:C";
     assert(beam_search_selection(items,3,result,3,"sys:b")==2);
     result[0]=1;result[1]=2;result[2]=0;
     assert(beam_search_selection(items,3,result,3,"SYS:B")==0);
     /* Compaction changes item indices; the selected path remains the key. */
     items[0].path="SYS:B";items[1].path="SYS:C";result[0]=1;result[1]=0;
     assert(beam_search_selection(items,2,result,2,"SYS:B")==1);
     assert(beam_search_selection(items,2,result,2,"SYS:Removed")==0);
    }
    struct BeamQueryMasks prefs_masks;
    const char *prefs_words=beam_search_builtin_alias("Beam68Prefs");
    assert(prefs_words && !beam_search_builtin_alias("OtherPrefs"));
    assert(beam_search_rank_alias("preferences","Beam68Prefs",prefs_words,-1,0)>0);
    assert(beam_search_rank_alias("settings","Beam68Prefs",prefs_words,-1,0)>0);
    beam_query_masks_build("preferences",&prefs_masks);
    assert(!beam_query_masks_reject(&prefs_masks,beam_text_mask("Beam68Prefs")|beam_text_mask(prefs_words)));

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
    assert(beam_search_rank("sh","Shell",-1,144)<beam_search_rank("sh","Shell",0,0));
    assert(beam_search_rank("srceen","ScreenMode",-1,0)>beam_search_rank("srceen","A ScreenMode",-1,0));
    assert(beam_search_before(10,"Alpha","SYS:A",10,"Beta","SYS:B"));
    assert(beam_search_before(10,"Same","SYS:A",10,"same","SYS:B"));
    assert(!beam_search_before(9,"Alpha","SYS:A",10,"Beta","SYS:B"));
    assert(beam_search_rank("sh","Shell",-1,0)>beam_search_rank_alias("sh","Other","sh",0,144));
    {struct BeamItem items[1];int indices[1]={0};memset(items,0,sizeof(items));items[0].path="SYS:Other";
     assert(beam_search_find_selection(items,1,indices,1,"SYS:Missing")==-1);
     assert(beam_search_find_selection(items,1,indices,1,"sys:other")==0);}
    {struct BeamItem items[50];char names[50][20],paths[50][30];int permutation,cap,i,j;
     for(i=0;i<50;i++){sprintf(names[i],"App%02d",49-i);sprintf(paths[i],"Work:%s",names[i]);items[i].name=names[i];items[i].path=paths[i];}
     for(permutation=0;permutation<7;permutation++)for(cap=1;cap<20;cap++) {
       int results[50],scores[50],count=0,reference[50],reference_scores[50];
       for(i=0;i<50;i++){int index=(i*7+permutation)%50,score=100+(index%3);
         beam_search_top_add(items,index,score,results,scores,&count,cap,0);
         reference[i]=index;reference_scores[i]=score;
       }
       for(i=1;i<50;i++)for(j=i;j>0 && beam_search_before(reference_scores[j],items[reference[j]].name,items[reference[j]].path,
           reference_scores[j-1],items[reference[j-1]].name,items[reference[j-1]].path);j--){
         int t=reference[j];reference[j]=reference[j-1];reference[j-1]=t;
         t=reference_scores[j];reference_scores[j]=reference_scores[j-1];reference_scores[j-1]=t;
       }
       assert(count==cap);for(i=0;i<cap;i++)assert(results[i]==reference[i] && scores[i]==reference_scores[i]);
     }}
    {struct BeamItem items[4];int q,i,j;const char *names[]={"App","Application","Other","Application19"};
     const char *queries[]={"app","application","application19","z"};
     for(i=0;i<4;i++){memset(&items[i],0,sizeof(items[i]));items[i].name=(char *)names[i];items[i].path=(char *)names[i];}
     for(q=0;q<4;q++){int results[4],scores[4],n=0,ref[4],refscore[4],rn=0;
       for(i=0;i<4;i++){int score=beam_search_rank_alias(queries[q],names[i],"application19",i==2?0:-1,i*40);
         if(score){beam_search_top_add(items,i,score,results,scores,&n,4,(unsigned int)strlen(queries[q]));ref[rn]=i;refscore[rn++]=score;}}
       for(i=1;i<rn;i++)for(j=i;j>0 && beam_search_before(refscore[j],names[ref[j]],names[ref[j]],refscore[j-1],names[ref[j-1]],names[ref[j-1]]);j--){
         int t=ref[j];ref[j]=ref[j-1];ref[j-1]=t;t=refscore[j];refscore[j]=refscore[j-1];refscore[j-1]=t;}
       assert(n==rn);for(i=0;i<n;i++)assert(results[i]==ref[i] && scores[i]==refscore[i]);
     }}
    puts("Search rank: names, aliases, pins, history and blank-query order: PASS");
    return 0;
}
