/*
    Config
*/

#include "maincfg.h"
#include "stm32drv.h"
#include "cks.h"
#include "iflash.h"
#include "log.h"

#include <string.h>
    
Config cfg;

#if HWVER < 2

typedef struct __attribute__((__packed__)) {
    uint8_t id;
    uint8_t ver;
    uint16_t sz;
} hdr_t;

static int efind(hdr_t &h) {
    uint32_t beg = CONFIG_ADDR, p = beg;
    // особенности записи:
    //      - запись словами - _FLASH_WBLK_SIZE = 8 байт (необходимо выравнивание данных)
    //      - после erase каждое слово пишется только один раз, следующая запись возможна только после erase
    // формат хранения:
    //      - 1 слово (8 байт) - всегда индикатор, что следом актуальные данные или пусто.
    //          при сохранении данных мы в это слово ничего не пишем
    //      - 4 байта - заголовок hdr_t
    //      - данные
    //      - 2 байта - контрольная сумма.
    //
    // при следующей записи помечаем 1 слово-индикатор как 0xef.... - это значит, что следом
    // идёт неактуальная запись, которую надо пропустить.
    //
    // - в любой непонятной ситуации возвращаем -1
    // - если найдена валидная запись, возвращаем
    while (p+_FLASH_WBLK_SIZE*2 < CONFIG_AEND) {
        auto ff = *reinterpret_cast<__IO uint64_t *>(p);
        bool ok = ff == 0xffffffffffffffff;
        if (!ok && (ff != 0xefffffffffffffff))
            return -1;
        p += sizeof(uint64_t);
        h = *const_cast<hdr_t *>(reinterpret_cast<__IO hdr_t *>(p));
        if ((h.id != CONFIG_HDR) || (h.ver < 1) || (h.ver > CONFIG_VER) || (h.sz < 4) || (h.sz > 2048))
            return -1;
        if (ok)
            return iflash::validcks(p + sizeof(hdr_t), h.sz) ? p - beg : -1;
        else
            p += _FLASH_WBLK_ALIGN(sizeof(hdr_t) + h.sz + sizeof(cks_t));
    }

    return -1;
}

#endif // if HWVER < 2

void Config::init() {

#if HWVER < 2

    hdr_t h;
    // ищем валидный блок с данными
    auto p = efind(h);
    data_t d;
    if (p > 0) {
        CONSOLE("config found on: 0x%04x", p);
        uint16_t sz = h.sz < sizeof(data_t) ? h.sz : sizeof(data_t);
        memcpy(&d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(CONFIG_ADDR + p + sizeof(hdr_t))), sz);
    }

#else // if HWVER < 2

    data_t d;

    auto r = iflash::last(iflash::TYPE_CONFIG);
    if (r.read(d)) {
        CONSOLE("config found on: 0x%06x", r.addr()-_FLASH_BASE);
    }

#endif

    cfg._d      = d;
    cfg._chg    = false;
}

#if HWVER < 2

static bool esave(int p, const Config::data_t &data) {
    if (p > 0) {
        p += CONFIG_ADDR;
        // а если валидный блок всё же найден,
        // то пометим его как "пропустить"...
        auto st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, p-_FLASH_WBLK_SIZE, 0xefffffffffffffff);
        CONSOLE("HAL_FLASH_Program: %d, errno: %d", st, st == HAL_OK ? 0 : HAL_FLASH_GetError());
        if (st != HAL_OK)
            return false;
        
        // а под запись будем использовать уже следующий блок, выровненный до _FLASH_WBLK_SIZE
        auto sz = reinterpret_cast<__IO hdr_t *>(p)->sz;
        p += _FLASH_WBLK_ALIGN(sizeof(hdr_t) + sz + sizeof(cks_t));

        if (p + _FLASH_WBLK_SIZE + _FLASH_WBLK_ALIGN(sizeof(hdr_t) + sizeof(Config::data_t) + sizeof(cks_t)) > CONFIG_AEND) {
            CONSOLE("flash-page fulled, need erase!");
            p = -1;
        }
    }
    if (p > 0) {
        // слово индикатор - обязан быть как нетронутый ещё при записи
        if (_FLASH_WBLK_ISERASE(p))
            p += _FLASH_WBLK_SIZE;
        else {
            CONSOLE("free-indicator no nulled, need erase!");
            p = -1;
        }
    }
    if (p < 0) {
        // в случае, если мы не нашли валидный блок
        // или новый после него не влезет,
        // то надо выполнить стирание всей страницы
        if (!iflash::erase(CONFIG_ADDR))
            return false;
        p = CONFIG_ADDR + _FLASH_WBLK_SIZE;
    }
    
    // подготавливаем данные под сохранение,
    // их надо выровнять до _FLASH_WBLK_SIZE
    struct __attribute__((__packed__)) {
        hdr_t h;
        Config::data_t d;
    } d = {
        .h = {
            .id     = CONFIG_HDR,
            .ver    = CONFIG_VER,
            .sz     = sizeof(Config::data_t)
        },
        .d = data
    };

    // пишем подготовленные данные
    CONSOLE("config saving to: 0x%06x (0x%04x)", p-_FLASH_WBLK_SIZE, p-CONFIG_ADDR);
    return iflash::writecks(p, d, sizeof(hdr_t));
}

#endif // HWVER < 2

bool Config::save() {
    if (!_chg)
        return true;

#if HWVER < 2

    hdr_t h;
    // ищем валидный блок с данными
    auto p = efind(h);

    if ((p >= 0) && (CONFIG_ADDR + p + (sizeof(hdr_t) + sizeof(cks_t))*2 + h.sz + sizeof(Config::data_t) > CONFIG_AEND))
        // если блок валидный, но после него новый всё равно не влезет
        p = -1;
    
    // далее, приступаем к записи, для этого
    // надо разблокировать флеш-память
    iflash::Unlocker _l;

    if (!_l || !esave(p, _d))
        return false;

#else // if HWVER < 2

    auto r = iflash::save(_FLASH_TYPE_CONFIG, _d);
    if (r) {
        CONSOLE("config saving to: page=0x%06x, addr=0x%04x", r.page().num(), r.a());
    }
    else {
        CONSOLE("config saving FAIL");
        return false;
    }

#endif // if HWVER < 2
    
    _chg = false;
    return true;
}

const Config::data_t* Config::operator->() const {
    return &_d;
}

 Config::data_t *Config::operator*() {
    _chg = true;
    return &_d;
}

bool Config::resetdefault() {
    data_t d;
    cfg._d      = d;
    cfg._chg    = true;

    return save();
}
