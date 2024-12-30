/*
    Alt calculate
*/

#include "altcalc.h"
#include <math.h>

/*******************************
 *          AltBuf: базовые методы
 *******************************/

void AltBuf::tick(float alt, uint16_t ms) {
    if (_buf.empty())
        _alt0 = 0;
    else
    if (_buf.full())
        _alt0   = _buf.frst().alt;

    _buf.push({
        ms,
        alt
    });
}

const uint32_t AltBuf::interval() const {
    uint32_t interval = 0;
    for (const auto &d: _buf)
        interval += d.ms;
    return interval;
}

const double AltBuf::sqdiff() const {
    auto s = avg().speed();
    uint32_t x = 0;
    double sq = 0;
    for (const auto &d: _buf) {
        x += d.ms;
        double altd = s * x / 1000 + _alt0 - d.alt;
        sq += altd * altd;
    }

    return sqrt(sq / _buf.size());
}

/*******************************
 *          AltBuf: субклассы
 *******************************/

AltBuf::VAvg::VAvg() :
    _interval(0),
    _alt(0),
    _speed(0)
{ }

AltBuf::VAvg::VAvg(const AltBuf &ab):
    VAvg()
{
    // самый первый _interval в массиве - не должен входить в суммарный интервал при определении скорости
    double alt = ab._alt0;
    for (const auto &d: ab._buf) {
        _interval += d.ms;
        alt += d.alt;
    }
    _alt = alt / (ab._buf.size()+1);
    _speed = (_interval > 0) ?
        // _interval м.б. равен нулю, даже при src.size()!=0,
        // но при _interval!=0 массив не может оказаться пустым
        (ab._buf.last().alt - ab._alt0) * 1000 / _interval :
        0;
}

AltBuf::VApp::VApp(const AltBuf &ab) :
    VAvg()
{
    // считаем коэффициенты линейной аппроксимации
    // sy - единственный ненулевой элемент в нулевой точке - равен _alt0
    double sy = ab._alt0, sxy = 0, sx = 0, sx2 = 0;
    // количество элементов на один больше, чем src.size(),
    // т.к. нулевой элемент находится не в src, а в alt0
    uint32_t x = 0, n = ab._buf.size() + 1;
    
    for (const auto &d: ab._buf) {
        if (d.ms == 0)
            continue;
        
        x   += d.ms;
        sx  += x;
        sx2 += x*x;
        sy  += d.alt;
        sxy += x*d.alt;
    }
    
    // коэфициенты
    double a = (sxy*n - (sx*sy)) / (sx2*n-(sx*sx));
    double b = (sy - (a*sx)) / n;

    _interval   = x;
    _alt        = a * x + b;
    _speed      = a * 1000;
}

AltBuf::VSavg::VSavg(const AltBuf &ab, uint8_t sz) :
    VAvg()
{
    // самый первый _interval в массиве - не должен входить в суммарный интервал при определении скорости
    if (sz >= ab._buf.size())
        sz = ab._buf.size()-1;
    double alt = 0;
    uint8_t n = 0;
    for (const auto &d: ab._buf) {
        alt += d.alt;
        n++;
        if (n > sz) break;
        _interval += d.ms;
    }
    _alt = alt / n;
    _speed = (_interval > 0) ?
        // _interval м.б. равен нулю, даже при src.size()!=0,
        // но при _interval!=0 массив не может оказаться пустым
        (ab._buf.last().alt - ab._buf[sz].alt) * 1000 / _interval :
        0;
}

float press2alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
}

void AltCalc::tick(float press, uint16_t ms) {
    bool full = _b->full();
    _press.push(press);
    
    if (!full)
        gndreset();

    _b.tick(press2alt(_pressgnd, press), ms);
}

void AltCalc::gndreset() {
    // пересчёт _pressgnd
    double pr = 0;
    for (auto &p: _press)
        pr += p;
    _pressgnd = pr / _press.size();
}

void AltCalc::gndset(float press, uint16_t ms) {
    _pressgnd = press;
    // если мы делаем gndset ещё до полной инициализации,
    // забиваем массив данных текущим давлением, 
    // тем самым мы принудительно завершаем инициализацию
    while (!_press.full())
        _press.push(press);
    while (!_b->full())
        _b.tick(press, ms);
}

