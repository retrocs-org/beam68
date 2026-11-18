/* Native fixture transfers: no network, external installer or viewer launches. */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#define BEAM_AMINET_QA
#define main plugin_main
#include "../../Plugins/AminetBrowser/AminetBrowser.c"
#undef main
static char qa_root[160],qa_dir[200];
static int fetches,opens,choices,mode,retries,fail_first;
int beam_aminet_qa_ask(const struct EasyStruct *p)
{
 if(!strcmp(p->es_GadgetFormat,"Download|Cancel"))return mode==3?0:1;
 if(!strcmp(p->es_GadgetFormat,"Retry|Close")){++retries;return mode==5?0:1;}
 if(strstr(p->es_GadgetFormat,"Open drawer")){++choices;return choices<4?choices:0;}
 return 0;
}
int beam_aminet_qa_open(const char *path)
{if(!path||!*path)return 0;++opens;return 1;}
int beam_aminet_fetch(const char *remote,const char *destination,unsigned long limit,unsigned long seconds,int length,int (*callback)(unsigned long,unsigned long),char *error,unsigned int cap)
{
 FILE *file;(void)limit;(void)seconds;(void)length;(void)callback;++fetches;
 file=fopen(destination,"wb");if(!file)return 0;
 if(strstr(remote,".readme"))fputs("Short: Fixture package\nInstallation instructions.\n",file);
 else fputs(mode==4?"<html>":"xx-lh5-",file);
 fclose(file);
 if(mode==5){char existing[240];strcpy(existing,qa_dir);AddPart(existing,"Fixture.lha",sizeof(existing));file=fopen(existing,"wb");if(file){fputs("KEEP",file);fclose(file);}}
 if(mode==2){cancelled=1;return 0;}
 if(fail_first && fetches==1){beam_plugin_copy(error,"Fixture interruption",cap);return 0;}
 return 1;
}
static int settings(const char *child)
{
 BPTR dir;FILE *f;
 strcpy(qa_dir,qa_root);AddPart(qa_dir,(STRPTR)child,sizeof(qa_dir));dir=CreateDir(qa_dir);if(!dir)return 0;UnLock(dir);
 dir=CreateDir("ENVARC:Beam68");if(dir)UnLock(dir);dir=CreateDir("ENVARC:Beam68/Plugins");if(dir)UnLock(dir);
 f=fopen("ENVARC:Beam68/Plugins/AminetBrowser.cfg","wb");if(!f)return 0;
 fprintf(f,"DOWNLOAD_DIR=%s\n",qa_dir);if(fclose(f))return 0;
 dir=CreateDir("ENV:Beam68");if(dir)UnLock(dir);dir=CreateDir("ENV:Beam68/Plugins");if(dir)UnLock(dir);
 f=fopen("ENV:Beam68/Plugins/AminetBrowser.cfg","wb");if(!f)return 0;
 fprintf(f,"DOWNLOAD_DIR=%s/success\n",qa_root);return fclose(f)==0;
}
static int clean_count(void)
{
 BPTR lock=Lock(qa_dir,ACCESS_READ);struct FileInfoBlock *fib;int count=0;
 if(!lock)return -1;
 fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return -1;}
 if(Examine(lock,fib))while(ExNext(lock,fib)){if(fib->fib_DirEntryType>0)count+=100;else ++count;}
 FreeDosObject(DOS_FIB,fib);UnLock(lock);return count;
}
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 20;}}while(0)
static int qa_run(int argc,char **argv)
{
 (void)argc;(void)argv;
 BPTR dir;int i;FILE *f;char target[240],data[8];
 for(i=0;i<100;++i){sprintf(qa_root,"T:Beam68CompletionQA%02d",i);dir=CreateDir(qa_root);if(dir)break;}
 CHECK(dir!=0);UnLock(dir);
 CHECK(settings("success"));fail_first=1;
 CHECK(download(0,NULL,"util/wb/Fixture.lha",0)==0);
 CHECK(fetches==3&&retries==1&&opens==3&&choices==4&&clean_count()==2);
 strcpy(target,qa_dir);AddPart(target,"Fixture.lha",sizeof(target));f=fopen(target,"rb");CHECK(f!=NULL);CHECK(fread(data,1,7,f)==7&&!memcmp(data,"xx-lh5-",7));fclose(f);
 puts("PASS: interrupted transfer retry, commit, completion actions, readme and staging cleanup.");
 mode=2;fetches=opens=choices=retries=0;fail_first=0;CHECK(settings("cancel"));
 {int rc=download(0,NULL,"util/wb/Fixture.lha",0);printf("cancel diagnostics: rc=%d fetches=%d opens=%d choices=%d files=%d error=%s\n",rc,fetches,opens,choices,clean_count(),error_text);CHECK(rc==0&&fetches==1&&opens==0&&choices==0&&clean_count()==0);}
 puts("PASS: cancellation removes partial data and staging drawer.");
 mode=3;fetches=0;CHECK(settings("consent"));CHECK(download(0,NULL,"util/wb/Fixture.lha",0)==0&&fetches==0&&clean_count()==0);
 puts("PASS: cancelled initial consent never transfers or opens files.");
 mode=4;fetches=opens=choices=retries=0;CHECK(settings("invalid"));
 CHECK(download(0,NULL,"util/wb/Fixture.lha",0)==0&&opens==2&&choices==4&&clean_count()==2);
 puts("PASS: malformed archive cannot invoke the Install completion action.");
 mode=5;fetches=opens=choices=retries=0;CHECK(settings("commit-race"));
 CHECK(download(0,NULL,"util/wb/Fixture.lha",0)==0&&opens==0&&retries==1&&clean_count()==1);
 strcpy(target,qa_dir);AddPart(target,"Fixture.lha",sizeof(target));f=fopen(target,"rb");CHECK(f!=NULL);CHECK(fread(data,1,4,f)==4&&!memcmp(data,"KEEP",4));fclose(f);
 puts("PASS: file created during transfer is preserved when archive commit fails.");return 0;
}

int main(int argc,char **argv){return beam_run_with_stack(65536UL,qa_run,argc,argv);}
