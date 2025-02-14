#ifndef _jump_logbook_H
#define _jump_logbook_H

#include "../../def.h"
#include "../sys/iflash.h"
#include "../sys/clock.h"

#ifdef USE_LOGBOOK

#define LOGBOOK_JUMPCOUNT   100
#define LOGBOOK_ITSZ        _FLASH_WBLK_ALIGN(sizeof(LogBook::item_t) + 4)
#define LOGBOOK_SIZE        _FLASH_PAGE_ALIGN(LOGBOOK_ITSZ * LOGBOOK_JUMPCOUNT)
#define LOGBOOK_AEND        (_FLASH_BASE + ((_FLASH_PAGE_ALL-1) * _FLASH_PAGE_SIZE))
#define LOGBOOK_ADDR        (LOGBOOK_AEND - LOGBOOK_SIZE)

#define LOGBOOK_HDR         '@'

namespace LogBook {
    typedef struct __attribute__((__packed__)) {
        uint16_t key;
        uint32_t num; // размерность должна быть той же, что в maincfg,
                      // т.к. во время прыжка сначала сюда пишется кол-во
                      // прыжков из maincfg, а по завершению - уже отсюда
                      // в maincfg
        tm_t tm;
        uint16_t toffsec;
        uint16_t ffsec;
        uint16_t cnpsec;
        int16_t  begalt;
        int16_t  cnpalt;
    } item_t;

    void beg_toff();
    void beg_ff(uint32_t ms, int16_t alt);
    void beg_cnp(uint32_t ms, int16_t alt);

    void tick_toff(uint16_t ms);
    void tick_ff(uint16_t ms);
    void tick_cnp(uint16_t ms);

    LogBook::item_t end(uint32_t ms);

    uint32_t findprv(uint32_t addr, item_t &item);

} // namespace LogBook

#endif // USE_LOGBOOK

#endif // _jump_proc_H
