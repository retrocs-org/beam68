/* Soundness of the launcher's query narrowing (search_items): for a
 * single-word query of 3+ characters, anything matching the query plus one
 * more character must also match the query itself.  Random names, aliases
 * and typo'd substrings; zero violations required. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search_rank.h"
static const char al[]="abcdeSHELL xy-_.:/1";
static void rnd(char *s,int n){int k;for(k=0;k<n;k++)s[k]=al[rand()%(sizeof(al)-1)];s[n]=0;}
int main(void){ int i,bad=0,checked=0; char q[32],q2[32],n[24],a[24]; srand(11);
 for(i=0;i<400000;i++){ int nl=4+rand()%14,alen=rand()%10,st,len,k;
  rnd(n,nl); rnd(a,alen);
  const char *src=(alen&&rand()%3==0)?a:n; int sl=(int)strlen(src); if(sl<4) continue;
  st=rand()%(sl-3); len=4+rand()%(sl-st-3>0?sl-st-3:1); if(st+len>sl) len=sl-st;
  memcpy(q2,src+st,len); q2[len]=0;
  for(k=0;k<len;k++) if(q2[k]==' ') q2[k]='a';
  if(rand()%2){ int p=rand()%len; q2[p]=al[rand()%(sizeof(al)-1)]; if(q2[p]==' ') q2[p]='c'; }
  if(rand()%4==0){ int p=rand()%len; memmove(q2+p,q2+p+1,len-p); len--; }
  if(len<4) continue;
  strcpy(q,q2); q[len-1]=0;
  if(beam_search_rank_alias(q2,n,alen?a:0,-1,0)>0){ checked++;
    if(beam_search_rank_alias(q,n,alen?a:0,-1,0)<=0){bad++; if(bad<8)printf("q=[%s] q2=[%s] n=[%s] a=[%s]\n",q,q2,n,a);} }
 }
 assert(bad==0 && checked>100000);
 printf("Query narrowing: %d growing queries, every match kept: PASS\n",checked); return 0;}
