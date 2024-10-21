#ifndef _jump_bmp280_H
#define _jump_bmp280_H

class BMP280 {
    enum {
        REG_DIG_T1 = 0x88,
        REG_DIG_T2 = 0x8A,
        REG_DIG_T3 = 0x8C,
        REG_DIG_P1 = 0x8E,
        REG_DIG_P2 = 0x90,
        REG_DIG_P3 = 0x92,
        REG_DIG_P4 = 0x94,
        REG_DIG_P5 = 0x96,
        REG_DIG_P6 = 0x98,
        REG_DIG_P7 = 0x9A,
        REG_DIG_P8 = 0x9C,
        REG_DIG_P9 = 0x9E,
        REG_CHIPID = 0xD0,
        REG_VERSION = 0xD1,
        REG_SOFTRESET = 0xE0,
        REG_CAL26 = 0xE1, /**< R calibration = 0xE1-0xF0 */
        REG_STATUS = 0xF3,
        REG_CONTROL = 0xF4,
        REG_CONFIG = 0xF5,
        REG_PRESSUREDATA = 0xF7,
        REG_TEMPDATA = 0xFA,
    };
};


#endif /* _jump_bmp280_H */
