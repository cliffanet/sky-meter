
#include "iflash.h"
#include "stm32drv.h"
#include "cks.h"
#include "log.h"

#include <memory.h>


namespace iflash {
    bool write(int addr, const uint8_t *d, size_t sz) {
        auto a = addr;
        while (sz > 0) {
            uint8_t v[_FLASH_WBLK_SIZE];
            if (sz > _FLASH_WBLK_SIZE) {
                memcpy(v, d, _FLASH_WBLK_SIZE);
                d += _FLASH_WBLK_SIZE;
                sz -= _FLASH_WBLK_SIZE;
            }
            else {
                memcpy(v, d, sz);
                bzero(v+sz, _FLASH_WBLK_SIZE-sz);
                d += sz;
                sz = 0;
            }

            auto st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, *reinterpret_cast<uint64_t *>(v));
            CONSOLE("HAL_FLASH_Program[0x%06x]: %d, errno: %d", addr-_FLASH_BASE, st, st == HAL_OK ? 0 : HAL_FLASH_GetError());
            if (st != HAL_OK)
                return false;
            
            addr += _FLASH_WBLK_SIZE;
        }

        CONSOLE("saved OK on: 0x%06x", a-_FLASH_BASE);

        return true;
    }

    bool writecks(int addr, const uint8_t *d, size_t sz, size_t hdrsz) {
        uint8_t dd[sz+sizeof(cks_t)];
        memcpy(dd, d, sz);
        if ((hdrsz >= 0) && (hdrsz < sz)) {
            cks_t c(d+hdrsz, sz-hdrsz);
            memcpy(dd+sz, &c, sizeof(cks_t));
        }
        else
            bzero(dd+sz, sizeof(cks_t));

        return write(addr, dd, sizeof(dd));
    }

    bool readcks(int addr, uint8_t *d, size_t sz) {
        memcpy(d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(addr)), sz);
        cks_t c1(d, sz);
        cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(addr + sz));

        return c1 == c2;
    }

    bool validcks(int addr, size_t sz) {
        cks_t c1(const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(addr)), sz);
        cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(addr + sz));
        return c1 == c2;
    }

} // namespace iflash
