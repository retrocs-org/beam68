#include "calc_engine.h"
#include <string.h>

#define BM_MAX 2147483647L
#define BM_MIN (-2147483647L - 1L)

struct Parser { const char *s; int error; int depth; };

static void ws(struct Parser *p)
{
    while (*p->s == ' ' || *p->s == '\t') ++p->s;
}

static long expr(struct Parser *p);

static long factor(struct Parser *p)
{
    long value = 0;
    int negative = 0;
    ws(p);
    if (++p->depth > 32) { p->error = 1; --p->depth; return 0; }
    if (*p->s == '+' || *p->s == '-') {
        negative = (*p->s == '-'); ++p->s;
        value = factor(p);
        if (negative && value == BM_MIN) p->error = 1;
        else if (negative) value = -value;
    } else if (*p->s == '(') {
        ++p->s;
        value = expr(p);
        ws(p);
        if (*p->s == ')') ++p->s;
        else p->error = 1;
    } else if (*p->s == '$' || *p->s == '%' ||
               (p->s[0] == '0' && (p->s[1] == 'x' || p->s[1] == 'X'))) {
        /* $FF / 0xFF hexadecimal, %1010 binary: up to 32 bits, two's
           complement, so $FFFFFFFF is -1 as on the 68000. */
        unsigned long bits = 0;
        int base = *p->s == '%' ? 2 : 16, digits = 0, max = base == 2 ? 32 : 8;
        p->s += *p->s == '0' ? 2 : 1;
        for (;;) {
            int c = *p->s, d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else break;
            if (d >= base) break;
            if (++digits > max) p->error = 1;
            bits = (bits << (base == 2 ? 1 : 4)) | (unsigned long)d;
            ++p->s;
        }
        if (!digits) p->error = 1;
        value = (bits & 0x80000000UL) ? BM_MIN+(long)(bits & 0x7FFFFFFFUL) : (long)bits;
    } else if (*p->s >= '0' && *p->s <= '9') {
        while (*p->s >= '0' && *p->s <= '9') {
            int digit = *p->s - '0';
            if (value > (BM_MAX - digit) / 10) p->error = 1;
            else if (!p->error) value = value * 10 + digit;
            ++p->s;
        }
    } else p->error = 1;
    --p->depth;
    return value;
}

static int mul_safe(long a, long b, long *out)
{
    if (a == 0 || b == 0) { *out=0; return 1; }
    if ((a == -1 && b == BM_MIN) || (b == -1 && a == BM_MIN)) return 0;
    if (a > 0) {
        if (b > 0 && a > BM_MAX/b) return 0;
        if (b < 0 && b < BM_MIN/a) return 0;
    } else {
        if (b > 0 && a < BM_MIN/b) return 0;
        if (b < 0 && a < BM_MAX/b) return 0;
    }
    *out = a*b;
    return 1;
}

static long term(struct Parser *p)
{
    long a = factor(p);
    for (;;) {
        long b, out;
        int op;
        ws(p);
        if (*p->s != '*' && *p->s != '/') return a;
        op = *p->s++;
        b = factor(p);
        if (p->error) return 0;
        if (op == '*') {
            if (!mul_safe(a,b,&out)) { p->error=1; return 0; }
        } else {
            if (!b || (a == BM_MIN && b == -1)) { p->error=1; return 0; }
            out = a/b;
        }
        a = out;
    }
}

static long expr(struct Parser *p)
{
    long a = term(p);
    for (;;) {
        long b;
        int op;
        ws(p);
        if (*p->s != '+' && *p->s != '-') return a;
        op = *p->s++;
        b = term(p);
        if (p->error) return 0;
        if (op == '+') {
            if ((b > 0 && a > BM_MAX-b) || (b < 0 && a < BM_MIN-b)) {
                p->error=1; return 0;
            }
            a += b;
        } else {
            if ((b < 0 && a > BM_MAX+b) || (b > 0 && a < BM_MIN+b)) {
                p->error=1; return 0;
            }
            a -= b;
        }
    }
}

int beam_calculate(const char *expression, long *answer)
{
    struct Parser p;
    long value;
    if (!expression || !answer) return 0;
    p.s = expression; p.error = 0; p.depth = 0;
    ws(&p);
    if (!*p.s) return 0;
    value = expr(&p);
    ws(&p);
    if (p.error || *p.s) return 0;
    *answer = value;
    return 1;
}