/*******************************
 *          AltDirect
 *******************************/

void AltDirect::tick(const AltBuf &ab) {
    dir_t dir =
        !ab.full() ?
            INIT :
        ab.app().speed() > AC_SPEED_FLAT ?
            UP :
        ab.app().speed() < -AC_SPEED_FLAT ?
            DOWN :
            FLAT;
    
    if (_mode == dir) {
        _cnt ++;
        _tm += ab.tm();
    }
    else {
        _mode = dir;
        _cnt = 0;
        _tm = 0;
    }
}

void AltDirect::tick(const AltCalc &ac) {
    tick(ac.buf());
}

void AltDirect::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltState
 *******************************/

void AltState::tick(const AltBuf & ab) {
    st_t st =
        !ab.full() ?
            INIT :
        ab.app().speed() > AC_SPEED_FLAT ?
            (
                ab.app().alt() < 40 ?
                    TAKEOFF40 :
                    TAKEOFF
            ) :
        
        (ab.app().alt() < 50) && 
        (ab.avg().speed() < 0.5) &&
        (ab.avg().speed() > -0.5) ?
            GROUND :
        
        (ab.app().speed() < -AC_SPEED_FLAT) &&
        (ab.app().alt() < 100) ?
            LANDING :
        
        (ab.app().speed() < -AC_SPEED_FREEFALL_I) || 
        ((_mode == FREEFALL) && (ab.app().speed() < -AC_SPEED_FREEFALL_O)) ?
            FREEFALL :
        
        (ab.app().speed() < -AC_SPEED_CANOPY_I) ||
        ((_mode == CANOPY) && (ab.app().speed() < -AC_SPEED_FLAT)) ?
            CANOPY :

            _mode;
    
    if (_mode == st) {
        _cnt ++;
        _tm += ab.tm();
    }
    else {
        _mode = st;
        _cnt = 0;
        _tm = 0;
    }
}

void AltState::tick(const AltCalc &ac) {
    tick(ac.buf());
}

void AltState::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltSqBig
 *******************************/

void AltSqBig::tick(const AltBuf &ab) {
    _val = ab.sqdiff();
    // большая турбуленция (высокое среднеквадратическое отклонение)
    if (!_big && (_val >= AC_SQBIG_THRESH)) {
        _big = true;
        _cnt = 0;
        _tm = 0;
    }
    else
    if (_big && (_val < AC_SQBIG_MIN)) {
        _big = false;
        _cnt = 0;
        _tm = 0;
    }
    else {
        _cnt ++;
        _tm += ab.tm();
    }
}

void AltSqBig::tick(const AltCalc &ac) {
    tick(ac.buf());
}

