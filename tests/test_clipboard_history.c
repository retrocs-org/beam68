#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "clipboard_history.h"
int main(void)
{
    struct BeamClipHistory h,loaded;char long_text[122];unsigned long first;FILE*f;int i;
    beam_clip_history_init(&h,2,256,60);
    assert(beam_clip_history_add(&h,"one",100));first=h.entries[0].id;
    assert(beam_clip_history_add(&h,"two",101));assert(beam_clip_history_add(&h,"three",102));
    assert(h.count==2 && !beam_clip_history_find(&h,first,103));
    assert(beam_clip_history_add(&h,"two",104) && h.count==2 && !strcmp(h.entries[0].text,"two"));
    assert(beam_clip_history_match("Hello World","WORLD"));assert(!beam_clip_history_match("Hello","abc"));
    assert(beam_clip_history_save(&h,"build/test-clipboard-history.tmp"));
    beam_clip_history_init(&loaded,16,2048,60);
    assert(beam_clip_history_load(&loaded,"build/test-clipboard-history.tmp",105));
    assert(loaded.count==2 && !strcmp(loaded.entries[0].text,"two"));
    assert(beam_clip_history_expire(&loaded,164) && loaded.count==0);
    memset(long_text,'x',121);long_text[121]=0;assert(!beam_clip_history_add(&h,long_text,104));
    beam_clip_history_init(&h,16,256,60);
    long_text[120]=0;
    for(i=0;i<8;i++){long_text[0]=(char)('a'+i);assert(beam_clip_history_add(&h,long_text,100+i));}
    assert(h.count==2);
    f=fopen("build/test-clipboard-history.tmp","wb");assert(f);fputs("BEAMCLIP1 999 2048 60 1\n",f);fclose(f);
    assert(!beam_clip_history_load(&loaded,"build/test-clipboard-history.tmp",105));
    assert(!beam_clip_history_add(&h,"",105));
    assert(beam_clip_history_add(&h,"clock rollback",200));
    assert(beam_clip_history_expire(&h,1) && h.count==0);
    remove("build/test-clipboard-history.tmp");puts("Clipboard history bounds, expiry, deduplication, search and malformed-file rejection: PASS");return 0;
}
