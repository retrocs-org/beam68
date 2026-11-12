#include "calc_engine.h"

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