void AltSqBig::reset() {
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltProfile
 *******************************/

AltProfile::AltProfile() :
    _prof(NULL),
    _sz(0),
    _icnt(0)
{ }

AltProfile::AltProfile(const prof_t *profile, uint8_t sz, uint8_t icnt) :
    _prof(profile),
    _sz(sz),
    _icnt(icnt)
{ }

void AltProfile::tick(const AltBuf::VAvg &avg, uint32_t tm) {
    if ((_prof == NULL) || (_sz == 0))
        return;
    
    if (_c == 0) {
        // стартуем определять вхождение в профиль по средней скорости
        const auto &p = _prof[_c];
        if ((avg.speed() >= p.min) && (avg.speed() <= p.max)) {
            _c ++;
            _alt = avg.alt();
            _tm = 0;
            _cnt = 0;
        }
        return;
    }

    // теперь считаем интервал от самого первого пункта в профиле
    _cnt ++;
    _tm += tm;

    if (_c >= _sz) return;

    // далее - каждые 10 тиков от старта будем проверять каждый следующий пункт профиля
    if ((_cnt/_c) < _icnt)
        return;

    auto alt = avg.alt() - _alt;
    const auto &p = _prof[_c];
    if ((alt >= p.min) && (alt <= p.max)) {
        // мы всё ещё соответствуем профилю начала прыга
        _c ++;
        _alt = avg.alt();
        return;
    }

    // мы вышли за пределы профиля
    if ((avg.speed() >= p.min) && (avg.speed() <= p.max)) {
        // но мы ещё в рамках старта профиля
        _c ++;
        _alt = avg.alt();
    }
    else {
        // выход из профиля полный - полный сброс процесса
        _c = 0;
        _alt = 0;
    }

    if (_cnt > 0) {
        _cnt = 0;
        _tm = 0;
    }
}

void AltProfile::reset() {
    _c      = 0;
    _alt    = 0;
    _cnt    = 0;
    _tm     = 0;
}

void AltProfile::clear() {
    reset();
    _prof   = NULL;
    _sz     = 0;
    _icnt   = 0;
}

/*******************************
 *          AltJmp
 *******************************/

void AltJmp::tick(const AltBuf &ab) {
    auto m = _mode;
    auto tm = ab.tm();
    auto avg = ab.avg();

    if ((_mode != TAKEOFF) && !_ff.empty())
        _ff.clear();

    switch (_mode) {
        case INIT:
            if (ab.full()) {
                m = GROUND;
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
        
        case GROUND:
            if ((avg.speed() > AC_SPEED_FLAT) || (avg.alt() > 100)) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_TOFF_COUNT) && (_c_tm >= AC_JMP_TOFF_TIME))
                    m = TAKEOFF;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
            
        case TAKEOFF: {
                static const AltProfile::prof_t profstr[] = {
                    { -50,  -15 },
                    { -100, -30 },
                    { -100, -40 },
                };
                static const AltProfile::prof_t profile[] = {
                    { -50,  -10 },
                    { -100, -23 },
                    { -100, -18 },
                    { -100, -8 },
                    { -100, -4 },
                    { -100, -2 }
                };
                if (_ff.empty())
                    _ff = _strict ?
                        AltProfile(profstr, 3) :
                        AltProfile(profile, 6);
                
                _ff.tick(ab.sav() /* avg */, tm);
                if (_ff.full()) {
                    // профиль закончился, принимаем окончательное решение
                    m = !_strict && (avg.speed() >= -AC_JMP_CNP_SPEED) ? CANOPY : FREEFALL;
                    // скорость средняя задерживается примерно на половину-весь размер буфера
                    _c_cnt= _ff.cnt() + 10 /* + AC_DATA_COUNT */;
                    _c_tm = _ff.tm();
                }
            }
            break;
            
        case FREEFALL:
            // Переход в режим CNP после начала прыга,
            // Дальше только окончание прыга может быть, даже если начнётся снова FF,
            // Для jmp только такой порядок переходов,
            // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
            if (avg.speed() >= -AC_JMP_CNP_SPEED) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_CNP_COUNT) && (_c_tm >= AC_JMP_CNP_TIME))
                    m = CANOPY;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
            
        case CANOPY:
            if (
                    (avg.alt() < 50) &&
                    // разницы +-0.5 (между началом и концом = скорость)
                    // может быть недостаточно.
                    // некоторые датчики шумят чуть сильнее
                    (avg.speed() < 0.7) &&
                    (avg.speed() > -0.7)
                ) {
                _c_cnt++;
                _c_tm += tm;
                if ((_c_cnt >= AC_JMP_GND_COUNT) && (_c_tm >= AC_JMP_GND_TIME))
                    m = GROUND;
            }
            else
            if (_c_cnt > 0) {
                _c_cnt= 0;
                _c_tm = 0;
            }
            break;
    }

    if (m == _mode) {
        _cnt ++;
        _tm += tm;
    }
    else {
        _mode   = m;
        _cnt    = _c_cnt;
        _tm     = _c_tm;
        // _c_cnt и _c_tm нельзя сбрасывать при изменении _mode,
        //  т.к. именно сразу после изменения _mode нам нужны эти значения.
        // Если хочется надёжности сброса этих счётчиков, то лучше это сделать
        // отдельным методом.
        //_c_cnt  = 0;
        //_c_tm   = 0;
    }
}

void AltJmp::tick(const AltCalc &ac) {
    tick(ac.buf());
}

