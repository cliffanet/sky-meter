
#include "init.h"
#include "power.h"
#include "clock.h"
#include "maincfg.h"
#include "../view/dspl.h"
#include "../view/btn.h"
#include "../jump/proc.h"

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
    pwr::init();
    const char *d = __DATE__;
    const char *t = __TIME__;
    tm_t tm = {
        .year   = static_cast<uint16_t>( __YEAR(d) ),
        .mon    = static_cast<uint8_t> ( __MONTH(d) ),
        .day    = static_cast<uint8_t> ( __DAY(d) ),
        .h      = static_cast<uint8_t> ( __HR(t) ),
        .m      = static_cast<uint8_t> ( __MIN(t) ),
        .s      = static_cast<uint8_t> ( __SEC(t) ),
    };
    tmAdjust(tm, 0);
    Config::init();
    Btn::init(); // д.б. перед Dspl::init, т.к. Dspl::init выбирает страницу и присваивает хендлеры
    Dspl::init();
    jmp::init();
}
