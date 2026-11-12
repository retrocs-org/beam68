#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sources.h"
int main(void)
{
    char url[200],title[100],name[48],text[200],cmd[200];
    assert(beam_module_name("mod.SpaceDebris") && beam_module_name("Axel F.MOD"));
    assert(beam_module_name("tune.xm") && beam_module_name("P61.dance") && beam_module_name("a.mmd1"));
    assert(!beam_module_name("readme") && !beam_module_name("module.c") && !beam_module_name(".mod"));
    assert(beam_bookmark_parse("<li><a href=\"http://aminet.net/\">Aminet <b>archive</b></a></li>",
                               url,sizeof(url),title,sizeof(title)));
    assert(!strcmp(url,"http://aminet.net/") && !strcmp(title,"Aminet archive"));
    assert(beam_bookmark_parse("<A HREF='https://eab.abime.net/'>EAB</A>",url,sizeof(url),title,sizeof(title)));
    assert(!strcmp(url,"https://eab.abime.net/") && !strcmp(title,"EAB"));
    assert(beam_bookmark_parse("<a href=\"http://x.org/\"></a>",url,sizeof(url),title,sizeof(title)) &&
           !strcmp(title,"http://x.org/"));
    assert(!beam_bookmark_parse("<a href=\"javascript:evil()\">x</a>",url,sizeof(url),title,sizeof(title)));
    assert(!beam_bookmark_parse("<a href=\"http://a/*b\">x</a>",url,sizeof(url),title,sizeof(title)));
    assert(!beam_bookmark_parse("plain text",url,sizeof(url),title,sizeof(title)));
    {
        const char *next; int partial; int count=0;
        const char *two="<a href=\"http://a.org/\">A</a> | <A HREF=\"http://b.org/\">B</A>";
        for (next=two;(next=beam_bookmark_next(next,url,sizeof(url),title,sizeof(title),&partial));) {
            ++count;
            assert(!strcmp(url,count==1?"http://a.org/":"http://b.org/"));
            assert(!strcmp(title,count==1?"A":"B"));
        }
        assert(count==2);
        assert(!beam_bookmark_next("<a href=\"http://c.org/\"\n",url,sizeof(url),title,sizeof(title),&partial) && partial);
        assert(beam_bookmark_next("<a href=\"http://c.org/\"\n  title=x>Long\n title</a>",url,sizeof(url),title,sizeof(title),&partial));
        assert(!strcmp(url,"http://c.org/") && !strcmp(title,"Long title"));
        assert(beam_bookmark_next("Amiga Future: https://www.amigafuture.de/\n",url,sizeof(url),title,sizeof(title),&partial));
        assert(!strcmp(url,"https://www.amigafuture.de/") && !strcmp(title,"Amiga Future"));
        assert(beam_bookmark_next("URL=http://aminet.net/ NAME=x",url,sizeof(url),title,sizeof(title),&partial) &&
               !strcmp(url,"http://aminet.net/") && !strcmp(title,"URL"));
        assert(!beam_bookmark_next("no links here",url,sizeof(url),title,sizeof(title),&partial) && !partial);
    }
    assert(beam_snippet_parse("sig\tBest regards, Tom\n",name,sizeof(name),text,sizeof(text)));
    assert(!strcmp(name,"sig") && !strcmp(text,"Best regards, Tom"));
    assert(!beam_snippet_parse("# comment\tx",name,sizeof(name),text,sizeof(text)));
    assert(!beam_snippet_parse("bad:name\tx",name,sizeof(name),text,sizeof(text)));
    assert(beam_template_fill("Run >NIL: HippoPlayer \"{file}\"","{file}","Work:Mods/mod.x",cmd,sizeof(cmd)));
    assert(!strcmp(cmd,"Run >NIL: HippoPlayer \"Work:Mods/mod.x\""));
    assert(!beam_template_fill("x {url}","{url}","http://a/\"b",cmd,sizeof(cmd)));
    {
        struct { char title[4]; unsigned char guard[4]; } small;
        int partial;
        memset(&small,0xa5,sizeof(small));
        assert(beam_bookmark_next("ABC= = = = https://example.org/",url,sizeof(url),small.title,sizeof(small.title),&partial));
        assert(small.title[3]==0 && small.guard[0]==0xa5 && small.guard[3]==0xa5);
        assert(!beam_bookmark_next("https://example.org/long",url,8,title,sizeof(title),&partial));
        assert(!beam_template_fill("x",NULL,"value",cmd,sizeof(cmd)));
        assert(!beam_template_fill("x","","value",cmd,sizeof(cmd)));
    }
    puts("Index sources: modules, bookmarks, snippets, command templates: PASS");
    return 0;
}
