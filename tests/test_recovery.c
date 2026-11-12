#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "recovery.h"

int main(void)
{
    char first[80],second[80];
    assert(beam_index_retry_capacity(2048)==1024);
    assert(beam_index_retry_capacity(31)==16);
    assert(beam_index_retry_capacity(16)==0);
    beam_no_results_assistance(1,0,first,sizeof(first),second,sizeof(second));
    assert(strstr(first,"Indexing")!=NULL && strstr(second,"F2")!=NULL);
    beam_no_results_assistance(0,0,first,sizeof(first),second,sizeof(second));
    assert(strstr(first,"No indexed")!=NULL && strstr(second,"F5")!=NULL);
    beam_no_results_assistance(0,12,first,sizeof(first),second,sizeof(second));
    assert(strstr(first,"No matching")!=NULL && strstr(second,"Ctrl+L")!=NULL);
    puts("Low-memory retry and no-results assistance: PASS");
    return 0;
}
