/*
    Internal flash
*/

#ifndef _sys_iflash_H
#define _sys_iflash_H

#include "../def.h"

#include <stdint.h>
#include <stddef.h>

#if HWVER < 2

// для stm32g4xx:
//      - только одна банка,
//      - размер страницы 2k,
//      - всего страниц 64,
//      - запись по 8 байт
#define _FLASH_BASE         0x08000000
#define _FLASH_PAGE_SIZE    2048

#if defined(STM32G473xx)
#define _FLASH_PAGE_ALL     256
#elif defined(STM32G431xx)
#define _FLASH_PAGE_ALL     64
#endif

#define _FLASH_PAGE_ISBEG(addr)     ((((addr) - _FLASH_BASE) % _FLASH_PAGE_SIZE) == 0)

#define _FLASH_WBLK_TYPE    uint64_t
#define _FLASH_WBLK_SIZE    sizeof(_FLASH_WBLK_TYPE)
#define _FLASH_WBLK_ERASE   0xffffffffffffffff

#define _FLASH_WBLK_ISERASE(addr)   (*reinterpret_cast<__IO _FLASH_WBLK_TYPE *>(addr) == _FLASH_WBLK_ERASE)

#define _FLASH_WBLK_ALIGN(sz)   (static_cast<size_t>(((sz) + _FLASH_WBLK_SIZE - 1) / _FLASH_WBLK_SIZE) * _FLASH_WBLK_SIZE)
#define _FLASH_PAGE_ALIGN(sz)   (static_cast<size_t>(((sz) + _FLASH_PAGE_SIZE - 1) / _FLASH_PAGE_SIZE) * _FLASH_PAGE_SIZE)

namespace iflash {
    class Unlocker {
        bool ok;
        public:
            Unlocker();
            ~Unlocker();
            operator bool() const { return ok; };
    };

    bool erase(int addr);

    // простая запись данных размером sz
    // во внутреннюю память, начиная с адреса addr (абсолютное значение)
    // итоговый записанный размер будет равен: _FLASH_WBLK_ALIGN(sz)
    bool write(int addr, const uint8_t *d, size_t sz);

    template <typename T>
    bool write(int addr, const T &d) {
        return write(addr, reinterpret_cast<const uint8_t *>(&d), sizeof(T));
    }

    // запись с автовычислением контролькой суммы.
    // Будет выполнена запись данных размером sz + 2 (размер контролькой суммы).
    // во внутреннюю память, начиная с адреса addr (абсолютное значение)
    // итоговый записанный размер будет равен: _FLASH_WBLK_ALIGN(sz+2)
    // контрольная сумма вычисляется с позиции hdrsz до конца блока -
    // т.к. размер hdrsz в начале блока не попадёт под вычисление cks
    bool writecks(int addr, const uint8_t *d, size_t sz, size_t hdrsz = 0);

    template <typename T>
    bool writecks(int addr, const T &d, size_t hdrsz = 0) {
        return writecks(addr, reinterpret_cast<const uint8_t *>(&d), sizeof(T), hdrsz);
    }

    // читает sz байт из внутренней флеш в d, а так же sizeof(cks_t) байт следом за ними,
    // по которым сверяет контрольную сумму
    bool readcks(int addr, uint8_t *d, size_t sz);

    template <typename T>
    bool readcks(int addr, T &d) {
        return readcks(addr, reinterpret_cast<uint8_t *>(&d), sizeof(T));
    }

    // аналогично readcks, но выполняет только проверку соответствия sck
    // во внутренней флеш, без копирования самих данных
    bool validcks(int addr, size_t sz);
} // namespace iflash



#else // if HWVER < 2

#define _FLASH_BASE         0x08000000
// пока что предполагаем, что все доступные нам страницы одинакового размера
#define _FLASH_PAGE_SIZE    (128*1024)

