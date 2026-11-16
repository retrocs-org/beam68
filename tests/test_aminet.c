#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "aminet.h"
static FILE *contents(const char *s)
{ FILE *f=tmpfile();assert(f);assert(fputs(s,f)>=0);rewind(f);return f; }
static void row(FILE *f,int n,const char *package)
{
    fprintf(f,"<tr class=\"lightrow pkg_row\"><td class=\"name_col\"><a href=\"/%s\">package%d</a></td><td><a href=\"/package/util/misc/pkg%d\">A &amp; B &#60;test&#62; &#27; %d</a></td></tr>\n",package,n,n,n);
}
int main(void)
{
    char out[400],package[100];long length;FILE *f;struct BeamAminetResults r;struct BeamAminetDetails d;int i;
    assert(beam_aminet_percent(0,0)==-1);
    assert(beam_aminet_percent(0,100)==0 && beam_aminet_percent(1,3)==33);
    assert(beam_aminet_percent(2,3)==66 && beam_aminet_percent(3,3)==100);
    assert(beam_aminet_percent(67108863UL,67108864UL)==99);
    assert(beam_aminet_percent(2147483647UL,4294967295UL)==49);
    assert(beam_aminet_percent(4294967295UL,4294967295UL)==100);
    assert(beam_aminet_path_valid("util/wb/Beam68-0.1.lha"));
    assert(beam_aminet_path_valid("mods/pro/a+b_1992.mod"));
    assert(!beam_aminet_path_valid("../wb/package.lha"));
    assert(!beam_aminet_path_valid("util/wb/../evil.lha"));
    assert(!beam_aminet_path_valid("/util/wb/package.lha"));
    assert(!beam_aminet_path_valid("util//package.lha"));
    assert(!beam_aminet_path_valid("util/wb/"));
    assert(!beam_aminet_path_valid("util/wb/package.lha\n"));
    assert(!beam_aminet_path_valid("util/wb/a*\".lha"));
    assert(!beam_aminet_path_valid("util/wb/a%2f..lha"));
    memset(out,'a',sizeof(out));out[sizeof(out)-1]=0;assert(!beam_aminet_path_valid(out));
    assert(beam_aminet_encode_query("demo & game+1992",out,sizeof(out)));
    assert(!strcmp(out,"demo%20%26%20game%2B1992"));
    assert(!beam_aminet_encode_query("a",out,sizeof(out)));
    assert(!beam_aminet_encode_query("ab\r\nInjected",out,sizeof(out)));
    assert(!beam_aminet_encode_query("hello",out,3));
    assert(beam_aminet_readme_path("util/misc/package.tar.gz",out,sizeof(out)));
    assert(!strcmp(out,"/util/misc/package.readme"));
    assert(beam_aminet_readme_path("util/wb/Beam68-0.1.lha",out,sizeof(out)));
    assert(!strcmp(out,"/util/wb/Beam68-0.1.readme"));
    assert(!beam_aminet_readme_path("../wb/evil.lha",out,sizeof(out)));
    beam_aminet_plain(" <b>Hello</b> &amp; &#10;there \033",out,sizeof(out));
    assert(!strcmp(out,"Hello & ?there"));
    f=tmpfile();assert(f);
    fputs("<table>\n<tr><a class=\"sort_link_name\">Name</a></tr>\n",f);
    row(f,0,"util/wb/../../evil.lha");
    for(i=0;i<50;i++) { snprintf(package,sizeof(package),"util/misc/package%d.lha",i);row(f,i,package); }
    fputs("</table>",f);rewind(f);
    assert(beam_aminet_search(f,0,&r));assert(r.count==6 && r.seen==50);
    assert(!strcmp(r.rows[0].path,"util/misc/package0.lha"));
    assert(strstr(r.rows[0].title,"A & B <test> ? 0"));
    rewind(f);assert(beam_aminet_search(f,48,&r));assert(r.count==2 && r.seen==50);
    assert(!strcmp(r.rows[0].path,"util/misc/package48.lha"));fclose(f);
    f=contents("Found 0 matching packages<br>");assert(beam_aminet_search(f,0,&r));assert(!r.count);fclose(f);
    f=contents("<html>Server failure</html>");assert(!beam_aminet_search(f,0,&r));fclose(f);
    f=contents("<tr class=\"pkg_row\"><td class=\"name_col\"><a href=\"/util/misc/x.lha\">x");assert(!beam_aminet_search(f,0,&r));fclose(f);
    f=contents("Short: Test package\nVersion: 1.2\nAuthor: Example\nArchitecture: m68k-amigaos >= 3.0\nRequires: 68020\n\nLong body\n");
    assert(beam_aminet_details(f,&d));assert(!strcmp(d.summary,"Test package"));assert(!strcmp(d.version,"1.2"));assert(!strcmp(d.requires,"68020"));fclose(f);
    f=contents("<html>not a readme</html>\n");assert(!beam_aminet_details(f,&d));fclose(f);
    assert(beam_aminet_http_header("HTTP/1.0 200 OK\r\nContent-Length: 123\r\n\r\n",1000,&length) && length==123);
    assert(beam_aminet_http_header("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n",1000,&length) && length==-1);
    assert(beam_aminet_http_header("HTTP/1.1 200 OK\r\ncontent-length: 1000\r\nContent-Encoding: identity\r\n\r\n",1000,&length) && length==1000);
    assert(!beam_aminet_http_header("HTTP/1.1 404 Missing\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 301 Moved\r\nLocation: http://evil/\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 1001\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 9999999999999999999\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5abc\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n\r\n",1000,&length));
    assert(!beam_aminet_http_header("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n",1000,&length));
    assert(beam_aminet_archive_header("util/misc/a.zip",(const unsigned char *)"PK\003\004",4));
    assert(!beam_aminet_archive_header("util/misc/a.zip",(const unsigned char *)"<html>",6));
    assert(beam_aminet_archive_header("util/misc/a.lha",(const unsigned char *)"xx-lh5-",7));
    assert(!beam_aminet_archive_header("util/misc/a.lha",(const unsigned char *)"xx-lh5-",6));
    assert(beam_aminet_archive_header("util/misc/a.tgz",(const unsigned char *)"\037\213\010",3));
    assert(!beam_aminet_archive_header("util/misc/a.run",(const unsigned char *)"xx-lh5-",7));
    puts("Aminet paths, URL encoding, bounded HTML/readme parsing, paging and HTTP headers: PASS");return 0;
}
