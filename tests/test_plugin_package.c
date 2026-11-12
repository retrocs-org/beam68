#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_package.h"

static void fill(struct BeamPlugin *p)
{
    memset(p,0,sizeof(*p));
    strcpy(p->name,"Weather"); strcpy(p->id,"org.example.weather");
    strcpy(p->plugin_version,"1.2"); strcpy(p->min_beam68,"0.1");
    strcpy(p->command,"PLUGINDIR:Weather"); p->trigger='%';
}

static unsigned char arc[2048];

static unsigned long lha0(unsigned long at,const char *name,unsigned long packed,int level1,const char *dir)
{
    unsigned long n=strlen(name),base=22+n+2+(level1?3:0),i=at;
    memset(arc+at,0,base);
    arc[at]=(unsigned char)(base-2);
    memcpy(arc+at+2,"-lh5-",5);
    arc[at+20]=(unsigned char)(level1?1:0);
    arc[at+21]=(unsigned char)n;
    memcpy(arc+at+22,name,n);
    i=at+base;
    if (level1) {
        unsigned long dl=dir?strlen(dir):0,ext=dir?dl+3:0;
        packed+=ext;
        if (dir) {
            arc[at+base-2]=(unsigned char)ext; arc[at+base-1]=0;
            arc[i]=2; memcpy(arc+i+1,dir,dl); arc[i+1+dl]=0; arc[i+2+dl]=0; i+=ext;
            packed-=0;
        }
        packed-=0;
    }
    arc[at+7]=(unsigned char)packed; arc[at+8]=(unsigned char)(packed>>8);
    return i+(level1&&dir?packed-(strlen(dir)+3):packed);
}

static unsigned long lha2(unsigned long at,const char *dir,const char *file,unsigned long packed)
{
    unsigned long dl=strlen(dir),fl=strlen(file),i=at+26,size;
    memset(arc+at,0,26);
    memcpy(arc+at+2,"-lh5-",5);
    arc[at+7]=(unsigned char)packed; arc[at+20]=2;
    arc[at+24]=(unsigned char)(fl+3);
    arc[i]=1; memcpy(arc+i+1,file,fl); i+=fl+1; arc[i]=(unsigned char)(dl+3); arc[i+1]=0; i+=2;
    arc[i]=2; memcpy(arc+i+1,dir,dl); i+=dl+1; arc[i]=0; arc[i+1]=0; i+=2;
    size=i-at;
    arc[at]=(unsigned char)size; arc[at+1]=(unsigned char)(size>>8);
    return i+packed;
}

static int list_ok(unsigned long len,int expect)
{
    char name[256];
    unsigned long pos=0,skip;
    int count=0;
    for (;;) {
        long rc=beam_lha_header(arc+pos,len-pos,name,sizeof(name),&skip);
        if (rc==0) break;
        if (rc<0) return -1;
        if (!beam_package_zip_name_safe(name)) return -2;
        pos+=(unsigned long)rc+skip; count++;
        if (pos>len) return -1;
    }
    return count==expect;
}

