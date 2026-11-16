#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wikipedia.h"
int beam_wiki_language(const char *s)
{
    static const char *languages[]={"en","sv","de","da","fi","no","fr","es","it","nl","pt","pl"};
    unsigned int i;if(!s)return 0;
    for(i=0;i<sizeof(languages)/sizeof(languages[0]);i++)if(!strcmp(s,languages[i]))return 1;
    return 0;
}
static int append(char *out,unsigned int cap,const char *s)
{
    unsigned int n=strlen(out),m=strlen(s);if(n+m>=cap)return 0;memcpy(out+n,s,m+1);return 1;
}
static int encoded(char *out,unsigned int cap,unsigned int c)
{
    static const char hex[]="0123456789ABCDEF";char v[4];
    if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-' || c=='_' || c=='.' || c=='~') {
        v[0]=(char)c;v[1]=0;
    } else {v[0]='%';v[1]=hex[c>>4];v[2]=hex[c&15];v[3]=0;}
    return append(out,cap,v);
}
int beam_wiki_search_url(const char *language,const char *query,char *out,unsigned int cap)
{
    const unsigned char *p=(const unsigned char *)query;
    if(!beam_wiki_language(language) || !query || !*query || strlen(query)>120 || !out || cap<240)return 0;
    sprintf(out,"https://%s.wikipedia.org/w/api.php?action=query&list=search&srprop=&srlimit=8&format=json&formatversion=2&srsearch=",language);
    while(*p) {
        if(*p<32 || *p==127)return 0;
        if(*p>=128 && !encoded(out,cap,0xc0|(*p>>6)))return 0;
        if(!encoded(out,cap,*p>=128?(0x80|(*p&63)):*p))return 0;
        p++;
    }
    return 1;
}
int beam_wiki_article_url(const char *language,unsigned long id,char *out,unsigned int cap,int api)
{
    if(!beam_wiki_language(language) || !id || id>0xffffffffUL || !out || cap<240)return 0;
    if(api)sprintf(out,"https://%s.wikipedia.org/w/api.php?action=query&prop=extracts&pageids=%lu&exintro=1&explaintext=1&exchars=1200&format=json&formatversion=2",language,id);
    else sprintf(out,"https://%s.wikipedia.org/?curid=%lu",language,id);
    return 1;
}
struct Parser { FILE *file;int c,bad,api_error;unsigned long bytes;struct BeamWikiResults *result; };
static struct Parser parser;
static void next(void)
{parser.c=fgetc(parser.file);if(++parser.bytes>65536UL)parser.bad=1;}
static void space(void){while(parser.c==' ' || parser.c=='\r' || parser.c=='\n' || parser.c=='\t')next();}
static int hex4(unsigned long *value)
{
    int i;unsigned long n=0;
    for(i=0;i<4;i++) {
        int h=parser.c;if(h>='0' && h<='9')h-='0';else if(h>='a' && h<='f')h=h-'a'+10;
        else if(h>='A' && h<='F')h=h-'A'+10;else return 0;
        n=n*16UL+(unsigned long)h;next();
    }
    *value=n;return 1;
}
static unsigned int native_char(unsigned long c)
{
    if(c==0x2018 || c==0x2019)return '\'';
    if(c==0x201c || c==0x201d)return '"';
    if(c==0x2013 || c==0x2014 || c==0x2022)return '-';
    if(c==0xa0)return ' ';
    if(c=='\n' || c=='\t')return (unsigned int)c;
    if(c<32 || c==127 || (c>=128 && c<160) || c>255)return '?';
    return (unsigned int)c;
}
static int string(char *out,unsigned int cap,int *truncated)
{
    unsigned int n=0;if(truncated)*truncated=0;if(out && cap)out[0]=0;
    if(parser.c!='"')return 0;
    next();
    while(parser.c!='"') {
        unsigned long cp;int c=parser.c;
        if(parser.bad || c<32 || c==EOF)return 0;
        next();cp=(unsigned long)c;
        if(c=='\\') {
            c=parser.c;next();
            if(c=='u') {
                if(!hex4(&cp))return 0;
                if(cp>=0xd800 && cp<=0xdbff) {
                    unsigned long low;
                    if(parser.c!='\\')return 0;
                    next();if(parser.c!='u')return 0;next();
                    if(!hex4(&low) || low<0xdc00 || low>0xdfff)return 0;
                    cp=0x10000UL+((cp-0xd800UL)<<10)+(low-0xdc00UL);
                } else if(cp>=0xdc00 && cp<=0xdfff)return 0;
            } else if(c=='n')cp='\n';else if(c=='r')cp='\n';else if(c=='t')cp='\t';
            else if(c=='b' || c=='f')cp=' ';
            else if(c=='"' || c=='/' || c=='\\')cp=(unsigned long)c;else return 0;
        } else if(c>=128) {
            int more,i;unsigned long minimum;
            if(c>=0xc2 && c<=0xdf){more=1;cp=(unsigned long)c&31UL;minimum=0x80;}
            else if(c>=0xe0 && c<=0xef){more=2;cp=(unsigned long)c&15UL;minimum=0x800;}
            else if(c>=0xf0 && c<=0xf4){more=3;cp=(unsigned long)c&7UL;minimum=0x10000;}
            else return 0;
            for(i=0;i<more;i++){if(parser.c<0x80 || parser.c>0xbf)return 0;cp=(cp<<6)|((unsigned long)parser.c&63UL);next();}
            if(cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff))return 0;
        }
        if(out && cap && n+1<cap){out[n++]=(char)native_char(cp);out[n]=0;}
        else if(truncated)*truncated=1;
    }
    next();return 1;
}
static int number(unsigned long *id)
{
    unsigned long n=0;int integer=1,overflow=0;
    if(parser.c=='-'){integer=0;next();}
    if(parser.c<'0' || parser.c>'9')return 0;
    if(parser.c=='0')next();
    else while(parser.c>='0' && parser.c<='9') {
        unsigned long d=(unsigned long)(parser.c-'0');if(n>(0xffffffffUL-d)/10UL)overflow=1;
        if(!overflow)n=n*10UL+d;
        next();
    }
    if(parser.c=='.') {integer=0;next();if(parser.c<'0' || parser.c>'9')return 0;while(parser.c>='0' && parser.c<='9')next();}
    if(parser.c=='e' || parser.c=='E') {
        integer=0;next();if(parser.c=='+' || parser.c=='-')next();
        if(parser.c<'0' || parser.c>'9')return 0;
        while(parser.c>='0' && parser.c<='9')next();
    }
    if(id){if(!integer || overflow || !n)return 0;*id=n;}
    return 1;
}
static int value(int depth,int scope);
static int object(int depth,int scope)
{
    char key[32];int index=-1,truncated,i;
    if(scope==2 && parser.result->count<BEAM_WIKI_LIMIT) {
        index=parser.result->count;memset(&parser.result->pages[index],0,sizeof(struct BeamWikiPage));
    }
    next();space();if(parser.c=='}'){next();return 1;}
    for(;;) {
        if(!string(key,sizeof(key),&truncated))return 0;
        space();if(parser.c!=':')return 0;next();space();
        if(scope==0 && !truncated && !strcmp(key,"error"))parser.api_error=1;
        if(index>=0 && !truncated && !strcmp(key,"pageid")) {
            if(!number(&parser.result->pages[index].id))return 0;
        } else if(index>=0 && !truncated && !strcmp(key,"title")) {
            if(!string(parser.result->pages[index].title,sizeof(parser.result->pages[index].title),NULL))return 0;
        } else if(index>=0 && !truncated && !strcmp(key,"extract")) {
            if(!string(parser.result->pages[index].extract,sizeof(parser.result->pages[index].extract),NULL))return 0;
        } else if(!value(depth+1,!truncated && scope==0 && !strcmp(key,"query")?1:
            !truncated && scope==1 && (!strcmp(key,"search") || !strcmp(key,"pages"))?3:-1))return 0;
        space();if(parser.c=='}'){next();break;}if(parser.c!=',')return 0;next();space();
    }
    if(index>=0 && parser.result->pages[index].id && parser.result->pages[index].title[0]) {
        for(i=0;i<index;i++)if(parser.result->pages[i].id==parser.result->pages[index].id)return 1;
        for(i=0;parser.result->pages[index].title[i];i++)if((unsigned char)parser.result->pages[index].title[i]<32)parser.result->pages[index].title[i]=' ';
        parser.result->count++;
    }
    return !parser.bad;
}
static int value(int depth,int scope)
{
    const char *literal=NULL;
    if(depth>16 || parser.bad)return 0;
    space();
    if(parser.c=='{')return object(depth,scope);
    if(parser.c=='[') {
        next();space();if(parser.c==']'){next();return 1;}
        for(;;){if(!value(depth+1,scope==3?2:-1))return 0;space();if(parser.c==']'){next();return 1;}if(parser.c!=',')return 0;next();}
    }
    if(parser.c=='"')return string(NULL,0,NULL);
    if(parser.c=='-' || (parser.c>='0' && parser.c<='9'))return number(NULL);
    if(parser.c=='t')literal="true";else if(parser.c=='f')literal="false";else if(parser.c=='n')literal="null";
    if(!literal)return 0;
    while(*literal){if(parser.c!=*literal++)return 0;next();}return 1;
}
int beam_wiki_read(FILE *file,struct BeamWikiResults *results)
{
    int ok;if(!file || !results)return 0;
    memset(results,0,sizeof(*results));memset(&parser,0,sizeof(parser));parser.file=file;parser.result=results;
    next();space();if(parser.c!='{')return 0;ok=value(0,0);space();
    return ok && !parser.bad && !parser.api_error && parser.c==EOF;
}
