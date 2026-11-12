#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "matcher.h"
#include "calc_engine.h"
#include "plugin_protocol.h"
static void good(const char *s, long expected)
{
    long value=0;
    assert(beam_calculate(s,&value));
    if (value!=expected) printf("%s => %ld (wanted %ld)\n",s,value,expected);
    assert(value==expected);
}
static void bad(const char *s)
{
    long val;
    assert(!beam_calculate(s,&val));
}
int main(void)
{
    char out[64];
    assert(beam_match_score("p", "Plex") > beam_match_score("px", "Plex"));
    assert(beam_match_score("pl", "Plex") > beam_match_score("lex", "Plex"));
    assert(beam_match_score("wb", "WorkBench") > 0);
    assert(beam_match_score("fit", "File Index Tool") > 0);
    assert(beam_match_score("zzz", "Plex") == 0);
    assert(beam_match_score("dp", "Deluxe Paint") >= 5000);
    assert(beam_match_score("wb", "WorkBench") >= 5000);
    assert(beam_match_score("fb", "File Browser") >= 5000);
    assert(beam_match_score("fi", "File Browser") > beam_match_score("fb", "File Browser"));
    assert(beam_match_score("qq", "File Browser") == 0);
    assert(beam_match_score("srceen", "ScreenMode") >= 3000);
    assert(beam_match_score("sxreen", "ScreenMode") >= 3000);
    assert(beam_match_score("scren", "ScreenMode") >= 3000);
    assert(beam_match_score("zzreen", "ScreenMode") == 0);
    assert(beam_match_score("ed", "Ed")==10000);
    good("2+2",4); good("2*(5+3)",16); good(" -12 / (2+1) ",-4);
    good("1+2*3",7); good(" 100/(2+3)",20);
    good("2147483647",2147483647L);
    good("-2147483647",-2147483647L);
    good("2* -3",-6); good("(10)-(3+2)",5);
    bad(""); bad("   "); bad("2++"); bad("2/0"); bad("(2+1");
    bad("2+1)"); bad("2^8"); bad("2147483648");
    bad("2147483647+1"); bad("100000*100000");
    bad("-2147483647-2"); bad("0/0");
    assert(beam_decode_hex("68656C6C6F",out,sizeof(out)));
    assert(!strcmp(out,"hello"));
    assert(beam_decode_hex("-",out,sizeof(out)) && !out[0]);
    assert(!beam_decode_hex("X0",out,sizeof(out)));
    assert(!beam_decode_hex("0",out,sizeof(out)));
    {
        long v; char out[80];
        assert(beam_calculate("$FF",&v) && v==255);
        assert(beam_calculate("0x10+%101",&v) && v==21);
        assert(beam_calculate("$FFFFFFFF",&v) && v==-1);
        assert(beam_calculate("%1111*2",&v) && v==30);
        assert(!beam_calculate("$",&v) && !beam_calculate("%2",&v));
        assert(!beam_calculate("$123456789",&v));
        beam_calc_format(255,out,sizeof(out)); assert(!strcmp(out,"255 $FF %11111111"));
        beam_calc_format(-1,out,sizeof(out)); assert(!strcmp(out,"-1 $FFFFFFFF"));
        beam_calc_format(0,out,sizeof(out)); assert(!strcmp(out,"0 $0 %0"));
        beam_calc_format(70000,out,sizeof(out)); assert(!strcmp(out,"70000 $11170"));
    }
    puts("All portable core tests passed.");
    return 0;
}
