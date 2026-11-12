/* Exercise the production message sender with a bounded in-memory Exec port. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_job.h"
#include "plugin_protocol.h"
#define main helloworld_main
#include "../sdk/Example/HelloWorld/HelloWorld.c"
#undef main
#define main calculator_main
#include "../Plugins/Calc/Calc.c"
#undef main
struct Beam68ReplyMessage *test_take_reply(void);
void test_delivery(int port,int memory);
int main(void)
{
    char hex[BEAM_PLUGIN_MAX_INPUT*2+2],input[121];
    char *argv[]={"HelloWorld",hex,"T:unused",BEAM68_REPLY_PORT,"7",NULL};
    struct Beam68ReplyMessage *m;
    int i;
    assert(beam_plugin_hex_argument("Amiga",hex,sizeof(hex)));
    assert(helloworld_main(5,argv)==0);
    m=test_take_reply();assert(m);
    assert(m->version==1 && m->magic==BEAM68_REPLY_MAGIC && m->serial==7);
    assert(m->message.mn_Length==sizeof(*m) && !m->message.mn_ReplyPort);
    assert(m->count==1 && !m->is_error && m->rows[0].kind==BEAM68_ROW_TEXT);
    assert(!strcmp(m->rows[0].text,"Hello, Amiga!"));free(m);
    argv[1]="-";assert(helloworld_main(5,argv)==0);
    m=test_take_reply();assert(m && m->is_error && m->count==1);free(m);
    assert(beam_plugin_hex_argument("12*(3+4)",hex,sizeof(hex)));
    argv[1]=hex;assert(calculator_main(5,argv)==0);
    m=test_take_reply();assert(m && !m->is_error && strstr(m->rows[0].text,"84"));free(m);
    assert(beam_plugin_hex_argument("1/0",hex,sizeof(hex)));
    assert(calculator_main(5,argv)==0);
    m=test_take_reply();assert(m && m->is_error && strstr(m->rows[0].text,"Invalid expression"));free(m);
    assert(beam_plugin_line(5,argv,"TEXT=Local time 12:34\n")==0);
    m=test_take_reply();assert(m && !strcmp(m->rows[0].text,"Local time 12:34"));free(m);
    assert(beam_plugin_line(5,argv,"invalid")==20 && !test_take_reply());
    argv[1]="4G";assert(helloworld_main(5,argv)==20);
    assert(helloworld_main(3,argv)==20 && !test_take_reply());
    argv[1]="414243";assert(beam68_plugin_input(5,argv,input,sizeof(input)) && !strcmp(input,"ABC"));
    beam68_rows_begin();
    assert(!beam68_row(99,"bad",NULL));
    assert(!beam68_row(BEAM68_ROW_OPEN,"bad",NULL));
    assert(beam68_row(BEAM68_ROW_OPEN,"Work:A\tB","Work:A"));
    assert(beam68_row(BEAM68_ROW_QUERY,"Again","find:abc"));
    for (i=2;i<8;i++) assert(beam68_row(BEAM68_ROW_TEXT,"text",NULL));
    assert(!beam68_row(BEAM68_ROW_TEXT,"ninth",NULL));
    assert(beam68_rows_send(5,argv,0)==0);
    m=test_take_reply();assert(m && m->count==8 && !strcmp(m->rows[0].text,"Work:A B"));
    assert(m->rows[0].kind==BEAM68_ROW_OPEN && !strcmp(m->rows[0].value,"Work:A"));
    assert(m->rows[1].kind==BEAM68_ROW_QUERY);free(m);
    argv[4]="4294967296";assert(!beam68_plugin_input(5,argv,input,sizeof(input)));assert(beam68_rows_send(5,argv,0)==20 && !test_take_reply());
    argv[4]="7oops";assert(beam68_rows_send(5,argv,0)==20);
    argv[4]="7";argv[3]="OTHER";assert(!beam68_plugin_input(5,argv,input,sizeof(input)));
    argv[3]=BEAM68_REPLY_PORT;
    test_delivery(0,1);assert(beam68_plugin_text(5,argv,"hello")==20 && !test_take_reply());
    test_delivery(1,0);assert(beam68_plugin_error(5,argv,"failed")==20 && !test_take_reply());
    test_delivery(1,1);assert(beam68_plugin_text(5,argv,"back")==0);
    m=test_take_reply();assert(m && !strcmp(m->rows[0].text,"back"));free(m);
    puts("Public protocol 1: native text/errors, eight selectable rows, routing and delivery failures: PASS");
    return 0;
}
