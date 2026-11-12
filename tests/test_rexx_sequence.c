#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "rexx_sequence.h"
int main(void)
{
    char command[200];
    assert(beam_rexx_next(REXX_DOPUS_NEW,1,"42","SYS:Tools","Clock",command,sizeof(command))==REXX_DOPUS_WAIT);
    assert(!strcmp(command,"lister wait 42"));
    assert(beam_rexx_next(REXX_DOPUS_WAIT,1,"42","SYS:Tools","Clock",command,sizeof(command))==REXX_DOPUS_SELECT);
    assert(!strcmp(command,"lister select 42 \"Clock\" on"));
    assert(beam_rexx_next(REXX_DOPUS_SELECT,1,"42","SYS:Tools","Clock",command,sizeof(command))==REXX_DOPUS_REFRESH);
    assert(!strcmp(command,"lister refresh 42"));
    assert(!beam_rexx_next(REXX_DOPUS_REFRESH,1,"42","SYS:Tools","Clock",command,sizeof(command)));
    assert(beam_rexx_next(REXX_DOPUS_NEW,0,"","SYS:Tools","Clock",command,sizeof(command))==REXX_DOPUS_SCAN);
    assert(!strcmp(command,"ScanDir \"SYS:Tools\""));
    assert(!beam_rexx_next(REXX_DOPUS_WAIT,0,"42","SYS:Tools","Clock",command,sizeof(command)));
    assert(!beam_rexx_next(REXX_MODULE,1,"","","",command,sizeof(command)));
    assert(!beam_rexx_next(REXX_DOPUS_NEW,1,"","SYS:","",command,sizeof(command)));
    assert(beam_rexx_next(REXX_DOPUS_WAIT,1,"42;quit","SYS:","a",command,sizeof(command))==-1);
    assert(beam_rexx_next(REXX_DOPUS_NEW,0,"","SYS:*N","",command,sizeof(command))==-1);
    assert(beam_rexx_next(REXX_DOPUS_WAIT,1,"42","SYS:","a",command,5)==-1);
    assert(!beam_rexx_handle_valid("123456789012345678901234"));
    assert(!beam_rexx_handle_valid(""));
    puts("Asynchronous ARexx sequence, fallback and command bounds: PASS");
    return 0;
}
