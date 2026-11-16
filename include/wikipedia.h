#ifndef BEAM_WIKIPEDIA_H
#define BEAM_WIKIPEDIA_H
#include <stdio.h>
#define BEAM_WIKI_LIMIT 8
struct BeamWikiPage { unsigned long id;char title[121],extract[1401]; };
struct BeamWikiResults { int count;struct BeamWikiPage pages[BEAM_WIKI_LIMIT]; };
int beam_wiki_language(const char *language);
int beam_wiki_search_url(const char *language,const char *query,char *out,unsigned int cap);
int beam_wiki_article_url(const char *language,unsigned long id,char *out,unsigned int cap,int api);
/* Bounded streaming JSON, UTF-8 decoded for Latin-1 native screens. */
int beam_wiki_read(FILE *file,struct BeamWikiResults *results);
#endif
