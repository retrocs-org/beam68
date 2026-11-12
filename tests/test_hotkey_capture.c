#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hotkey_capture.h"

static void expect(unsigned code, unsigned modifiers, int mapped,
                   const char *expected)
{
    char value[96];
    assert(beam_hotkey_format(value,sizeof(value),code,modifiers,mapped));
    assert(strcmp(value,expected)==0);
}

int main(void)
{
    char value[12];
    expect(0x40,BEAM_HOTKEY_CONTROL|BEAM_HOTKEY_ALT,0,
           "rawkey control alt space");
    expect(0x50,BEAM_HOTKEY_SHIFT|BEAM_HOTKEY_LCOMMAND,0,
           "rawkey shift lcommand f1");
    expect(0x59,BEAM_HOTKEY_RCOMMAND,0,"rawkey rcommand f10");
    expect(0x10,BEAM_HOTKEY_ALT,'Q',"rawkey alt q");
    expect(0x4c,0,0,"rawkey up");
    assert(beam_hotkey_is_modifier(0x60));
    assert(beam_hotkey_is_modifier(0x67));
    assert(!beam_hotkey_is_modifier(0x40));
    assert(!beam_hotkey_format(value,sizeof(value),0x60,0,0));
    assert(!beam_hotkey_format(value,sizeof(value),0x10,0,0));
    assert(!beam_hotkey_format(value,sizeof(value),0x10,0,' '));
    assert(!beam_hotkey_format(value,sizeof(value),0x40,
                               BEAM_HOTKEY_CONTROL|BEAM_HOTKEY_ALT,0));
    assert(!beam_hotkey_has_duplicates("rawkey control alt space"));
    assert(beam_hotkey_has_duplicates("rawkey control control space"));
    assert(beam_hotkey_has_duplicates("rawkey control space f1"));
    assert(beam_hotkey_has_duplicates("rawkey alt alt q"));
    assert(beam_hotkey_has_duplicates("control alt space"));
    puts("Hotkey capture formatting tests passed.");
    return 0;
}
