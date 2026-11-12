#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "favorites.h"
static void write_data(const char *data)
{
    FILE *p=fopen("build/testsettings/ENVARC/Beam68/Favorites.txt","wb");
    assert(p); assert(fputs(data,p)>=0); assert(!fclose(p));
}
int main(void)
{
    static struct BeamFavorites a,b;
    unsigned int i;
    char path[64];
    if (system("mkdir -p build/testsettings/T/Beam68 build/testsettings/ENV/Beam68 build/testsettings/ENVARC/Beam68")!=0) { fputs("test setup command failed\n",stderr); return 1; }
    beam_favorites_clear(&a);
    assert(beam_favorites_toggle(&a,"SYS:Tools/Ed")==1);
    assert(beam_favorites_toggle(&a,"sys:tools/ED")==0);
    assert(a.count==0);
    assert(beam_favorites_add(&a,"SYS:Tools/Ed"));
    assert(beam_favorites_add(&a,"sys:tools/ed") && a.count==1);
    assert(!beam_favorites_add(&a,"Bad\nPath"));
    assert(!beam_favorites_add(&a,"Bad\"Path"));
    assert(beam_favorites_add(&a,"Games:Deluxe Paint"));
    assert(beam_favorites_set_group(&a,"Games:Deluxe Paint","Graphics"));
    assert(beam_favorites_in_group(&a,"games:deluxe paint","graphics"));
    assert(beam_favorites_save(&a,BEAM_FAVS_ENVARC));
    assert(beam_favorites_load(&b,BEAM_FAVS_ENVARC));
    assert(b.count==2 && beam_favorites_contains(&b,"sys:TOOLS/ED"));
    assert(beam_favorites_in_group(&b,"Games:Deluxe Paint","Graphics"));
    assert(beam_favorites_remove(&b,"SYS:TOOLS/ED"));
    assert(b.count==1 && !beam_favorites_contains(&b,"SYS:Tools/Ed"));
    write_data("# pinned apps\nTools|SYS:Tools/Ed\nTools|SYS:TOOLS/ed\n; ignored\n\nGraphics|Games:Deluxe Paint\n\tbad\n");
    assert(beam_favorites_load(&b,BEAM_FAVS_ENVARC));
    assert(b.count==2);
    assert(beam_favorites_in_group(&b,"SYS:Tools/Ed","Tools"));
    assert(beam_favorites_save(&b,BEAM_FAVS_ENVARC));
    {
        FILE *c=fopen("build/testsettings/ENVARC/Beam68/Favorites.txt","rb");
        char line[256];
        int found=0;
        assert(c);
        while (fgets(line,sizeof(line),c)) if (!strcmp(line,"# pinned apps\n")) ++found;
        fclose(c);
        assert(found==1);
    }
    beam_favorites_clear(&a);
    for (i=0;i<BEAM_FAVORITES_CAP;i++) {
        sprintf(path,"Apps:App%u",i);
        assert(beam_favorites_add(&a,path));
    }
    assert(!beam_favorites_add(&a,"Apps:Extra"));
    assert(beam_favorites_toggle(&a,"Apps:Extra")==-1);
    assert(beam_favorites_save(&a,BEAM_FAVS_ENV));
    assert(beam_favorites_load_active(&b));
    assert(b.count==BEAM_FAVORITES_CAP);
    beam_favorites_clear(&a);
    assert(beam_favorites_add(&a,"RAM:SessionApp"));
    assert(beam_favorites_save(&a,BEAM_FAVS_SESSION));
    assert(beam_favorites_load_active(&b));
    assert(b.count==1 && beam_favorites_contains(&b,"RAM:SessionApp"));
    puts("Favorites: case folding, toggle, comments, capacity, session priority, atomic files: PASS");
    return 0;
}
