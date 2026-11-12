#include <exec/types.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

static int leap(int y)
{ return !(y%400) || (!(y%4) && y%100); }

static int valid_date(int y,int m,int d)
{
    static const int mdays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    int max;
    if (y<1600 || y>9999 || m<1 || m>12) return 0;
    max=mdays[m-1]+(m==2 && leap(y));
    return d>=1 && d<=max;
}

static int weekday(int y,int m,int d)
{
    static const int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};
    if (m<3) y--;
    return (y+y/4-y/100+y/400+t[m-1]+d)%7;
}

static int format_stamp(struct DateStamp *stamp,char *out)
{
    struct DateTime dt;
    char date[32],time[32],day[16];
    memset(&dt,0,sizeof(dt));
    dt.dat_Stamp=*stamp;dt.dat_Format=FORMAT_DEF;dt.dat_Flags=0;
    dt.dat_StrDay=day;dt.dat_StrDate=date;dt.dat_StrTime=time;
    if (!DateToStr(&dt)) return 0;
    sprintf(out,"%s %s %s",day,date,time);
    return 1;
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *s,*end;
    struct DateStamp stamp;
    long delta;
    int y,m,d,w;
    static const int days[]={PLMSG_DAY_SUNDAY,PLMSG_DAY_MONDAY,PLMSG_DAY_TUESDAY,
        PLMSG_DAY_WEDNESDAY,PLMSG_DAY_THURSDAY,PLMSG_DAY_FRIDAY,PLMSG_DAY_SATURDAY};
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);DateStamp(&stamp);
    if (!*s || beam_ascii_equal(s,"today")) {
        if (!format_stamp(&stamp,out)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DATE_FORMAT_FAILED));
    } else if (beam_ascii_prefix(s,"add:")) {
        delta=strtol(s+4,&end,10);
        if (*end || delta<-365000L || delta>365000L) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DATE_ADD_USAGE));
        stamp.ds_Days+=delta;
        if (!format_stamp(&stamp,out)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DATE_RANGE));
    } else if (beam_ascii_prefix(s,"weekday:")) {
        if (sscanf(s+8,"%d-%d-%d",&y,&m,&d)!=3 || !valid_date(y,m,d))
            return beam_plugin_error(argc,argv,plugin_str(PLMSG_DATE_WEEKDAY_USAGE));
        w=weekday(y,m,d);sprintf(out,plugin_str(PLMSG_DATE_WEEKDAY),y,m,d,plugin_str(days[w]));
    } else if (beam_ascii_equal(s,"timestamp")) {
        unsigned long amiga_seconds=(unsigned long)stamp.ds_Days*86400UL+
            (unsigned long)stamp.ds_Minute*60UL+(unsigned long)stamp.ds_Tick/50UL;
        sprintf(out,plugin_str(PLMSG_DATE_EPOCH),amiga_seconds);
    } else return beam_plugin_error(argc,argv,plugin_str(PLMSG_DATE_USAGE));
    return beam_plugin_text(argc,argv,out);
}
