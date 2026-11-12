#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "catalogue.h"
int main(void)
{
    struct BeamCatalogueEntry e;
    char cmd[300];
    assert(beam_catalogue_parse_line("PLUGIN|org.ex.weather|Weather|1.2|Ann|Forecast|http://ex.org/W-1.2.lha",&e));
    assert(e.kind==BEAM_CAT_PLUGIN && !strcmp(e.name,"Weather") && !strcmp(e.url,"http://ex.org/W-1.2.lha"));
    assert(beam_catalogue_parse_line("THEME|org.ex.amber|Amber|1.0|Ann|Warm|http://ex.org/Amber.theme\n",&e) && e.kind==BEAM_CAT_THEME);
    assert(!beam_catalogue_parse_line("THEME|org.ex.amber|Amber|1.0|Ann|Warm|http://ex.org/Amber.lha",&e));
    assert(!beam_catalogue_parse_line("PLUGIN|bad id|X|1|A|D|http://x/y.lha",&e));
    assert(!beam_catalogue_parse_line("PLUGIN|a.b|X|one|A|D|http://x/y.lha",&e));
    assert(!beam_catalogue_parse_line("PLUGIN|a.b|X|1|A|D|http://x/y.exe",&e));
    assert(!beam_catalogue_parse_line("# comment",&e) && !beam_catalogue_parse_line("",&e));
    assert(beam_catalogue_command("wget -q -O \"{file}\" \"{url}\"","http://ex.org/a.lha","T:dl.lha",cmd,sizeof(cmd)));
    assert(!strcmp(cmd,"wget -q -O \"T:dl.lha\" \"http://ex.org/a.lha\""));
    assert(!beam_catalogue_command("x {url}","http://ex.org/a b.lha","T:f",cmd,sizeof(cmd)));
    assert(!beam_catalogue_command("x {url}","http://ex.org/\"x.lha","T:f",cmd,sizeof(cmd)));
    puts("Catalogue: index lines, kinds, validation, download command: PASS");
    return 0;
}
