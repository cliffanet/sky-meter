#ifndef _jump_bmp280_H
#define _jump_bmp280_H

#include <stdint.h>

class TransWire;

#define BMP280_CHIPID (0x58)

class BMP280 {
    enum {
        REG_CAL_T1 = 0x88,
        REG_CAL_T2 = 0x8A,
        REG_CAL_T3 = 0x8C,
        REG_CAL_P1 = 0x8E,
        REG_CAL_P2 = 0x90,
        REG_CAL_P3 = 0x92,
        REG_CAL_P4 = 0x94,
        REG_CAL_P5 = 0x96,
        REG_CAL_P6 = 0x98,
        REG_CAL_P7 = 0x9A,
        REG_CAL_P8 = 0x9C,
        REG_CAL_P9 = 0x9E,
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

    TransWire &_dev;

    uint16_t read16le(uint8_t reg);
    uint32_t read24(uint8_t reg);

    struct {
        uint16_t    t1;
        int16_t     t2;
        int16_t     t3;

        uint16_t    p1;
        int16_t     p2;
        int16_t     p3;
        int16_t     p4;
        int16_t     p5;
        int16_t     p6;
        int16_t     p7;
        int16_t     p8;
        int16_t     p9;
    } _calib;
    void calib();

    int32_t tempfine(); 

    public:
        BMP280(TransWire &_dev);

        bool init();

        uint8_t chipid();
        uint8_t version();
        uint8_t status();

        /** Oversampling rate for the sensor. */
        typedef enum {
            /** No over-sampling. */
            SAMPLING_NONE = 0x00,
            /** 1x over-sampling. */
            SAMPLING_X1 = 0x01,
            /** 2x over-sampling. */
            SAMPLING_X2 = 0x02,
            /** 4x over-sampling. */
            SAMPLING_X4 = 0x03,
            /** 8x over-sampling. */
            SAMPLING_X8 = 0x04,
            /** 16x over-sampling. */
            SAMPLING_X16 = 0x05
        } ctrl_sampling_t;

        /** Operating mode for the sensor. */
        typedef enum {
            /** Sleep mode. */
            MODE_SLEEP = 0x00,
            /** Forced mode. */
            MODE_FORCED = 0x01,
            /** Normal mode. */
            MODE_NORMAL = 0x03
        } ctrl_mode_t;
        bool setctrl(ctrl_mode_t mode, ctrl_sampling_t smpltemp, ctrl_sampling_t smplpres);

        /** Filtering level for sensor data. */
        typedef enum {
            /** No filtering. */
            FILTER_OFF = 0x00,
            /** 2x filtering. */
            FILTER_X2 = 0x01,
            /** 4x filtering. */
            FILTER_X4 = 0x02,
            /** 8x filtering. */
            FILTER_X8 = 0x03,
            /** 16x filtering. */
            FILTER_X16 = 0x04
        } ctrl_filter_t;

        /** Standby duration in ms */
        typedef enum {
            /** 1 ms standby. */
            STANDBY_MS_1 = 0x00,
            /** 62.5 ms standby. */
            STANDBY_MS_63 = 0x01,
            /** 125 ms standby. */
            STANDBY_MS_125 = 0x02,
            /** 250 ms standby. */
            STANDBY_MS_250 = 0x03,
            /** 500 ms standby. */
            STANDBY_MS_500 = 0x04,
            /** 1000 ms standby. */
            STANDBY_MS_1000 = 0x05,
            /** 2000 ms standby. */
            STANDBY_MS_2000 = 0x06,
            /** 4000 ms standby. */
            STANDBY_MS_4000 = 0x07
        } ctrl_standby_t;
        bool setconf(ctrl_filter_t filter, ctrl_standby_t standby);

        bool reset();

        float temp();
        float press();
};


#endif /* _jump_bmp280_H */
