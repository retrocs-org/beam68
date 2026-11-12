#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "plugin_common.h"

int main(void)
{
    char text[32]="  Hello World  ";
    char copy[6];
    assert(!strcmp(beam_plugin_trim(text),"Hello World"));
    assert(beam_ascii_equal("Find:","fIND:"));
    assert(beam_ascii_prefix("diskfree:RAM:","DISKFREE:"));
    assert(beam_ascii_contains("ScreenMode","mode"));
    assert(!beam_ascii_contains("Workbench","screen"));
    beam_plugin_copy(copy,"abcdef",sizeof(copy));
    assert(!strcmp(copy,"abcde"));
    puts("Plugin command text helpers: PASS");
    return 0;
}