int main(void)
{
    struct BeamPlugin p;
    char err[96],out[BEAM_MAX_PATH],spec[BEAM_MAX_PATH];
    /* PLUGINDIR: resolution */
    assert(beam_plugindir_resolve("Work:Beam68/Plugins/W/W.plugin","PLUGINDIR:W",out,sizeof(out)));
    assert(!strcmp(out,"Work:Beam68/Plugins/W/W"));
    assert(beam_plugindir_resolve("RAM:W.plugin","plugindir:bin/W",out,sizeof(out)));
    assert(!strcmp(out,"RAM:bin/W"));
    assert(!beam_plugindir_resolve("RAM:W.plugin","PLUGINDIR:/W",out,sizeof(out)));
    assert(!beam_plugindir_resolve("RAM:W.plugin","PLUGINDIR:a//W",out,sizeof(out)));
    assert(!beam_plugindir_resolve("RAM:W.plugin","PLUGINDIR:SYS:W",out,sizeof(out)));
    assert(beam_plugindir_resolve("RAM:W.plugin","PROGDIR:Beam68",out,sizeof(out)));
    assert(!strcmp(out,"PROGDIR:Beam68"));
    /* versions */
    assert(beam_version_compare("0.1","0.1")==0);
    assert(beam_version_compare("0.10","0.9")>0);
    assert(beam_version_compare("1","1.0.0")==0);
    assert(beam_version_compare("1.2.1","1.2")>0);
    assert(beam_version_compare("x","0.1")<0);
    assert(!beam_version_valid("1..2") && !beam_version_valid(".1") && !beam_version_valid("1."));
    /* flat 0.1 layout migration */
    strcpy(spec,"PROGDIR:Plugins/Calc.plugin");
    beam_plugin_path_migrate(spec,sizeof(spec));
    assert(!strcmp(spec,"PROGDIR:Plugins/Calc/Calc.plugin"));
    beam_plugin_path_migrate(spec,sizeof(spec));
    assert(!strcmp(spec,"PROGDIR:Plugins/Calc/Calc.plugin"));
    strcpy(spec,"Work:Mine/Other.plugin");
    beam_plugin_path_migrate(spec,sizeof(spec));
    assert(!strcmp(spec,"Work:Mine/Other.plugin"));
    /* archive kinds and names */
    assert(beam_package_archive_kind("RAM:Beam68-Plugin-W.LHA")==BEAM_ARCHIVE_LHA);
    assert(beam_package_archive_kind("RAM:w.lzh")==BEAM_ARCHIVE_LHA);
    assert(beam_package_archive_kind("RAM:w.zip")==BEAM_ARCHIVE_ZIP);
    assert(beam_package_archive_kind("RAM:w.plugin")==BEAM_ARCHIVE_NONE);
    assert(beam_package_zip_name_safe("Weather/Weather.plugin"));
    assert(beam_package_zip_name_safe("Weather/"));
    assert(!beam_package_zip_name_safe("/Weather"));
    assert(!beam_package_zip_name_safe("../evil"));
    assert(!beam_package_zip_name_safe("Weather/../../S/Startup-Sequence"));
    assert(!beam_package_zip_name_safe("SYS:S/Startup-Sequence"));
    assert(!beam_package_zip_name_safe("a//b"));
    assert(beam_package_lha_line_safe("   1234   2048 60.2% 27-Oct-26 10:12:00 Weather/Weather"));
    assert(!beam_package_lha_line_safe("   1234   2048 60.2% 27-Oct-26 10:12:00 ../S/x"));
    assert(!beam_package_lha_line_safe("   1234   2048 60.2% 27-Oct-26 10:12:00 /x"));
    assert(beam_package_drawer_name_valid("Weather") && !beam_package_drawer_name_valid("../x") &&
           !beam_package_drawer_name_valid("A B"));
    /* Only the first public protocol is accepted. */
    {
        FILE *f=fopen("build/testsettings/RAM/v2.plugin","wb");
        struct BeamPlugin q;
        if (!f) { assert(system("mkdir -p build/testsettings/RAM")==0); f=fopen("build/testsettings/RAM/v2.plugin","wb"); }
        assert(f);
        fputs("VERSION=2\nNAME=V2\nTRIGGER=+\nCOMMAND=PLUGINDIR:V2\n",f); fclose(f);
        assert(!beam_plugin_read(&q,"RAM:v2.plugin"));
        f=fopen("build/testsettings/RAM/v2.plugin","wb");
        fputs("VERSION=3\nNAME=V3\nTRIGGER=+\nCOMMAND=PLUGINDIR:V3\n",f); fclose(f);
        assert(!beam_plugin_read(&q,"RAM:v2.plugin"));
    }
    /* manifest rules */
    fill(&p); assert(beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); p.id[0]=0; assert(!beam_package_manifest_check(&p,err,sizeof(err)) && err[0]);
    fill(&p); strcpy(p.id,"bad id"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); p.plugin_version[0]=0; assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.min_beam68,"99.0"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.command,"PROGDIR:Plugins/Weather"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.command,"PLUGINDIR:/Weather"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.icon,"PLUGINDIR:Weather"); assert(beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.icon,"SYS:Disk"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.icon,"PROGDIR:Beam68"); assert(beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.settings,"ENVARC:Beam68/Plugins/Weather.cfg"); assert(beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.guide,"PLUGINDIR:Weather.guide"); assert(beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.guide,"SYS:Help/x.guide"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.settings,"ENVARC:Beam68/Plugins/../x.cfg"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.settings,"S:Startup-Sequence"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    /* LHA headers: level 0 with '\\' paths, level 1 with a directory
       extension, level 2; unsafe names are caught by the same rule as ZIP. */
    {
        unsigned long end;
        char name[256]; unsigned long skip;
        memset(arc,0,sizeof(arc));
        end=lha0(0,"Weather\\Weather.plugin",10,0,NULL);
        end=lha0(end,"Weather",20,1,"Weather\xff");
        end=lha2(end,"Weather\xff","Weather.info",30);
        arc[end]=0;
        assert(list_ok(end+1,3)==1);
        assert(beam_lha_header(arc,end+1,name,sizeof(name),&skip)>0 &&
               !strcmp(name,"Weather/Weather.plugin") && skip==10);
        memset(arc,0,sizeof(arc));
        end=lha0(0,"../S/Startup-Sequence",4,0,NULL); arc[end]=0;
        assert(list_ok(end+1,1)==-2);
        memset(arc,0,sizeof(arc));
        end=lha2(0,"\xff","evil",4); arc[end]=0;
        assert(list_ok(end+1,1)==-2);
        memset(arc,0,sizeof(arc));
        memcpy(arc,"\x10\x00-xx5-",7);
        assert(beam_lha_header(arc,64,name,sizeof(name),&skip)==-1);
        assert(beam_lha_header(arc,5,name,sizeof(name),&skip)==-2);
    }
    assert(beam_version_compare("4294967296","2")>0);
    assert(beam_version_compare("999999999999999","999999999999998")>0);
    assert(beam_version_compare("000000000000001","1.0.0")==0);
    fill(&p); strcpy(p.plugin_version,"invalid"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    fill(&p); strcpy(p.min_beam68,"invalid"); assert(!beam_package_manifest_check(&p,err,sizeof(err)));
    assert(!beam_package_zip_name_safe("Weather/bad\nname"));
    assert(!beam_package_zip_name_safe("Weather/bad*name"));
    assert(!beam_package_zip_name_safe("Weather/bad`name"));
    {
        unsigned char zip[128]; char name[64]; struct BeamZipEntry entry;
        memset(zip,0,sizeof(zip)); memcpy(zip,"PK\001\002",4);
        zip[28]=14; memcpy(zip+46,"Weather/plugin",14);
        assert(beam_zip_header(zip,sizeof(zip),name,sizeof(name),&entry)==60);
        zip[41]=0xa0; /* Unix symlink mode 0120000 */
        assert(beam_zip_header(zip,sizeof(zip),name,sizeof(name),&entry)==-1);
        zip[41]=0; zip[8]=1;
        assert(beam_zip_header(zip,sizeof(zip),name,sizeof(name),&entry)==-1);
        zip[8]=0; zip[27]=2; /* >16 MiB uncompressed */
        assert(beam_zip_header(zip,sizeof(zip),name,sizeof(name),&entry)==-1);
        zip[27]=0; zip[46]='\n';
        assert(beam_zip_header(zip,sizeof(zip),name,sizeof(name),&entry)==-1);
    }
    puts("Plugin packages: PLUGINDIR:, versions, migration, archive names, LHA headers, manifest rules: PASS");
    return 0;
}