void beam_calc_format(long value, char *out, unsigned int cap)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[80];
    unsigned long bits = (unsigned long)value & 0xFFFFFFFFUL;
    int i, n = 0, shown = 0;
    /* decimal */
    {
        char tmp[16];
        unsigned long mag = value < 0 ? (unsigned long)(-(value + 1)) + 1UL : (unsigned long)value;
        int t = 0;
        if (value < 0) buf[n++] = '-';
        do { tmp[t++] = (char)('0' + mag % 10UL); mag /= 10UL; } while (mag);
        while (t) buf[n++] = tmp[--t];
    }
    buf[n++] = ' '; buf[n++] = '$';
    for (i = 28; i >= 0; i -= 4) {
        int d = (int)((bits >> i) & 15UL);
        if (d || shown || i == 0) { buf[n++] = hex[d]; shown = 1; }
    }
    if (bits <= 0xFFFFUL) {
        buf[n++] = ' '; buf[n++] = '%';
        shown = 0;
        for (i = 15; i >= 0; i--) {
            int b = (int)((bits >> i) & 1UL);
            if (b || shown || i == 0) { buf[n++] = (char)('0' + b); shown = 1; }
        }
    }
    buf[n] = 0;
    for (i = 0; buf[i] && (unsigned int)i + 1 < cap; i++) out[i] = buf[i];
    if (cap) out[i] = 0;
}

/* Decimal unit conversion uses reduced integer ratios; no floating point,
 * network access or changes to the arithmetic expression grammar. */
struct CalcUnit { const char *name; unsigned long factor; int dimension; };
static const struct CalcUnit calc_units[] = {
    {"B",1,0},{"KB",1000,0},{"MB",1000000,0},{"GB",1000000000,0},
    {"KiB",1024,0},{"MiB",1048576,0},{"GiB",1073741824,0},
    {"ms",1,1},{"s",1000,1},{"min",60000,1},{"h",3600000,1},{"day",86400000,1},
    {"mm",1000,2},{"cm",10000,2},{"m",1000000,2},{"km",1000000000,2},
    {"in",25400,2},{"ft",304800,2},{"yd",914400,2},{"mi",1609344000,2},
    {"mg",1,3},{"g",1000,3},{"kg",1000000,3},
    {"C",1,4},{"F",1,4},{"K",1,4},
    {0,0,0}
};
static int calc_word(const char **input,char *out,unsigned int size)
{
    const char *p=*input;unsigned int n=0;
    while(*p==' ' || *p=='\t')++p;
    while((*p>='A'&&*p<='Z') || (*p>='a'&&*p<='z')) {
        if(n+1>=size)return 0;
        out[n++]=*p++;
    }
    out[n]=0;*input=p;return n!=0;
}
static int calc_equal(const char *a,const char *b)
{
    while(*a && *b){int x=*a++,y=*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}
    return !*a && !*b;
}
static unsigned long calc_gcd(unsigned long a,unsigned long b)
{ while(b){unsigned long r=a%b;a=b;b=r;}return a; }
static const struct CalcUnit *calc_unit(const char *word)
{
    const struct CalcUnit *u;
    for(u=calc_units;u->name;u++)if(calc_equal(word,u->name))return u;
    return 0;
}
/* Multiply a remainder by ten without overflowing a 32-bit unsigned long. */
static int calc_digit(unsigned long *remainder,unsigned long denominator)
{
    unsigned long r=0;int digit=0,i;
    for(i=0;i<10;i++) {
        if(r>=denominator-*remainder){r-=denominator-*remainder;++digit;}
        else r+=*remainder;
    }
    *remainder=r;return digit;
}
static unsigned long calc_magnitude(long n)
{ return n<0?(unsigned long)(-(n+1))+1UL:(unsigned long)n; }
static int calc_signed_multiply(long *n,unsigned long factor)
{
    unsigned long magnitude=calc_magnitude(*n),limit=*n<0?2147483648UL:2147483647UL;
    if(factor && magnitude>limit/factor)return 0;
    magnitude*=factor;
    *n=*n<0?-(long)(magnitude-1UL)-1L:(long)magnitude;
    return 1;
}
static void calc_reduce(long *n,unsigned long *d)
{
    unsigned long g=calc_gcd(calc_magnitude(*n),*d);
    *n/= (long)g;*d/=g;
}
static int calc_offset(long *n,unsigned long *d,long hundredths)
{
    unsigned long g,m;long offset;
    calc_reduce(n,d);g=calc_gcd(*d,100UL);m=100UL/g;
    if(!calc_signed_multiply(n,m) || *d/g>2147483647UL/calc_magnitude(hundredths))return 0;
    offset=hundredths*(long)(*d/g);
    if((offset>0 && *n>BM_MAX-offset) || (offset<0 && *n<BM_MIN-offset))return 0;
    *n+=offset;*d*=m;calc_reduce(n,d);return 1;
}
static int calc_ratio(long *n,unsigned long *d,unsigned long a,unsigned long b)
{
    unsigned long g=calc_gcd(calc_magnitude(*n),b);*n/=(long)g;b/=g;
    g=calc_gcd(a,*d);a/=g;*d/=g;
    if(*d>2147483647UL/b || !calc_signed_multiply(n,a))return 0;
    *d*=b;return 1;
}
static int calc_temperature(const struct CalcUnit *from,const struct CalcUnit *to,
                            unsigned long *magnitude,unsigned long *denominator,int *negative)
{
    long n;unsigned long d=*denominator;
    if(*magnitude>2147483647UL)return 0;
    n=*negative?-(long)*magnitude:(long)*magnitude;
    if(calc_equal(from->name,"C")){if(!calc_offset(&n,&d,27315))return 0;}
    else if(calc_equal(from->name,"F")){
        if(!calc_offset(&n,&d,45967) || !calc_ratio(&n,&d,5,9))return 0;
    }
    if(n<0)return 0; /* Kelvin cannot be negative. */
    if(calc_equal(to->name,"C")){if(!calc_offset(&n,&d,-27315))return 0;}
    else if(calc_equal(to->name,"F")){
        if(!calc_ratio(&n,&d,9,5) || !calc_offset(&n,&d,-45967))return 0;
    }
    *negative=n<0;*magnitude=calc_magnitude(n);*denominator=d;return 1;
}

