/*
    Internal flash
*/

#ifndef _sys_iflash_H
#define _sys_iflash_H

#include "../def.h"

#include <stdint.h>
#include <stddef.h>

// для stm32g43x:
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

#endif // _sys_iflash_H
