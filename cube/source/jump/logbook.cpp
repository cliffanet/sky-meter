
#include "logbook.h"

#ifdef USE_LOGBOOK

#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"
#include "../sys/iflash.h"
#include "../sys/log.h"

#include <memory.h>

static LogBook::item_t _cur = { 0 };
static uint32_t _ms = 0;
static uint16_t _incms(uint32_t ms) {
    _ms += ms;
    if (_ms < 1000)
        return 0;
    
    auto sec = _ms / 1000;
    _ms %= 1000;

    return sec;
}

static void _keygen() {
    _cur.num = cfg->jmpcnt + 1;
    _cur.tm = tmNow();
    if (_cur.key == 0)
        _cur.key = tmRand();
}

#if HWVER < 2

static bool _read(uint32_t addr, LogBook::item_t &item) {
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

static bool _save() {
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

#else // if HWVER < 2

static bool _save() {
    auto r = iflash::save(_FLASH_TYPE_LOGBOOK, _cur);
    if (r) {
        CONSOLE("logbook saving to: page=0x%06x, addr=0x%04x", r.page().num(), r.a());
    }
    else {
        CONSOLE("logbook saving FAIL");
        return false;
    }

    (*cfg)->jmpcnt = _cur.num;

    return cfg.save();
}
#endif // if HWVER < 2

namespace LogBook {
    void beg_toff() {
        bzero(&_cur, sizeof(_cur));
        _keygen();
        _ms = 0;
    }

    void beg_ff(uint32_t ms, int16_t alt) {
        _keygen();
        _cur.begalt = alt;
        
        auto sec = ms / 1000;
        _cur.ffsec += sec;
        if (_cur.toffsec >= sec)
            _cur.toffsec -= sec;
        
        _ms = ms % 1000;
    }

    void beg_cnp(uint32_t ms, int16_t alt) {
        _keygen();
        if (_cur.begalt == 0)
            _cur.begalt = alt;
        _cur.cnpalt = alt;

        auto sec = ms / 1000;
        _cur.cnpsec += sec;
        if (_cur.ffsec >= sec)
            _cur.ffsec -= sec;
        
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

    LogBook::item_t end(uint32_t ms) {
        if (_cur.key > 0) {
            auto sec = ms / 1000;
            if (_cur.cnpsec >= sec)
                _cur.cnpsec -= sec;
            
            _save();
        }
        auto _last = _cur;
        bzero(&_cur, sizeof(_cur));
        _ms = 0;
        return _last;
    }

#if HWVER < 2

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

#endif // if HWVER < 2

} // namespace LogBook 

#endif // USE_LOGBOOK
