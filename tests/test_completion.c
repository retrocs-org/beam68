#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "completion.h"

int main(void)
{
    struct BeamCompletions list;
    beam_completion_reset(&list);
    assert(beam_completion_add(&list,"sh","shell:dir "));
    assert(beam_completion_add(&list,"sh","shell:list "));
    assert(!beam_completion_add(&list,"sh","SHELL:DIR "));
    assert(!beam_completion_add(&list,"shell:dir ","shell:dir "));
    assert(list.count==2 && list.selected==0);
    assert(!strcmp(beam_completion_selected(&list),"shell:dir "));
    assert(beam_completion_move(&list,1)==1);
    assert(!strcmp(beam_completion_selected(&list),"shell:list "));
    assert(beam_completion_move(&list,1)==0);
    assert(beam_completion_move(&list,-1)==1);
    puts("Visible completion candidates, dedup and wrapping navigation: PASS");
    return 0;
}
