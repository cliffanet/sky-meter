/*
    Error
*/

#include "err.h"

#include <stdio.h>

typedef struct {
    uint8_t errno;
    bool    active;
    uint16_t cnt;
    uint32_t tmr;
} errel_t;

#define ERR_ALLSZ   3
static errel_t _all[ERR_ALLSZ] = { 0 };

#define ERR_TMR     (36000 * 24 * 2)

namespace err {

    void add(uint8_t eno) {
        // смещаем список вниз
        int idst = 0;
        uint16_t cnt = 0;
        for (; idst < (ERR_ALLSZ-1); idst++)
            if (_all[idst].errno == 0)
                break;
            else
            if (_all[idst].errno == eno) {
                cnt = _all[idst].cnt;
                break;
            }
        for (; idst > 0; idst--)
            _all[idst] = _all[idst-1];
        
        // пишем в начало списка
        _all->errno = eno;
        _all->active= true;
        _all->cnt = cnt + 1;
        _all->tmr = ERR_TMR;
    }

    void tostr(char *s, size_t sz) {
        bool c = false;
        if (sz > 0)
            *s = '\0';
        for (const auto &e: _all) {
            if (sz < 2)
                break;
            if (e.errno == 0)
                continue;
            if (c) {
                *s = ',';
                s ++;
                sz --;
            }
            auto l = snprintf(s, sz, "E-%02X", e.errno);
            s += l;
            sz -= l;
            c = true;
        }
    }

    void tick() {
        for (auto &e: _all) {
            if ((e.errno == 0) || (e.tmr == 0))
                continue;
            e.tmr --;
            if (e.active && (e.tmr < ERR_TMR - 6000))
                e.active = false;
            if (e.tmr == 0)
                e.errno = 0;
        }
    }

} // namespace err 


