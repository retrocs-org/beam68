/* Disposable-guest receiver: validates real WBStartup arguments, no GUI. */
#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/startup.h>
#include <proto/dos.h>
#include <string.h>
int main(int argc,char **argv)
{
    struct WBStartup *startup=(struct WBStartup *)argv;
    BPTR output;char path[300];int valid=argc==0 && startup && startup->sm_NumArgs==2;
    if(valid)valid=NameFromLock(startup->sm_ArgList[1].wa_Lock,path,sizeof(path)) &&
        AddPart(path,startup->sm_ArgList[1].wa_Name,sizeof(path));
    output=Open("T:Beam68OpenWithQA.txt",MODE_NEWFILE);if(!output)return 20;
    FPuts(output,valid?"PASS\n":"FAIL\n");if(valid){FPuts(output,path);FPuts(output,"\n");}
    Close(output);return valid?0:20;
}