int beam_calc_result(const char *input,char *out,unsigned int capacity)
{
    const char *p=input;const struct CalcUnit *from,*to;
    char word[16],destination[16],buf[96],digits[16];
    unsigned long numerator=0,denominator=1,a,b,g,whole,rem;
    int negative=0,n=0,k=0,fraction=0,decimal=0,seen=0,i,approx;
    long answer;
    if(!out || !capacity)return 0;
    out[0]=0;if(!input)return 0;
    if(beam_calculate(input,&answer)) {
        beam_calc_format(answer,buf,sizeof(buf));
        if(strlen(buf)+1>capacity)return 0;
        strcpy(out,buf);return 1;
    }
    while(*p==' ' || *p=='\t')++p;
    if(*p=='+' || *p=='-'){negative=*p=='-';++p;}
    while((*p>='0'&&*p<='9') || *p=='.') {
        if(*p=='.'){if(decimal)return 0;decimal=1;++p;continue;}
        if(decimal && ++fraction>3)return 0;
        if(numerator>(0xffffffffUL-(unsigned long)(*p-'0'))/10UL)return 0;
        numerator=numerator*10UL+(unsigned long)(*p++-'0');seen=1;
        if(decimal)denominator*=10UL;
    }
    if(!seen || !calc_word(&p,word,sizeof(word)) || !(from=calc_unit(word)))return 0;
    if(!calc_word(&p,word,sizeof(word)) || (!calc_equal(word,"to")&&!calc_equal(word,"in")))return 0;
    if(!calc_word(&p,destination,sizeof(destination)) || !(to=calc_unit(destination)) || from->dimension!=to->dimension)return 0;
    while(*p==' ' || *p=='\t')++p;
    if(*p)return 0;
    if(from->dimension==4) {
        if(!calc_temperature(from,to,&numerator,&denominator,&negative))return 0;
    } else {
    a=from->factor;b=to->factor;
    g=calc_gcd(a,b);a/=g;b/=g;
    g=calc_gcd(numerator,b);numerator/=g;b/=g;
    g=calc_gcd(a,denominator);a/=g;denominator/=g;
    if((a && numerator>0xffffffffUL/a) || denominator>0xffffffffUL/b)return 0;
    numerator*=a;denominator*=b;
    }
    whole=numerator/denominator;rem=numerator%denominator;
    if(negative && numerator)buf[n++]='-';
    do{digits[k++]=(char)('0'+whole%10UL);whole/=10UL;}while(whole);
    while(k)buf[n++]=digits[--k];
    if(rem){buf[n++]='.';for(i=0;i<12 && rem;i++)buf[n++]=(char)('0'+calc_digit(&rem,denominator));}
    approx=rem!=0;buf[n]=0;
    while(n && buf[n-1]=='0' && strchr(buf,'.'))--n;
    if(n && buf[n-1]=='.')--n;
    buf[n++]=' ';for(p=to->name;*p;p++)buf[n++]=*p;
    buf[n]=0;
    if((unsigned int)n+1+(approx?1:0)>capacity)return 0;
    if(approx){out[0]='~';strcpy(out+1,buf);}else strcpy(out,buf);
    return 1;
}
