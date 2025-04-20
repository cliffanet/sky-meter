#ifndef _jump_bmp280_H
#define _jump_bmp280_H

#include <stdint.h>

class TransWire;

#define BMP280_CHIPID (0x58)

// использовать целочисленную математику, если процессор не умеет float
#define BMP280_FIXEDPOINTMATH

class BMP280 {
    enum {
        REG_CAL     = 0x88,
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
        REG_TEMPPRES = 0xF7,
        REG_PRESSUREDATA = 0xF7,
        REG_TEMPDATA = 0xFA,
    };

    TransWire &_dev;

    /*
    bool read16le(uint8_t reg, uint16_t &v);
    bool read16le(uint8_t reg, int16_t &v);
    bool read24(uint8_t reg, int32_t &v);
    */

    uint16_t    dig_T1;
    int16_t     dig_T2;
    int16_t     dig_T3;

    uint16_t    dig_P1;
    int16_t     dig_P2;
    int16_t     dig_P3;
    int16_t     dig_P4;
    int16_t     dig_P5;
    int16_t     dig_P6;
    int16_t     dig_P7;
    int16_t     dig_P8;
    int16_t     dig_P9;
    bool calib();

    bool tempfine(int32_t &v);

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
        bool setctrl(ctrl_mode_t mode = MODE_NORMAL, ctrl_sampling_t smpltemp = SAMPLING_X16, ctrl_sampling_t smplpres = SAMPLING_X16);

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
        bool setconf(ctrl_filter_t filter = FILTER_OFF, ctrl_standby_t standby = STANDBY_MS_1);

        bool sleep();
        bool reset();

        /*
        bool temp(float &v);
        bool press(float &v);
        */

        bool meas(float &press, float &temp);
        inline bool meas(float &press) { float temp; return meas(press, temp); }
};


#endif /* _jump_bmp280_H */
