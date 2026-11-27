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
    assert(beam_calc_result("1.5 MiB to KiB",out,sizeof(out)) && !strcmp(out,"1536 KiB"));
    assert(beam_calc_result("90 min to h",out,sizeof(out)) && !strcmp(out,"1.5 h"));
    assert(beam_calc_result("1 in to cm",out,sizeof(out)) && !strcmp(out,"2.54 cm"));
    assert(beam_calc_result("-2 kg to g",out,sizeof(out)) && !strcmp(out,"-2000 g"));
    assert(beam_calc_result("1 GiB to B",out,sizeof(out)) && !strcmp(out,"1073741824 B"));
    assert(beam_calc_result("1 s in min",out,sizeof(out)) && !strcmp(out,"~0.016666666666 min"));
    assert(!beam_calc_result("1 m to kg",out,sizeof(out)) && !out[0]);
    assert(!beam_calc_result("1.2345 m to cm",out,sizeof(out)));
    assert(!beam_calc_result("1 MB to KiB junk",out,sizeof(out)));
    assert(!beam_calc_result("4294967296 B to KB",out,sizeof(out)));
    assert(!beam_calc_result("1 m to cm",out,2) && !out[0]);
    assert(beam_calc_result("1 mm to km",out,sizeof(out)) && !strcmp(out,"0.000001 km"));
    assert(beam_calc_result("1 B to GiB",out,sizeof(out)) && !strcmp(out,"~0.000000000931 GiB"));
    assert(beam_calc_result("0 C to F",out,sizeof(out)) && !strcmp(out,"32 F"));
    assert(beam_calc_result("-40 F to C",out,sizeof(out)) && !strcmp(out,"-40 C"));
    assert(beam_calc_result("0 C to K",out,sizeof(out)) && !strcmp(out,"273.15 K"));
    assert(beam_calc_result("-273.15 C to K",out,sizeof(out)) && !strcmp(out,"0 K"));
    assert(beam_calc_result("-459.67 F to K",out,sizeof(out)) && !strcmp(out,"0 K"));
    assert(!beam_calc_result("-273.151 C to F",out,sizeof(out)));
    assert(!beam_calc_result("-1 K to C",out,sizeof(out)));
    assert(!beam_calc_result("1 C to cm",out,sizeof(out)));
    puts("All portable core tests passed.");
    return 0;
}
