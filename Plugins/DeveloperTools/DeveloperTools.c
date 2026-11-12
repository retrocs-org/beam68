#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "plugin_protocol.h"
#include "plugin_common.h"
#include "plugin_locale.h"

static int number(const char *s,unsigned long *value)
{
    char *end;
    int base=10;
    if (*s=='$') { s++;base=16; }
    else if (s[0]=='0' && (s[1]=='x'||s[1]=='X')) { s+=2;base=16; }
    if (!*s) return 0;
    *value=strtoul(s,&end,base);
    return *end==0;
}

int main(int argc,char **argv)
{
    char input[BEAM_PLUGIN_MAX_INPUT+1],out[121];
    char *s,*p;
    unsigned long v,mask=0;
    if (!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if (beam_ascii_prefix(s,"hex:")) {
        if (!number(beam_plugin_trim(s+4),&v)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_HEX_USAGE));
        sprintf(out,"%lu = $%lX = 0x%lX",v,v,v);
    } else if (beam_ascii_prefix(s,"dec:")) {
        if (!number(beam_plugin_trim(s+4),&v)) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_DEC_USAGE));
        sprintf(out,"$%lX = %lu",v,v);
    } else if (beam_ascii_prefix(s,"ascii:")) {
        if (!number(beam_plugin_trim(s+6),&v) || v>255) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_ASCII_RANGE));
        if (v>=32 && v<127) sprintf(out,"ASCII %lu = '%c' = $%02lX",v,(int)v,v);
        else sprintf(out,plugin_str(PLMSG_DEV_ASCII_CONTROL),v,v);
    } else if (beam_ascii_prefix(s,"char:") && s[5]) {
        v=(unsigned char)s[5];sprintf(out,"'%c' = ASCII %lu = $%02lX",s[5],v,v);
    } else if (beam_ascii_prefix(s,"mask:")) {
        p=s+5;
        while (*p) {
            char *end;
            unsigned long bit=strtoul(p,&end,10);
            if (end==p || bit>31) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_MASK_USAGE));
            mask|=1UL<<bit;p=end;
            if (*p==',') p++;
            else if (*p) return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_MASK_SEPARATE));
        }
        sprintf(out,plugin_str(PLMSG_DEV_MASK),mask,mask);
    } else if (beam_ascii_prefix(s,"prot:")) {
        p=beam_plugin_trim(s+5);
        if (number(p,&v)) {
            sprintf(out,plugin_str(PLMSG_DEV_PROT),v,
                (v&8)?'-':'+',(v&4)?'-':'+',(v&2)?'-':'+',(v&1)?'-':'+');
        } else {
            int r=0,w=0,e=0,d=0;
            while (*p) {
                int c=(unsigned char)*p++;
                if (c>='a'&&c<='z') c-=32;
                if (c=='R') r=1; else if (c=='W') w=1;
                else if (c=='E') e=1; else if (c=='D') d=1;
                else return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_PROT_USAGE));
            }
            v=(r?0:8)|(w?0:4)|(e?0:2)|(d?0:1);
            sprintf(out,plugin_str(PLMSG_DEV_PROT_MASK),v);
        }
    } else return beam_plugin_error(argc,argv,plugin_str(PLMSG_DEV_USAGE));
    return beam_plugin_text(argc,argv,out);
}
