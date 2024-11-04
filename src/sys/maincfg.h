/*
    Config
*/

#ifndef _sys_config_H
#define _sys_config_H

#include "../../def.h"

#include <stdint.h>

// для stm32g43x:
//      - только одна банка,
//      - размер страницы 2k,
//      - всего страниц 64,
//      - запись по 8 байт
#define _FLASH_PAGE_SIZE    2048
#define _FLASH_PAGE_NUM     63
#define _FLASH_PAGE_ALL     64
#define _FLASH_WBLK_SIZE    8

#define _FLASH_WBLK_ALIGN(sz)   (static_cast<size_t>(((sz) + _FLASH_WBLK_SIZE - 1) / _FLASH_WBLK_SIZE) * _FLASH_WBLK_SIZE)

#define CONFIG_ADDR         (0x08000000 + (_FLASH_PAGE_NUM * _FLASH_PAGE_SIZE))
#define CONFIG_AEND         (0x08000000 + (_FLASH_PAGE_ALL * _FLASH_PAGE_SIZE))

#define CONFIG_HDR      '#'

#define CONFIG_VER      1

class Config {
    public:
        typedef struct __attribute__((__packed__)) {
            uint32_t    jmpcnt      = 0;        // кол-во прыжков
            bool        flip180     = false;    // развернуть экран/кнопки на 180
            uint8_t     contrast    = 10;       // контраст экрана
            bool        autognd     = true;     // автоподстройка уровня земли
            bool        altmeter    = true;     // показывать высоту в метрах (на высотах менее 1000)
            int16_t     altcorrect  = 0;        // превышение площадки приземления
            int16_t     :0;
            bool        lopwronjmp  = false;    // использовать энергосбер. режим (с уходом в сон на 100мс) в режиме прыжка
        } data_t;

        static void init();
        bool save();

        const data_t* operator->() const;
        data_t* operator*();
        bool changed() const { return _chg; }

        bool resetdefault();

    private:
        data_t _d;
        bool _chg = false;
};

extern Config cfg;

#endif // _sys_config_H
