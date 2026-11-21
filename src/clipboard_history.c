#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "clipboard_history.h"
void beam_clip_history_init(struct BeamClipHistory *h,unsigned int limit,unsigned int budget,unsigned long expiry)
{
    memset(h,0,sizeof(*h));h->limit=limit<1?1:limit>16?16:limit;
    h->budget=budget<256?256:budget>2048?2048:budget;h->expiry=expiry?expiry:1800;h->next_id=1;
}
static void remove_entry(struct BeamClipHistory *h,unsigned int n)
{if(n<h->count){memmove(h->entries+n,h->entries+n+1,(h->count-n-1)*sizeof(h->entries[0]));--h->count;memset(h->entries+h->count,0,sizeof(h->entries[0]));}}
int beam_clip_history_expire(struct BeamClipHistory *h,unsigned long now)
{
    unsigned int i=0;int changed=0;
    while(i<h->count){if(now<h->entries[i].stamp || now-h->entries[i].stamp>=h->expiry){remove_entry(h,i);changed=1;}else ++i;}
    return changed;
}
int beam_clip_history_add(struct BeamClipHistory *h,const char *text,unsigned long now)
{
    unsigned int i,n,used=0;
    beam_clip_history_expire(h,now);n=(unsigned int)strlen(text);
    if(!n || n>BEAM_CLIP_HISTORY_TEXT || n+1>h->budget)return 0;
    for(i=0;i<h->count;i++)if(!strcmp(h->entries[i].text,text)){remove_entry(h,i);break;}
    for(i=0;i<h->count;i++)used+=(unsigned int)strlen(h->entries[i].text)+1;
    while(h->count && (h->count>=h->limit || used+n+1>h->budget)){
        used-=(unsigned int)strlen(h->entries[h->count-1].text)+1;remove_entry(h,h->count-1);
    }
    memmove(h->entries+1,h->entries,h->count*sizeof(h->entries[0]));
    if(!h->next_id)h->next_id=1;
    h->entries[0].id=h->next_id++;h->entries[0].stamp=now;strcpy(h->entries[0].text,text);++h->count;return 1;
}
const char *beam_clip_history_find(struct BeamClipHistory *h,unsigned long id,unsigned long now)
{unsigned int i;beam_clip_history_expire(h,now);for(i=0;i<h->count;i++)if(h->entries[i].id==id)return h->entries[i].text;return NULL;}
int beam_clip_history_match(const char *text,const char *filter)
{
    unsigned int i,j;
    if(!filter || !*filter)return 1;
    for(i=0;text[i];i++){
        for(j=0;filter[j] && text[i+j];j++){
            int a=(unsigned char)text[i+j],b=(unsigned char)filter[j];
            if(a>='A' && a<='Z')a+=32;
            if(b>='A' && b<='Z')b+=32;
            if(a!=b)break;
        }
        if(!filter[j])return 1;
    }
    return 0;
}
int beam_clip_history_save(const struct BeamClipHistory *h,const char *path)
{
    static const char hex[]="0123456789ABCDEF";unsigned int i,j;FILE *f=fopen(path,"wb");int ok=1;
    if(!f)return 0;
    if(fprintf(f,"BEAMCLIP1 %u %u %lu %lu\n",h->count,h->budget,h->expiry,h->next_id)<0)ok=0;
    for(i=0;ok && i<h->count;i++){
        if(fprintf(f,"%lu %lu ",h->entries[i].id,h->entries[i].stamp)<0){ok=0;break;}
        for(j=0;h->entries[i].text[j];j++){
            unsigned char c=(unsigned char)h->entries[i].text[j];
            if(fputc(hex[c>>4],f)==EOF || fputc(hex[c&15],f)==EOF){ok=0;break;}
        }
        if(fputc('\n',f)==EOF)ok=0;
    }
    if(fclose(f))ok=0;
    return ok;
}
static int unhex(int c)
{if(c>='0' && c<='9')return c-'0';if(c>='A' && c<='F')return c-'A'+10;return -1;}
int beam_clip_history_load(struct BeamClipHistory *h,const char *path,unsigned long now)
{
    struct BeamClipHistory parsed;char line[300],hex[242],extra;unsigned int count,budget,i,j,used=0;unsigned long expiry,next;
    FILE *f=fopen(path,"rb");int ok=0;if(!f)return 0;
    if(!fgets(line,sizeof(line),f) || sscanf(line,"BEAMCLIP1 %u %u %lu %lu %c",&count,&budget,&expiry,&next,&extra)!=4 ||
       count>16 || budget<256 || budget>2048 || expiry<60 || expiry>14400)goto done;
    beam_clip_history_init(&parsed,16,budget,expiry);parsed.next_id=next;
    for(i=0;i<count;i++){
        if(!fgets(line,sizeof(line),f) || sscanf(line,"%lu %lu %241s %c",&parsed.entries[i].id,&parsed.entries[i].stamp,hex,&extra)!=3 ||
           !parsed.entries[i].id || strlen(hex)%2 || strlen(hex)>240)goto done;
        for(j=0;hex[j];j+=2){int a=unhex(hex[j]),b=unhex(hex[j+1]);if(a<0 || b<0 || !(a*16+b))goto done;parsed.entries[i].text[j/2]=(char)(a*16+b);}
        parsed.entries[i].text[j/2]=0;used+=j/2+1;
        if(used>budget)goto done;
        parsed.count++;
    }
    if(fgetc(f)!=EOF)goto done;
    beam_clip_history_expire(&parsed,now);*h=parsed;ok=1;
done:
    memset(&parsed,0,sizeof(parsed));fclose(f);return ok;
}
