
#include "logbook.h"

#ifdef USE_LOGBOOK

#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"

#include <memory.h>

static LogBook::item_t _cur = { 0 };
static uint32_t _ms = 0;
static uint16_t _incms(uint32_t ms) {
    _ms += ms;
    if (_ms < 1000)
        return 0;
    auto sec = _ms / 1000;
    _ms %= 1000;

    return 0;
}

void _keygen() {
    _cur.num = cfg->jmpcnt + 1;
    _cur.tm = tmNow();
    if (_cur.key == 0)
        _cur.key = tmRand();
}

bool _read(uint32_t addr, LogBook::item_t &item) {
    if (*reinterpret_cast<__IO uint8_t *>(addr) != LOGBOOK_HDR)
        return false;
    
    auto sz = *reinterpret_cast<__IO uint8_t *>(addr+1);
    if (!iflash::validcks(addr+2, sz))
        return false;
    
    uint8_t sz1 = sizeof(LogBook::item_t);
    if (sz1 > sz) {
        bzero(&item, sz1);
        sz1 = sz;
    }
    memcpy(&item, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(addr+2)), sz1);

    return true;
}

bool _save() {
    uint32_t p = cfg->lbaddr;
    if ((p == 0) || (p < LOGBOOK_ADDR) || ((p+LOGBOOK_ITSZ) > LOGBOOK_AEND))
        p = LOGBOOK_ADDR;
    if (!_FLASH_PAGE_ISBEG(p) && !_FLASH_WBLK_ISERASE(p))
        p = LOGBOOK_ADDR;
    
    {
        iflash::Unlocker _l;
        if (!_l) return false;
        
        if (_FLASH_PAGE_ISBEG(p))
            if (!iflash::erase(p))
                return false;

        struct __attribute__((__packed__)) {
            uint8_t h;
            uint8_t sz;
            LogBook::item_t d;
        } d = {
            .h  = LOGBOOK_HDR,
            .sz = sizeof(LogBook::item_t),
            .d  = _cur
        };

        if (!iflash::writecks(p, d, 2))
            return false;
    }
    
    p += LOGBOOK_ITSZ;

    (*cfg)->lbaddr = p;
    (*cfg)->jmpcnt = _cur.num;

    return cfg.save();
}

namespace LogBook {
    void beg_toff() {
        bzero(&_cur, sizeof(_cur));
        _keygen();
        _ms = 0;
    }

    void beg_ff(uint32_t ms, int16_t alt) {
        _keygen();
        _cur.begalt = alt;
        _cur.ffsec += ms / 1000;
        _ms = ms % 1000;
    }

    void beg_cnp(uint32_t ms, int16_t alt) {
        _keygen();
        if (_cur.begalt == 0)
            _cur.begalt = alt;
        _cur.cnpalt = alt;
        _cur.cnpsec += ms / 1000;
        _ms = ms % 1000;
    }

    void tick_toff(uint16_t ms) {
        _cur.toffsec+= _incms(ms);
    }

    void tick_ff(uint16_t ms) {
        _cur.ffsec  += _incms(ms);
    }

    void tick_cnp(uint16_t ms) {
        _cur.cnpsec += _incms(ms);
    }

    void end() {
        if (_cur.key > 0)
            _save();
        bzero(&_cur, sizeof(_cur));
        _ms = 0;
    }

    uint32_t findprv(uint32_t addr, item_t &item) {
        // если в cfg->lbaddr у нас ещё ничего нет,
        // значит логбук пока пустой.
        if (cfg->lbaddr == 0)
            return 0;
        
        // стартовый адрес поиска
        auto a = addr - (LOGBOOK_ITSZ / 2);
        // Логбук у нас работает по принципу кольцевого буфера,
        // где cfg->lbaddr - это указатель на текущее положение
        // в этом буфере.
        // При поиске в обратную сторону нам надо понять, где находится
        // стартовый адрес - до этого курсора или после. Это означает,
        // выполнили ли мы уже переход от начала участка хранения
        // в его конец, или еще нет.
        bool roll = addr > cfg->lbaddr;

        for (int i = 0; i < 10; i++) {
            if (roll && (a < cfg->lbaddr))
                // окончание кольцевого буфера
                break;
            if (a < LOGBOOK_ADDR) {
                if (roll)
                    break;
                a = LOGBOOK_AEND - (LOGBOOK_ITSZ / 2);
            }

            if (_read(a, item))
                return a;
            
            a -= _FLASH_WBLK_SIZE;
        }

        return 0;
    }

} // namespace LogBook 

#endif // USE_LOGBOOK
