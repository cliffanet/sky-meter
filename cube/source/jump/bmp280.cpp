
#include "bmp280.h"

#include "../sys/trwire.h"
#include "../sys/stm32drv.h"

extern SPI_HandleTypeDef hspi1;

BMP280::BMP280(TransWire &_dev) :
    _dev(_dev)
{  }

bool BMP280::read16le(uint8_t reg, uint16_t &v) {
    uint8_t d[2] = { 0 };
    if (!_dev.read(reg, d, sizeof(d))) {
        v = 0;
        return false;
    }
    v = (static_cast<uint16_t>(d[1]) << 8) | d[0];
    return true;
}

bool BMP280::read16le(uint8_t reg, int16_t &v) {
    uint8_t d[2] = { 0 };
    if (!_dev.read(reg, d, sizeof(d))) {
        v = 0;
        return false;
    }
    v = (static_cast<int16_t>(d[1]) << 8) | d[0];
    return true;
}

bool BMP280::read24(uint8_t reg, int32_t &v) {
    uint8_t d[3] = { 0 };
    if (!_dev.read(reg, d, sizeof(d))) {
        v = 0;
        return false;
    }
    v = (static_cast<uint32_t>(d[0]) << 16) | (static_cast<uint32_t>(d[1]) << 8) | d[0];
    return true;
}

bool BMP280::calib() {
    bool r[] = {
        read16le(REG_CAL_T1, _calib.t1),
        read16le(REG_CAL_T2, _calib.t2),
        read16le(REG_CAL_T3, _calib.t3),

        read16le(REG_CAL_P1, _calib.p1),
        read16le(REG_CAL_P2, _calib.p2),
        read16le(REG_CAL_P3, _calib.p3),
        read16le(REG_CAL_P4, _calib.p4),
        read16le(REG_CAL_P5, _calib.p5),
        read16le(REG_CAL_P6, _calib.p6),
        read16le(REG_CAL_P7, _calib.p7),
        read16le(REG_CAL_P8, _calib.p8),
        read16le(REG_CAL_P9, _calib.p9)
    };
    for (auto rr: r)
        if (!rr)
            return false;
    return true;
}

bool BMP280::tempfine(int32_t &v) {
    int32_t adc_T;
    if (!read24(REG_TEMPDATA, adc_T)) {
        v = 0;
        return false;
    }

    adc_T >>= 4;
    int32_t calt1 = _calib.t1;

    int32_t v1 = ((((adc_T >> 3) - (calt1 << 1))) * _calib.t2) >> 11;

    int32_t v2 = ((((adc_T >> 4) - calt1) * ((adc_T >> 4) - calt1)) >> 12);
    int32_t v3 = _calib.t3 >> 14;

    v = v1 + v2 * v3;
    return true;
}

bool BMP280::init() {
    if (!_dev.init())
        return false;

    calib();

    return
        setctrl() &&
        setconf() &&
        (chipid() == BMP280_CHIPID);
}

uint8_t BMP280::chipid() {
    return _dev.read8(REG_CHIPID);
}

uint8_t BMP280::version() {
    return _dev.read8(REG_VERSION);
}

uint8_t BMP280::status() {
    return _dev.read8(REG_STATUS);
}

bool BMP280::setctrl(ctrl_mode_t mode, ctrl_sampling_t smpltemp, ctrl_sampling_t smplpres) {
    return _dev.write8(
        REG_CONTROL,
        ((smpltemp & 0x07) << 5) |
        ((smplpres & 0x07) << 2) |
        (mode & 0x03)
    );
}

bool BMP280::setconf(ctrl_filter_t filter, ctrl_standby_t standby) {
    return _dev.write8(
        REG_CONFIG,
        ((standby & 0x07) << 5) |
        ((filter  & 0x07) << 2)
    );
}

bool BMP280::sleep() {
    return setctrl(BMP280::MODE_SLEEP, BMP280::SAMPLING_NONE, BMP280::SAMPLING_NONE);
}

bool BMP280::reset()
{
    return _dev.write8(REG_SOFTRESET, 0xB6);
}

bool BMP280::temp(float &v) {
    int32_t tfine;
    if (!tempfine(tfine)) {
        v = 0;
        return false;
    }
    return static_cast<float>((tfine * 5 + 128) >> 8) / 100;
}

bool BMP280::press(float &v) {
    int32_t tfine, adc_P;
    if (!tempfine(tfine) || !read24(REG_PRESSUREDATA, adc_P)) {
        v = 0;
        return false;
    }
    adc_P >>= 4;

    int64_t v1 = tfine - 128000;
    int64_t v2 = v1 * v1 * _calib.p6;
    v2 +=   (v1 * _calib.p5) << 17;
    v2 +=   static_cast<int64_t>(_calib.p4) << 35;
    v1 =    ((v1 * v1 * _calib.p3) >> 8) +
            ((v1 * _calib.p2) << 12);
    v1 =    (((static_cast<int64_t>(1) << 47) + v1) * _calib.p1) >> 33;

    if (v1 == 0) {
        // avoid exception caused by division by zero
        v = 0;
        return false;
    }
    
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = (static_cast<int64_t>(_calib.p9) * (p >> 13) * (p >> 13)) >> 25;
    v2 = (static_cast<int64_t>(_calib.p8) * p) >> 19;

    float pp = ((p + v1 + v2) >> 8) + (static_cast<int64_t>(_calib.p7) << 4);
    v = pp / 256;
    return true;
}