void AltJmp::reset(mode_t m) {
    _mode   = m;
    _cnt    = 0;
    _tm     = 0;
    resetnew();
}

void AltJmp::resetnew() {
    _c_cnt  = 0;
    _c_tm   = 0;
}

/*******************************
 *          AltStrict
 *******************************/

void AltStrict::tick(const AltBuf &ab) {
    auto tm = ab.tm();
    _prof.tick(ab.sav(5), tm);
    _dir.tick(ab);

    bool chg =
        _mode == AltJmp::INIT ?
            ab.full() :
            _prof.full();
    
    if (!chg && (_mode > AltJmp::GROUND)) {
        auto avg = ab.avg();
        chg = 
            (avg.alt() < 50) && 
            (_dir.mode() == AltDirect::FLAT) &&
            (_dir.cnt() > 200);
        if (chg)
            _nxt = AltJmp::GROUND;
    }

    if (!chg) {
        _cnt ++;
        _tm += tm;
        return;
    }

    // профиль закончился успешно
    _mode   = _nxt;
    if (_prof.empty()) {
        _cnt    = 0;
        _tm     = 0;
    }
    else {
        // скорость средняя задерживается примерно на половину-весь размер буфера
        _cnt    = _prof.cnt() + 10 /* + AC_DATA_COUNT */;
        _tm     = _prof.tm();
    }

    switch (_mode) {
        case AltJmp::GROUND: {
                static const AltProfile::prof_t to_toff[] = {
                    { 1,  50 },
                    { 1,  50 },
                    { 2,  50 },
                    { 2,  50 },
                    { 2,  50 },
                };
                _prof   = AltProfile(to_toff, 5);
                _nxt    = AltJmp::TAKEOFF;
            }
            break;

        case AltJmp::TAKEOFF: {
                static const AltProfile::prof_t to_ff[] = {
                    { -50,  -10 },
                    { -100, -23 },
                    { -100, -35 },
                    { -100, -35 }
                };
                _prof   = AltProfile(to_ff, 4);
                _nxt    = AltJmp::FREEFALL;
            }
            break;

        case AltJmp::FREEFALL: {
                static const AltProfile::prof_t to_cnp[] = {
                    { -35,  10 },
                    { -15,  10 },
                    { -15,  10 },
                    { -15,  5 }
                };
                _prof   = AltProfile(to_cnp, 4);
                _nxt    = AltJmp::CANOPY;
            }
            break;

        case AltJmp::CANOPY: {
                _prof.clear();
                _nxt    = AltJmp::GROUND;
        }
    }
}

void AltStrict::tick(const AltCalc &ac) {
    tick(ac.buf());
}

void AltStrict::reset() {
    _mode = AltJmp::INIT;
    _cnt = 0;
    _tm = 0;
}

/*******************************
 *          AltSleep
 *******************************/

#include "../sys/log.h"
void AltSleep::tick(float press) {
    if (_istoff || (_pressgnd == 0)) {
        clear();
        _pressgnd = press;
        return;
    }

    float alt = press2alt(_pressgnd, press);
    CONSOLE("_pressgnd: %.2f, press: %.2f, alt: %.2f, _altlast: %.2f, _toffcnt: %d", _pressgnd, press, alt, _altlast, _toffcnt);

    if (alt > 100) {
        _toffcnt = 0;
        _istoff = true;
        return;
    }
    if (alt > (_altlast + 0.7)) {
        if (_toffcnt < 0)
            _toffcnt = 0;
        _toffcnt ++;
        if (((_toffcnt >= 5) && (alt > 80)) || (_toffcnt >= 150)) {
            CONSOLE("is toff");
            _toffcnt = 0;
            _istoff = true;
            return;
        }
    }
    else {
        if (_toffcnt > 0)
            _toffcnt --;
        _toffcnt --;
        if (_toffcnt <= -10) {
            CONSOLE("gnd reset");
            _pressgnd = press;
            _toffcnt = 0;
        }
    }
    
    _altlast = alt;
}

void AltSleep::clear() {
    _pressgnd = 0;
    _altlast = 0;
    _toffcnt = 0;
    _istoff = false;
}
