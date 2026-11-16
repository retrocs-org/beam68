#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wikipedia.h"
static struct BeamWikiResults result;
static int parse(const char *text)
{FILE *file=tmpfile();int ok;assert(file);assert(fputs(text,file)>=0);rewind(file);ok=beam_wiki_read(file,&result);fclose(file);return ok;}
int main(void)
{
    char url[1024];FILE *file;int i;
    assert(beam_wiki_language("en") && beam_wiki_language("sv"));
    assert(!beam_wiki_language("en.evil") && !beam_wiki_language(NULL));
    assert(beam_wiki_search_url("sv","r\xe4ksm\xf6rg\xe5s",url,sizeof(url)));
    assert(strstr(url,"sv.wikipedia.org/") && strstr(url,"r%C3%A4ksm%C3%B6rg%C3%A5s"));
    assert(beam_wiki_search_url("en","a\"&b",url,sizeof(url)) && strstr(url,"a%22%26b"));
    assert(!beam_wiki_search_url("en","x\nattack",url,sizeof(url)));
    assert(!beam_wiki_search_url("en","word",url,20));
    assert(beam_wiki_article_url("en",123,url,sizeof(url),0) && !strcmp(url,"https://en.wikipedia.org/?curid=123"));
    assert(!beam_wiki_article_url("en",0,url,sizeof(url),1));
    assert(parse(" {\"batchcomplete\":true,\"query\":{\"search\":[{\"ns\":0,\"title\":\"Amiga\",\"pageid\":123,\"size\":10},{\"pageid\":456,\"title\":\"Commodore\"}]}} "));
    assert(result.count==2 && result.pages[0].id==123 && !strcmp(result.pages[1].title,"Commodore"));
    assert(parse("{\"query\":{\"pages\":[{\"pageid\":123,\"title\":\"Caf\\u00e9\",\"extract\":\"Line one\\nLine two \\ud83d\\ude00\"}]}}"));
    assert(result.count==1 && !strcmp(result.pages[0].title,"Caf\xe9") && strstr(result.pages[0].extract,"Line one\nLine two ?"));
    assert(parse("{\"query\":{\"search\":[{\"pageid\":1,\"title\":\"Caf\xc3\xa9\"},{\"pageid\":1,\"title\":\"Duplicate\"}]}}"));
    assert(result.count==1 && !strcmp(result.pages[0].title,"Caf\xe9"));
    assert(parse("{\"query\":{\"search\":[]}}") && !result.count);
    assert(!parse("{\"error\":{\"code\":\"ratelimited\"}}"));
    assert(!parse("{\"query\":{\"search\":[{\"pageid\":4294967296,\"title\":\"bad\"}]}}"));
    assert(!parse("{\"query\":{\"search\":[{\"pageid\":01,\"title\":\"bad\"}]}}"));
    assert(!parse("{\"query\":{\"search\":[{\"pageid\":1,\"title\":\"\\ud800\"}]}}"));
    assert(!parse("{\"query\":{\"search\":[{\"pageid\":1,\"title\":\"\xc0\xaf\"}]}}"));
    assert(!parse("{\"query\":{\"search\":["));
    assert(!parse("{} trailing"));
    file=tmpfile();assert(file);fputs("{\"query\":{\"search\":[",file);
    for(i=1;i<=12;i++)fprintf(file,"%s{\"pageid\":%d,\"title\":\"Page %d\"}",i==1?"":",",i,i);
    fputs("]}}",file);rewind(file);assert(beam_wiki_read(file,&result) && result.count==8);fclose(file);
    file=tmpfile();assert(file);for(i=0;i<20;i++)fputs("{\"x\":",file);fputs("0",file);for(i=0;i<20;i++)fputs("}",file);
    rewind(file);assert(!beam_wiki_read(file,&result));fclose(file);
    file=tmpfile();assert(file);fputs("{\"x\":\"",file);for(i=0;i<66000;i++)fputc('a',file);fputs("\"}",file);
    rewind(file);assert(!beam_wiki_read(file,&result));fclose(file);
    puts("Wikipedia: bounded JSON, search and summaries, UTF-8/Unicode, safe URLs, limits and malformed responses: PASS");return 0;
}
