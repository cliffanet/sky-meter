/*
    Config
*/

#ifndef _sys_config_H
#define _sys_config_H

#include "../../def.h"

#include <stdint.h>

#define CONFIG_ADDR         (_FLASH_BASE + ((_FLASH_PAGE_ALL-1) * _FLASH_PAGE_SIZE))
#define CONFIG_AEND         (_FLASH_BASE + (_FLASH_PAGE_ALL * _FLASH_PAGE_SIZE))

#define CONFIG_HDR      '#'

#define CONFIG_VER      1

class Config {
    public:
        typedef struct __attribute__((__packed__)) {
            uint32_t    jmpcnt      = 0;        // кол-во прыжков
            uint32_t    lbaddr      = 0;        // абсолютный адрес во внутренней флеш, первый свободный для записи logbook;
                                                    // относительный лучше не использовать, т.к. стартовые адреса могут меняться
                                                    // в разных версиях прошивки
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
