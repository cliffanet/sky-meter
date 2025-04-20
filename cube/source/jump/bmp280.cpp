
#include "bmp280.h"

#include "../sys/trwire.h"
#include "../sys/buf.h"

BMP280::BMP280(TransWire &_dev) :
    _dev(_dev)
{  }

/*
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
*/

bool BMP280::calib() {
    uint8_t d[26];
    if (!_dev.read(REG_CAL, d, sizeof(d)))
        return false;

    bufrx b(d, sizeof(d));

    b.u16le(dig_T1);
    b.d16le(dig_T2);
    b.d16le(dig_T3);

    b.u16le(dig_P1);
    b.d16le(dig_P2);
    b.d16le(dig_P3);
    b.d16le(dig_P4);
    b.d16le(dig_P5);
    b.d16le(dig_P6);
    b.d16le(dig_P7);
    b.d16le(dig_P8);
    b.d16le(dig_P9);
    
    return true;
}

/*
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
*/

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

/*
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
*/

bool BMP280::meas(float &press, float &temp) {
    uint8_t d[6];
    // datasheet настоятельно рекомендует получать
    // температуру и давление одной командой, в этом
    // случае чип гарантирует, что оба значения будут
    // из одного цикла измерений (согласованные между собой)
    if (!_dev.read(REG_TEMPPRES, d, sizeof(d)))
        return false;

    bufrx b(d, sizeof(d));

    typedef int32_t BMP280_S32_t;
    BMP280_S32_t t_fine, adc_T, adc_P;
    b.d24(adc_P);   adc_P >>= 4;
    b.d24(adc_T);   adc_T >>= 4;

    // код мат-логики взят из datasheet, имена переменных,
    // в т.ч. калибровочных подогнаны под него, чтобы
    // минимизировать исправления и опечатки из-за них

#ifdef BMP280_FIXEDPOINTMATH
    typedef uint32_t BMP280_U32_t;

    {
        BMP280_S32_t var1, var2;
        var1 = ((((adc_T>>3) - ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
        var2 = (((((adc_T>>4) - ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_t)dig_T1))) >> 12) *
                ((BMP280_S32_t)dig_T3)) >> 14;
        t_fine = var1 + var2;
        float T = (t_fine * 5 + 128) >> 8;
        temp = T / 100;
    }
    {
        BMP280_S32_t var1, var2;
        BMP280_U32_t p;
        var1 = (((BMP280_S32_t)t_fine)>>1) - (BMP280_S32_t)64000;
        var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((BMP280_S32_t)dig_P6);
        var2 = var2 + ((var1*((BMP280_S32_t)dig_P5))<<1);
        var2 = (var2>>2)+(((BMP280_S32_t)dig_P4)<<16);
        var1 = (((dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((BMP280_S32_t)dig_P2) * var1)>>1))>>18;
        var1 =((((32768+var1))*((BMP280_S32_t)dig_P1))>>15);
        if (var1 == 0)
            return 0; // avoid exception caused by division by zero
        
        p = (((BMP280_U32_t)(((BMP280_S32_t)1048576)-adc_P)-(var2>>12)))*3125;
        if (p < 0x80000000) {
            p = (p << 1) / ((BMP280_U32_t)var1);
        }
        else {
            p = (p / (BMP280_U32_t)var1) * 2;
        }
        var1 = (((BMP280_S32_t)dig_P9) * ((BMP280_S32_t)(((p>>3) * (p>>3))>>13)))>>12;
        var2 = (((BMP280_S32_t)(p>>2)) * ((BMP280_S32_t)dig_P8))>>13;
        press = (BMP280_U32_t)((BMP280_S32_t)p + ((var1 + var2 + dig_P7) >> 4));
    }

#else // ifdef BMP280_FIXEDPOINTMATH

    {
        double var1, var2;
        var1 = (((double)adc_T)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2);
        var2 = ((((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0) *
        (((double)adc_T)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3);
        t_fine = (BMP280_S32_t)(var1 + var2);
        temp = (var1 + var2) / 5120.0;
    }
    {
        double var1, var2, p;
        var1 = ((double)t_fine/2.0) - 64000.0;
        var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
        var2 = var2 + var1 * ((double)dig_P5) * 2.0;
        var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
        var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
        var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
        if (var1 == 0.0)
            return 0; // avoid exception caused by division by zero
        
        p = 1048576.0 - (double)adc_P;
        p = (p - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = ((double)dig_P9) * p * p / 2147483648.0;
        var2 = p * ((double)dig_P8) / 32768.0;
        press = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
    }

#endif // ifdef BMP280_FIXEDPOINTMATH

    return true;
}
