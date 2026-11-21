#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "launcher_ui.h"
int main(void)
{
    char suggested[80];
    assert(beam_inline_complete("eD","Editor",suggested,sizeof(suggested)) && !strcmp(suggested,"eDitor"));
    assert(!beam_inline_complete("edtr","Editor",suggested,sizeof(suggested)));
    assert(!beam_inline_complete("Editor","Editor",suggested,sizeof(suggested)));
    assert(!beam_inline_complete("","Editor",suggested,sizeof(suggested)));
    assert(!beam_inline_complete("E","Editor",suggested,3));
    assert(!beam_inline_complete("E","Editor\n",suggested,sizeof(suggested)));

    /* Raw releases/modifiers between text keys must not split a burst.
       Enter, navigation and Shift+Delete must see current results. */
    assert(!beam_input_flush_search(BEAM_INPUT_TEXT,'a',0));
    assert(!beam_input_flush_search(BEAM_INPUT_RAW,0xa0,0));
    assert(!beam_input_flush_search(BEAM_INPUT_RAW,0x60,0));
    assert(!beam_input_flush_search(BEAM_INPUT_RAW,0x20,0));
    assert(beam_input_flush_search(BEAM_INPUT_TEXT,13,0));
    assert(beam_input_flush_search(BEAM_INPUT_TEXT,127,1));
    assert(!beam_input_flush_search(BEAM_INPUT_TEXT,127,0));
    assert(beam_input_flush_search(BEAM_INPUT_RAW,0x4d,0));
    assert(beam_input_flush_search(BEAM_INPUT_RAW,0x4e,0));
    assert(beam_input_flush_search(BEAM_INPUT_RAW,0x42,0));
    assert(beam_input_flush_search(BEAM_INPUT_OTHER,0,0));
    struct BeamLatency latency={0,0,0};
    char q[9]="abc";
    int cursor=3;
    assert(beam_elapsed_ms(1,999900,2,100)==0);
    assert(beam_elapsed_ms(1,900000,2,100000)==200);
    assert(beam_elapsed_ms(0,0,7200,0)==7200000UL);
    assert(beam_elapsed_ms(2,100,1,999900)==0);
    assert(beam_elapsed_ms(0,0,~0UL/1000UL+1UL,0)==~0UL);
    assert(beam_elapsed_us(1,999900,2,100)==200);
    assert(beam_elapsed_us(2,100,1,999900)==0);
    assert(beam_elapsed_us(2,200,2,100)==0);
    assert(beam_elapsed_us(0,0,~0UL/1000000UL+1UL,0)==~0UL);
    beam_latency_record(&latency,100);
    beam_latency_record(&latency,30);
    assert(latency.last==30 && latency.maximum==100 && latency.samples==2);
    latency.samples=~0UL;
    beam_latency_record(&latency,200);
    assert(latency.samples==~0UL && latency.maximum==200);
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_LEFT,0) && cursor==2);
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'X'));
    assert(!strcmp(q,"abXc") && cursor==3);
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_BACKSPACE,0));
    assert(!strcmp(q,"abc") && cursor==2);
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_DELETE,0));
    assert(!strcmp(q,"ab") && cursor==2);
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_HOME,0));
    assert(cursor==0 && !beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_BACKSPACE,0));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_END,0) && cursor==2);
    assert(!beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_RIGHT,0));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_CLEAR,0));
    assert(!strcmp(q,"") && cursor==0);
    assert(!beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'\n'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'Z'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'1'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'2'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'3'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'4'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'5'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'6'));
    assert(beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'7'));
    assert(!beam_query_edit(q,sizeof(q),&cursor,BEAM_EDIT_INSERT,'8'));
    assert(strlen(q)==8 && cursor==8);
    /* Result shortcuts: arrows, Shift+arrows and Ctrl+arrows. */
    assert(beam_result_navigate(4,10,-1,0,0,3)==3);
    assert(beam_result_navigate(4,10, 1,0,0,3)==5);
    assert(beam_result_navigate(0,10,-1,0,0,3)==9);
    assert(beam_result_navigate(9,10, 1,0,0,3)==0);
    assert(beam_result_navigate(4,10,-1,1,0,3)==1);
    assert(beam_result_navigate(4,10, 1,1,0,3)==7);
    assert(beam_result_navigate(4,10,-1,1,1,3)==0);
    assert(beam_result_navigate(4,10, 1,1,1,3)==9);
    assert(beam_result_navigate(9,10, 1,1,0,12)==9);
    assert(beam_result_navigate(0,10,-1,1,0,12)==0);
    assert(beam_result_navigate(-1,10,1,0,0,3)==1);
    assert(beam_result_navigate(20,10,-1,0,0,3)==8);
    assert(beam_result_navigate(0,0,1,0,0,3)==-1);
    assert(beam_result_navigate(0,10,0,0,0,3)==0);
    assert(beam_result_rows(38,15,155,8)==7);
    assert(beam_result_rows(38,15,60,8)==1);
    assert(beam_result_rows(38,15,38,8)==0);
    assert(beam_result_scroll(5,0,3,8)==3);
    assert(beam_result_scroll(0,3,3,8)==0);
    assert(beam_result_scroll(7,3,3,8)==5);
    assert(beam_result_scroll(0,0,0,8)==0);
    assert(beam_hit_result(36,38,15,155,6)==0);
    assert(beam_hit_result(51,38,15,155,6)==1);
    assert(beam_hit_result(109,38,15,155,6)==4);
    assert(beam_hit_result(35,38,15,155,6)==-1);
    assert(beam_hit_result(142,38,15,155,6)==-1); /* final clipped row */
    assert(beam_hit_result(151,38,15,155,4)==-1);
    {
        unsigned int next;
        assert(beam_text_line("alpha beta gamma",9,&next)==5 && next==6);
        assert(beam_text_line("longtoken",4,&next)==4 && next==4);
        assert(beam_text_line("one\ntwo",20,&next)==3 && next==4);
        assert(beam_text_line("one",20,&next)==3 && next==3);
        assert(beam_text_line("",10,&next)==0 && next==0);
        assert(beam_text_line("one",0,&next)==0 && next==0);
    }
    puts("Launcher inline edit, limits, mouse hit testing: PASS");
    return 0;
}
