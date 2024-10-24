
#include "init.h"
#include "../sys/clock.h"
#include "../view/dspl.h"

#define __MONTH(d) (\
    d[2] == 'n' ? (d[1] == 'a' ? 1 : 6) \
    : d[2] == 'b' ? 2 \
    : d[2] == 'r' ? (d[0] == 'M' ? 3 : 4) \
    : d[2] == 'y' ? 5 \
    : d[2] == 'l' ? 7 \
    : d[2] == 'g' ? 8 \
    : d[2] == 'p' ? 9 \
    : d[2] == 't' ? 10 \
    : d[2] == 'v' ? 11 \
    : 12)

#define NUM(s)          ((s >= '0') && (s <= '9') ? s - '0' : 0)
#define NUM2(s1, s2)    (NUM(s1) * 10 + (NUM(s2)))

#define __YEAR(d)   NUM2(d[9], d[10])
#define __DAY(d)    NUM2(d[4], d[5])

#define __HR(t)     NUM2(t[0], t[1])
#define __MIN(t)    NUM2(t[3], t[4])
#define __SEC(t)    NUM2(t[6], t[7])

extern "C"
void init_full() {
    const char *d = __DATE__;
    const char *t = __TIME__;
    tm_t tm = {
        .year   = __YEAR(d),
        .mon    = __MONTH(d),
        .day    = __DAY(d),
        .h      = __HR(t),
        .m      = __MIN(t),
        .s      = __SEC(t),
    };
    tmAdjust(tm, 0);
    Dspl::init();
}
