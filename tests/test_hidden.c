#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hidden.h"
int main(void)
{
    static struct BeamHidden h,g;
    if (system("mkdir -p build/testsettings/ENVARC/Beam68")!=0) return 1;
    h.count=0;
    assert(beam_hidden_toggle(&h,"SYS:Tools/Junk")==1);
    assert(beam_hidden_toggle(&h,"Work:Old")==1);
    assert(beam_hidden_contains(&h,"sys:tools/junk"));
    assert(beam_hidden_toggle(&h,"SYS:TOOLS/JUNK")==0 && h.count==1);
    assert(beam_hidden_toggle(&h,"")==-1);
    assert(beam_hidden_save(&h,BEAM_HIDDEN_PATH));
    beam_hidden_load(&g,BEAM_HIDDEN_PATH);
    assert(g.count==1 && beam_hidden_contains(&g,"Work:Old"));
    {
        static struct BeamRecentDocs r,q;
        int i;
        char p[32];
        r.count=0;
        beam_recent_docs_record(&r,"Work:A.txt");
        beam_recent_docs_record(&r,"Work:B.txt");
        beam_recent_docs_record(&r,"work:a.txt");
        assert(r.count==2 && !strcmp(r.paths[0],"work:a.txt") && !strcmp(r.paths[1],"Work:B.txt"));
        for (i=0;i<40;i++) { sprintf(p,"RAM:%d",i); beam_recent_docs_record(&r,p); }
        assert(r.count==BEAM_RECENT_DOCS_DEFAULT && !strcmp(r.paths[0],"RAM:39"));
        assert(beam_recent_docs_save(&r,BEAM_RECENT_DOCS_PATH));
        beam_recent_docs_load(&q,BEAM_RECENT_DOCS_PATH);
        assert(q.count==BEAM_RECENT_DOCS_DEFAULT && !strcmp(q.paths[15],"RAM:24"));
    }
    {
        static struct BeamRecentDocs small;
        assert(beam_recent_docs_size(&small,3));
        beam_recent_docs_record(&small,"A:1"); beam_recent_docs_record(&small,"A:2");
        beam_recent_docs_record(&small,"A:3"); beam_recent_docs_record(&small,"A:4");
        assert(small.count==3 && !strcmp(small.paths[0],"A:4") && !strcmp(small.paths[2],"A:2"));
        assert(beam_recent_docs_size(&small,2) && small.count==2 && !strcmp(small.paths[1],"A:3"));
        beam_recent_docs_free(&small);
    }
    puts("Hidden results and recent documents: toggle, case-insensitive match, persistence: PASS");
    return 0;
}
