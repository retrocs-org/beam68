#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack_guard.h"
#include <stdio.h>
#include <string.h>
#include "aminet.h"
#include "aminet_http.h"
static struct BeamAminetResults results;
static struct BeamAminetDetails details;
static char error[121],remote[200],local[240],root[100];
static const char *local_file(const char *name)
{ strcpy(local,root);AddPart(local,(STRPTR)name,sizeof(local));return local; }
static int cancel(unsigned long bytes) { (void)bytes;return 0; }
#define CHECK(x) do { if(!(x)) { printf("FAIL line %d: %s; %s\n",__LINE__,#x,error);return 20; } } while(0)
static int run(int argc,char **argv) {
 FILE *f;char package[97];int i;BPTR lock=0;
 (void)argc;(void)argv;
 for(i=0;i<100;i++) { sprintf(root,"T:Beam68AminetQA%02d",i);lock=CreateDir(root);if(lock)break; }
 CHECK(lock);UnLock(lock);printf("Isolated native Aminet checks: %s\n",root);

 CHECK(beam_aminet_fetch("/search?query=beam68",local_file("search.html"),262144UL,20,0,NULL,error,sizeof(error)));
 f=fopen(local_file("search.html"),"r");CHECK(f);CHECK(beam_aminet_search(f,0,&results));fclose(f);
 CHECK(results.count>0);strcpy(package,results.rows[0].path);puts("PASS: live native HTTP search and inline package parser");
 CHECK(beam_aminet_readme_path(package,remote,sizeof(remote)));
 CHECK(beam_aminet_fetch(remote,local_file("readme.txt"),262144UL,20,0,NULL,error,sizeof(error)));
 f=fopen(local_file("readme.txt"),"r");CHECK(f);CHECK(beam_aminet_details(f,&details));fclose(f);printf("PASS: live native readme: %s\n",details.summary);
 sprintf(remote,"/%s",package);
 CHECK(beam_aminet_fetch(remote,local_file("archive.part"),67108864UL,90,1,NULL,error,sizeof(error)));puts("PASS: live native archive download (not executed)");
 CHECK(!beam_aminet_fetch("/search?query=beam68",local_file("too-small.part"),1,20,0,NULL,error,sizeof(error)));
 f=fopen(local_file("too-small.part"),"r");CHECK(!f);puts("PASS: size limit rejects body and removes partial file");
 CHECK(!beam_aminet_fetch("/search?query=beam68",local_file("cancel.part"),262144UL,20,0,cancel,error,sizeof(error)));
 f=fopen(local_file("cancel.part"),"r");CHECK(!f);puts("PASS: cancellation before transfer leaves no partial file");
 puts("Aminet Browser native network checks: PASS");return 0;
}

int main(int argc,char **argv)
{ return beam_run_with_stack(32768UL,run,argc,argv); }
