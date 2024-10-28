/*
    Config
*/

#include "maincfg.h"
#include "stm32drv.h"
#include "log.h"

#include <string.h>
    
Config cfg;

typedef struct __attribute__((__packed__)) cks_s {
    uint8_t a, b;

    bool operator== (const struct cks_s & cks) {
        return (this == &cks) || ((this->a == cks.a) && (this->b == cks.b));
    };
    operator bool() const { return (a != 0) || (b != 0); }

    void clear() {
        a = 0;
        b = 0;
    }

    void add(uint8_t d) {
        a += d;
        b += a;
    }

    void add(const uint8_t *d, size_t _sz) {
        for (; _sz > 0; _sz--, d++) {
            a += *d;
            b += a;
        }
    }
} cks_t;

typedef struct __attribute__((__packed__)) {
    uint8_t id;
    uint8_t ver;
    uint16_t sz;
} hdr_t;

static int efind(hdr_t &h) {
    uint32_t beg = CONFIG_ADDR, p = beg;
    while (p+sizeof(hdr_t) < CONFIG_AEND) {
        h = *const_cast<hdr_t *>(reinterpret_cast<__IO hdr_t *>(p));
        if ((h.ver < 1) || (h.ver > CONFIG_VER) || (h.sz < 4) || (h.sz > 2048))
            return -1;
        auto d = p + sizeof(hdr_t);
        if (h.id == CONFIG_HDR_OK) {
            cks_t c1 = { 0 };
            c1.add(const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(d)), h.sz);
            cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(d + h.sz));
            return c1 == c2 ? p - beg : -1;
        }
        else
        if (h.id == CONFIG_HDR_CLR)
            p = d + h.sz + sizeof(cks_t);
        else
            return -1;
    }

    return -1;
}

void Config::init() {
    hdr_t h;
    // ищем валидный блок с данными
    auto p = efind(h);
    data_t d;
    if (p >= 0) {
        uint16_t sz = h.sz < sizeof(data_t) ? h.sz : sizeof(data_t);
        memcpy(&d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(CONFIG_ADDR + p + sizeof(hdr_t))), sz);
    }
    cfg._d      = d;
    cfg._chg    = false;
}

static bool esave(int p, const Config::data_t &data) {
    if (p < 0) {
        // в случае, если мы не нашли валидный блок
        // или новый после него не влезет,
        // то надо выполнить стирание всей страницы
        FLASH_EraseInitTypeDef e = { 0 };
        e.TypeErase = FLASH_TYPEERASE_PAGES;
	    e.Banks     = 1;
        e.Page      = 63;
        e.NbPages   = 1;
        uint32_t epage = 0;
        auto st = HAL_FLASHEx_Erase(&e, &epage);
        CONSOLE("HAL_FLASHEx_Erase: %d", st);
        if (st != HAL_OK)
            return false;
        p = 0;
    }
    else {
        // а если валидный блок всё же найден,
        // то пометим его как "пропустить"...
        // читаем dword, т.к. побайтно писать не сможем
        auto sz = reinterpret_cast<__IO hdr_t *>(p)->sz;
        auto d = *reinterpret_cast<__IO uint64_t *>(p);
        *reinterpret_cast<__IO uint8_t *>(d) = CONFIG_HDR_CLR;
        auto st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, p, d);
        CONSOLE("HAL_FLASH_Program: %d", st);
        if (st != HAL_OK)
            return false;

        // а под запись будем использовать уже следующий блок
        p += sizeof(hdr_t) + sz + sizeof(cks_t);
    }
    
    // подготавливаем данные под сохранение,
    // их надо выровнять до _FLASH_WBLK_SIZE
    size_t wcnt = (sizeof(hdr_t) + sizeof(Config::data_t) + sizeof(cks_t) + _FLASH_WBLK_SIZE - 1) / _FLASH_WBLK_SIZE;
    uint8_t d[wcnt * _FLASH_WBLK_SIZE] = { 0 };
    hdr_t h = {
        .id     = CONFIG_HDR_OK,
        .ver    = CONFIG_VER,
        .sz     = sizeof(Config::data_t)
    };
    memcpy(d, &h, sizeof(hdr_t));
    memcpy(d + sizeof(hdr_t), &data, sizeof(Config::data_t));
    cks_t c = { 0 };
    c.add(reinterpret_cast<const uint8_t *>(&data), sizeof(Config::data_t));
    memcpy(d + sizeof(hdr_t) + sizeof(Config::data_t), &c, sizeof(cks_t));

    // пишем подготовленные данные
    auto sz = sizeof(d);
    auto dd = d;
    for (; sz >= _FLASH_WBLK_SIZE; dd += _FLASH_WBLK_SIZE, sz -= _FLASH_WBLK_SIZE) {
        auto st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, p, *reinterpret_cast<uint64_t *>(d));
        CONSOLE("HAL_FLASH_Program: %d", st);
        if (st != HAL_OK)
            return false;
    }

    return true;
}

bool Config::save() {
    if (!_chg)
        return true;
    
    hdr_t h;
    // ищем валидный блок с данными
    auto p = efind(h);

    if ((p >= 0) && (CONFIG_ADDR + p + (sizeof(hdr_t) + sizeof(cks_t))*2 + h.sz + sizeof(Config::data_t) > CONFIG_AEND))
        // если блок валидный, но после него новый всё равно не влезет
        p = -1;
    
    // далее, приступаем к записи, для этого
    // надо разблокировать флеш-память
    auto st = HAL_FLASH_Unlock();
    CONSOLE("FLASH_Unlock: %d", st);
    if (st != HAL_OK)
        return false;

    bool ok = esave(p, _d);

    // всё сделали, пора обратно блокировать память
    st = HAL_FLASH_Lock();
    CONSOLE("HAL_FLASH_Lock: %d", st);

    if (!ok)
        return false;
    
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
