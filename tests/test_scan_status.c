#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "scan_status.h"
int main(void)
{
    struct BeamScanStatus s={0,4,0,0,0,0,0,1,0,0};
    char line[256],shortline[7];
    assert(!beam_scan_cacheable(&s));
    assert(beam_scan_status_format(&s,line,sizeof(line)));
    assert(strstr(line,"scanning=1")!=NULL);
    assert(strstr(line,"roots=0/4")!=NULL);
    assert(beam_scan_progress_format(&s,line,sizeof(line)));
    assert(!strcmp(line,"Indexing 0/4 drawers | 0 entries | 0 apps"));
    assert(!beam_scan_status_format(&s,shortline,sizeof(shortline)));
    assert(shortline[0]==0);
    s.roots_done=4;s.entries=153;s.applications=19;s.unreadable=2;
    s.io_errors=1;s.overlong=3;s.scanning=0;s.capped=1;
    assert(beam_scan_status_format(&s,line,sizeof(line)));
    assert(strstr(line,"roots=4/4")!=NULL);
    assert(strstr(line,"apps=19")!=NULL);
    assert(strstr(line,"unreadable=2")!=NULL);
    assert(strstr(line,"ioerrors=1")!=NULL);
    assert(strstr(line,"capped=1")!=NULL);
    assert(!beam_scan_cacheable(&s)); /* capped / errors */
    s.cached=1;
    assert(beam_scan_status_format(&s,line,sizeof(line)));
    assert(strstr(line,"cached=1")!=NULL);
    assert(beam_scan_progress_format(&s,line,sizeof(line)));
    assert(!strcmp(line,"Index cache: 19 applications"));
    s.scanning=0; s.capped=0; s.cached=0; s.unreadable=0; s.io_errors=0; s.overlong=0;
    assert(beam_scan_cacheable(&s));
    s.capped=1; assert(!beam_scan_cacheable(&s)); s.capped=0;
    s.unreadable=1; assert(!beam_scan_cacheable(&s)); s.unreadable=0;
    s.io_errors=1; assert(!beam_scan_cacheable(&s)); s.io_errors=0;
    s.overlong=1; assert(!beam_scan_cacheable(&s)); s.overlong=0;
    s.roots_done=2; assert(!beam_scan_cacheable(&s)); s.roots_done=4;
    s.scanning=1; assert(!beam_scan_cacheable(&s));
    s.scanning=0;s.cached=0;
    assert(beam_scan_progress_format(&s,line,sizeof(line)));
    assert(!strcmp(line,"Indexed 19 apps from 4 drawers"));
    puts("Index scan progress, cache-completeness policy and diagnostics: PASS");
    return 0;
}