// т.к. страницы бывают разного размера на некоторых чипах,
// то отталкиваться будем не от номера страницы, а от её адреса (без учёта базового смещения _FLASH_BASE)
#define _FLASH_PAGE_BEG     0x40000
#define _FLASH_PAGE_END     0x7ffff
// номер страницы нам всё-таки нужен для erase
#define _FLASH_PAGENUM_BEG  6

// размер блока для записи
#define _FLASH_WBLK_SIZE            sizeof(uint32_t)
#define _FLASH_WBLK_ALIGN(sz)       (static_cast<size_t>(((sz) + _FLASH_WBLK_SIZE - 1) / _FLASH_WBLK_SIZE) * _FLASH_WBLK_SIZE)
#define _FLASH_WRITE(addr, ref)     HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *reinterpret_cast<uint32_t *>(ref))

// размер заголовка страницы
// не может быть меньше 8 и обязательно кратен _FLASH_WBLK_SIZE
#define _FLASH_PHDR_SIZE            _FLASH_WBLK_ALIGN(8)
// оставшееся место под данные
#define _FLASH_PAGE_DSZ             (_FLASH_PAGE_SIZE-_FLASH_PHDR_SIZE)


namespace iflash {

    // типы(заголовки) записей
    #define _FLASH_TYPE_PAGEHDR     '#'
    #define _FLASH_TYPE_PAGEEND     '~'
    #define _FLASH_TYPE_CONFIG      '$'
    #define _FLASH_TYPE_LOGBOOK     '@'

    inline bool validtype(uint8_t t) {
        return
            (t == _FLASH_TYPE_PAGEEND) ||
            (t == _FLASH_TYPE_CONFIG) ||
            (t == _FLASH_TYPE_LOGBOOK);
    }

    class Page {
        uint32_t _a;
    public:
        Page();
        Page(uint32_t addr);

        operator bool() const;
        //uint32_t addr() const { return _FLASH_BASE + _a; }
        uint32_t addr(uint32_t a) const { return _FLASH_BASE + _a + _FLASH_PHDR_SIZE + a; }
        uint32_t num() const;

        Page prev() const;
        Page next() const;

        bool create(uint32_t num);
    };

    class Rec {
        Page _p;
        uint32_t _a;
    public:
        Rec();
        Rec(Page page, uint32_t addr);

        operator bool() const;
        Page page() const { return _p; }
        uint32_t a()const { return _a; }
        uint8_t type() const;
        uint8_t sz() const;
        uint8_t fullsz() const { return _FLASH_WBLK_ALIGN(5 + sz()); };
        uint8_t pgavail() const { return _FLASH_PAGE_DSZ > _a + 5 ? _FLASH_PAGE_DSZ - (_a + 5) : 0; }
        bool    pgavail(size_t sz) const { return _FLASH_WBLK_ALIGN(_a + sz + 5) <= _FLASH_PAGE_DSZ; }
        bool    ispgbeg() const { return _a == 0; }
        bool    ispgend() const { return (_FLASH_WBLK_ALIGN(_a + 5) > _FLASH_PAGE_DSZ) || (*this && (type() == _FLASH_TYPE_PAGEEND)); };

        uint32_t addr() const { return _p.addr(_a); }
        bool valid();

        bool read(uint8_t *d, size_t _sz);
        template <typename T>
        bool read(T &d) {
            return read(reinterpret_cast<uint8_t *>(&d), sizeof(T));
        }

        Rec prev() const;
        Rec next() const;
        Rec pglast() const;

        Rec prevme() const;
        Rec nextme() const;
    };

    typedef enum {
        TYPE_ANY = 0,
        TYPE_CONFIG,
        TYPE_LOGBOOK,
        TYPE_SIZE
    } rtype_t;
    void init();
    Rec last(rtype_t type);

    Rec save(uint8_t type, const uint8_t *d, size_t sz);
    template <typename T>
    Rec save(uint8_t type, const T &d) {
        return save(type, reinterpret_cast<const uint8_t *>(&d), sizeof(T));
    }

} // namespace iflash


#endif // if HWVER < 2

#endif // _sys_iflash_H
