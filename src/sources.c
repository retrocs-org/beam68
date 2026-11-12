#include <string.h>
#include "beam.h"
#include "sources.h"

static int lower(int c) { return (c>='A' && c<='Z')?c+32:c; }

static int prefix_ci(const char *s,const char *p)
{
    while (*p) if (lower((unsigned char)*s++)!=*p++) return 0;
    return 1;
}

int beam_module_name(const char *name)
{
    static const char *const pre[]={"mod.","med.","p61.","ahx.","thx.","xm.","s3m.","it.","dbm.","okt.",NULL};
    static const char *const suf[]={".mod",".med",".mmd0",".mmd1",".mmd2",".mmd3",".xm",".s3m",
                                    ".it",".ahx",".thx",".p61",".dbm",".okt",".sid",NULL};
    unsigned int n,i,m;
    if (!name || !*name) return 0;
    n=(unsigned int)strlen(name);
    for (i=0;pre[i];i++) if (n>strlen(pre[i]) && prefix_ci(name,pre[i])) return 1;
    for (i=0;suf[i];i++) {
        m=(unsigned int)strlen(suf[i]);
        if (n>m && prefix_ci(name+n-m,suf[i])) return 1;
    }
    return 0;
}

static const char *find_ci(const char *s,const char *what)
{
    for (;*s;s++) if (prefix_ci(s,what)) return s;
    return NULL;
}

int beam_bookmark_parse(const char *line,char *url,unsigned int ucap,
                        char *title,unsigned int tcap)
{
    const char *a,*h,*q,*end,*t;
    unsigned int n=0,depth=0;
    char quote;
    if (!line || !url || !title || !ucap || !tcap) return 0;
    url[0]=0; title[0]=0;
    a=find_ci(line,"<a ");
    if (!a) return 0;
    h=find_ci(a,"href=");
    if (!h) return 0;
    h+=5;
    quote=(*h=='"' || *h=='\'')?*h++:0;
    for (q=h;*q && (quote?*q!=quote:(*q!=' ' && *q!='>'));q++) {
        if ((unsigned char)*q<32 || n+1>=ucap) return 0;
        url[n++]=*q;
    }
    url[n]=0;
    if (!prefix_ci(url,"http://") && !prefix_ci(url,"https://") &&
        !prefix_ci(url,"ftp://") && !prefix_ci(url,"gopher://") && !prefix_ci(url,"file://"))
        return 0;
    if (strchr(url,'"') || strchr(url,'*') || strchr(url,'`')) return 0;
    t=strchr(q,'>');
    if (!t) return 0;
    end=find_ci(t,"</a");
    if (!end) end=t+strlen(t);
    n=0;
    for (t=t+1;t<end;t++) {                 /* text, tags stripped */
        if (*t=='<') { depth=1; continue; }
        if (*t=='>') { depth=0; continue; }
        if (depth || (unsigned char)*t<32) continue;
        if (*t==' ' && (!n || title[n-1]==' ')) continue;
        if (n+1<tcap) title[n++]=*t;
    }
    while (n && title[n-1]==' ') --n;
    title[n]=0;
    if (!n) beam_copy(title,url,tcap);
    return 1;
}

static int url_ok(const char *u)
{
    return (prefix_ci(u,"http://") || prefix_ci(u,"https://") || prefix_ci(u,"ftp://") ||
            prefix_ci(u,"gopher://") || prefix_ci(u,"file://")) &&
           !strchr(u,'"') && !strchr(u,'*') && !strchr(u,'`');
}

const char *beam_bookmark_next(const char *text,char *url,unsigned int ucap,
                               char *title,unsigned int tcap,int *partial)
{
    const char *a,*close,*h,*p;
    if (partial) *partial=0;
    if (!text || !url || !title || !ucap || !tcap) return NULL;
    while (*text) {
        a=find_ci(text,"<a ");
        if (a) {
            close=find_ci(a,"</a");
            if (!close) {
                /* Anchor continues on a later line, unless it has no href
                   (then it is not ours and can be skipped). */
                if (find_ci(a,"href=") || !strchr(a,'>')) { if (partial) *partial=1; return NULL; }
                text=a+3; continue;
            }
            {   /* parse just this anchor */
                char one[1024];
                unsigned int n=(unsigned int)(close-a)+4;
                if (n>=sizeof(one)) { text=close+3; continue; }
                memcpy(one,a,n); one[n]=0;
                if (beam_bookmark_parse(one,url,ucap,title,tcap)) return close+3;
                text=close+3;
                continue;
            }
        }
        /* No HTML anchors left: bare URLs, e.g. plain-text hotlists. */
        h=find_ci(text,"http://");
        p=find_ci(text,"https://");
        if (!h || (p && p<h)) h=p;
        if (!h) return NULL;
        {
            unsigned int n=0,t=0,depth=0;
            const char *q,*s0=text;
            for (q=h;*q && *q!=' ' && *q!='\t' && *q!='"' && *q!='<' && *q!='>' &&
                     *q!='\n' && *q!='\r';q++)
                if (n+1<ucap) url[n++]=*q;
                else { url[0]=0; title[0]=0; return NULL; }
            url[n]=0;
            /* title: the text before the URL on this line, tags stripped */
            for (p=s0;p<h;p++) {
                if (*p=='<') { depth=1; continue; }
                if (*p=='>') { depth=0; continue; }
                if (depth || (unsigned char)*p<32 || *p=='|' || *p=='=') {
                    if (t && t+1<tcap && title[t-1]!=' ') title[t++]=' ';
                    continue;
                }
                if (*p==' ' && (!t || title[t-1]==' ')) continue;
                if (t+1<tcap) title[t++]=*p;
            }
            while (t && (title[t-1]==' ' || title[t-1]==':' || title[t-1]=='-')) --t;
            title[t]=0;
            if (!t) beam_copy(title,url,tcap);
            if (url_ok(url)) return q;
            text=q;
        }
    }
    return NULL;
}

int beam_snippet_parse(const char *line,char *name,unsigned int ncap,char *text,unsigned int tcap)
{
    const char *tab;
    unsigned int n,i;
    if (!line || *line=='#' || !(tab=strchr(line,'\t'))) return 0;
    n=(unsigned int)(tab-line);
    if (!n || n>=ncap) return 0;
    for (i=0;i<n;i++) if ((unsigned char)line[i]<32 || line[i]==':' || line[i]=='/') return 0;
    memcpy(name,line,n); name[n]=0;
    beam_copy(text,tab+1,tcap);
    n=(unsigned int)strlen(text);
    while (n && (text[n-1]=='\n' || text[n-1]=='\r')) text[--n]=0;
    return n>0;
}

int beam_template_fill(const char *t,const char *key,const char *value,char *out,unsigned int cap)
{
    unsigned int n=0,k,v;
    const char *s;
    if (!t || !key || !*key || !value || !*value || !out || !cap) return 0;
    k=(unsigned int)strlen(key);
    for (s=value;*s;s++) if ((unsigned char)*s<32 || *s=='"' || *s=='*' || *s=='`') return 0;
    v=(unsigned int)strlen(value);
    while (*t) {
        if (!strncmp(t,key,k)) {
            if (n+v+1>=cap) return 0;
            memcpy(out+n,value,v); n+=v; t+=k;
        } else {
            if (n+2>=cap) return 0;
            out[n++]=*t++;
        }
    }
    out[n]=0;
    return 1;
}
