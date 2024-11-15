
#include "proc.h"
#include "bmp280.h"
#include "altcalc.h"
#include "../sys/trwire.h"
#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"
#include "../sys/log.h"
#include "../view/page.h"

#include <cmath>
#include "string.h"

static TransSPI _spi(GPIOB, GPIO_PIN_0);
static BMP280 _bmp(_spi);

static AltCalc _ac;
static AltJmp _jmp(true);
static AltSqBig _sq;
static AltStrict _jstr;

static uint8_t page = 0;

#ifdef USE_JMPTRACE
typedef struct {
    int alt;
    bool ismode;
    bool mchg;
} log_t;
static ring<log_t, 3*60*10> _log;
static int _lmin = 0, _lmax = 0;
#endif // USE_JMPTRACE

    //-----------------------------------
    //---           draw
    //-----------------------------------

    static int16_t alt() {
        // Для отображения высоты app() лучше подходит,
        // т.к. она точнее показывает текущую высоту,
        // опаздывает от неё меньше, чем avg()
        int16_t alt = round(_ac.app().alt());
        int16_t o = alt % ALT_STEP;
        alt -= o;
        if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
        return alt;
    }

    static const char *strtm(char *s, uint32_t tm) {
        tm /= 1000;
        auto sec = tm % 60;
        tm -= sec;

        if (tm <= 0) {
            sprn("%d s", sec);
            return s;
        }

        tm /= 60;
        auto min = tm % 60;
        tm -= min;

        if (tm <= 0) {
            sprn("%d:%02d", min, sec);
            return s;
        }

        sprn("%d:%02d:%02d", tm/60, min, sec);
        return s;
    }

    static const char *modestr(char *s, AltJmp::mode_t m) {
        const char *pstr =
            m == AltJmp::INIT       ? "INIT" :
            m == AltJmp::GROUND     ? "GND" :
            m == AltJmp::TAKEOFF    ? "TOFF" :
            m == AltJmp::FREEFALL   ? "FF" :
            m == AltJmp::CANOPY     ? "CNP" : "-";
        strcpy(s, pstr);
        return s;
    }

    //-----------------------------------
    static void _draw(DSPL_ARG) {
        Dspl::drawBatt(u8g2);
        Dspl::drawClock(u8g2);
        
        char s[strsz], m[strsz], t[strsz];

        // info
        DSPL_FONT(u8g2_font_6x13B_tr);

        uint8_t y = 40;
        sprn("%.1f m/s", _ac.avg().speed());
        DSPL_STR(static_cast<int>(DSPL_S_RIGHT(s))+60-DSPL_DWIDTH, y, s);

        y += DSPL_S_HEIGHT+2;
        sprn("%s (%s)", modestr(m, _jmp.mode()), strtm(t, _jmp.tm()));
        DSPL_STR(0, y, s);

        y += DSPL_S_HEIGHT+2;
        if (_jmp.newtm() > 0) {
            sprn("new: %s (%d)", strtm(t, _jmp.newtm()), _jmp.newcnt());
            DSPL_STR(0, y, s);
        }

        y += DSPL_S_HEIGHT+2;
        if (_jmp.ff().active()) {
            sprn("ff: %d", _jmp.ff().num());
            DSPL_STR(0, y, s);
        }

        y += DSPL_S_HEIGHT+2;
        sprn("q:%0.1f(%s)", _sq.val(), strtm(t, _sq.tm()));
        DSPL_STR(0, y, s);

        sprn("s[%d]:%s(%s)", _jstr.prof().num(), modestr(m, _jstr.mode()), strtm(t, _jstr.tm()));
        DSPL_STR(DSPL_S_RIGHT(s)-15, DSPL_DHEIGHT-1, s);
        
        switch (page) {
            case 0: {
                // alt
                DSPL_FONT(u8g2_font_logisoso62_tn);
                sprn("%d", alt());
                DSPL_STR(DSPL_S_RIGHT(s), 80, s);

            }
            break;
#ifdef USE_JMPTRACE
            case 1: {
                // alt
                DSPL_FONT(u8g2_font_fub20_tr);

                int y = DSPL_S_HEIGHT;
                sprn("%d", alt());
                DSPL_STR(90-DSPL_S_WIDTH(s), y, s);

                // вертикальная шкала
                DSPL_LINE(DSPL_DWIDTH-5, 0,                 DSPL_DWIDTH, 0);
                DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT-1,    DSPL_DWIDTH, DSPL_DHEIGHT-1);
                DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT/2,    DSPL_DWIDTH, DSPL_DHEIGHT/2);

                DSPL_FONT(u8g2_font_b10_b_t_japanese1);
                sprn("%d", _lmin);
                DSPL_STR(DSPL_S_RIGHT(s)-5, DSPL_DHEIGHT-1, s);
                sprn("%d", _lmax);
                DSPL_STR(DSPL_S_RIGHT(s)-5, DSPL_S_HEIGHT, s);
                sprn("%d", _lmin+(_lmax-_lmin)/2);
                DSPL_STR(DSPL_S_RIGHT(s)-5, (DSPL_DHEIGHT+DSPL_S_HEIGHT-1)/2, s);

                // рисуем графики
#define LPADD 12
                double dx = static_cast<double>(DSPL_DWIDTH-LPADD) / _log.capacity();
                double xd = DSPL_DWIDTH;
                double dy = static_cast<double>(DSPL_DHEIGHT) / (_lmax-_lmin);
                for (const auto &l : _log) {
                    // точки графика
                    int x = static_cast<int>(round(xd));
                    int y = static_cast<int>(round(dy * (l.alt - _lmin)));
                    if ((y >= 0) && (y < DSPL_DHEIGHT)) {
                        y = DSPL_DHEIGHT-y-1;
                        DSPL_PIXEL(x, y);
                    
                        // высчитанное место изменения режима
                        if (l.ismode)
                            DSPL_DISC(x, y, 2);
                    }
                    
                    if (l.mchg)
                        // точка изменения режима (принятия реешения)
                        for (int y = 0; y < DSPL_DHEIGHT; y+=3)
                            DSPL_PIXEL(x, y);

                    xd -= dx;
                }
            }
            break;
#endif // USE_JMPTRACE
        }
    }
    //-----------------------------------

/* ------------------------------------------------------------------------------------------- *
 *  Текущее давление "у земли", нужно для отслеживания начала подъёма в режиме "сон"
 * ------------------------------------------------------------------------------------------- */
static AltSleep _slp;
    //-----------------------------------

namespace jmp {

    void init() {
        if (_bmp.init())
            CONSOLE("BMP280 ok: chipid=0x%02x, version=0x%02x", _bmp.chipid(), _bmp.version());
        else
            CONSOLE("BMP280 fail (chipid: 0x%02x)", _bmp.chipid());
        // тут мы оказываемся только после физической подачи питания,
        HAL_Delay(500); // надо дать успеть датчику проинициироваться
    }

    void setdraw() {
        Dspl::set(_draw);
    }

    void pagenxt() {
#ifdef USE_JMPTRACE
        page++; page %= 2;
#endif // USE_JMPTRACE
    }

    void resetgnd() {
        _ac.gndreset();
    }

    void resetmode() {
        _jmp.reset();
        _sq.reset();
        _jstr.reset();
    }

    uint8_t chipid() {
        return _bmp.chipid();
    }

    float press() {
        return _bmp.press();
    }

    AltJmp::mode_t mode() {
        return _jmp.mode();
    }

    bool isgnd() {
        return _jmp.mode() == AltJmp::GROUND;
    }


    void sleep() {
        _slp.clear();
        _slp.tick(_ac.pressgnd(), 100);
        _slp.tick(_ac.press(), 1);
        _bmp.setctrl(BMP280::MODE_SLEEP, BMP280::SAMPLING_NONE, BMP280::SAMPLING_NONE);
        CONSOLE("saved");
    }

    bool sleep2toff(uint32_t ms) {
        if (!_bmp.setctrl()) {
            CONSOLE("NO BMP280");
            return false;
        }
        
        _slp.tick(_bmp.press(), ms);
        _ac.gndset(_slp.pressgnd());
        if (!_slp.istoff()) {
            _bmp.setctrl(BMP280::MODE_SLEEP, BMP280::SAMPLING_NONE, BMP280::SAMPLING_NONE);
            return false;
        }

        CONSOLE("TAKEOFF _pressgnd: %0.2f", _slp.pressgnd());
        _jmp.reset(AltJmp::TAKEOFF);
        _sq.reset();
        _jstr.reset();
        _slp.clear();

        return true;
    }

    void tick(uint32_t ms) {
        _ac.tick(_bmp.press(), ms);
        auto m = _jmp.mode();
        _jmp.tick(_ac);
        const bool chgmode = m != _jmp.mode();
        _sq.tick(_ac);
        _jstr.tick(_ac);

        // gndreset
        if (
                (_jmp.mode() == AltJmp::GROUND) &&
                (abs(_ac.avg().speed()) < AC_SPEED_FLAT) &&
                (_jmp.tm() > ALT_AUTOGND_INTERVAL)
            ) {
            _ac.gndreset();
            _jmp.reset();
            CONSOLE("auto GND reseted");
        }

#ifdef USE_JMPTRACE
        // добавление в _log
        _log.push({ static_cast<int>(_ac.alt()), false, chgmode });
        if (chgmode && (_jmp.cnt() < _log.size()))
            _log[_jmp.cnt()].ismode = true;

        _lmin = _log[0].alt;
        _lmax = _log[0].alt;
        for (const auto &l : _log) {
            if (_lmin > l.alt)
                _lmin = l.alt;
            if (_lmax < l.alt)
                _lmax = l.alt;
        }

        auto y = _lmin - (_lmin % 300);
        if (y > _lmin+5) y -= 300; // +5, чтобы случайные -1-2m не уводили в -300
        _lmin = y;
        for (; y <= 15000; y += 300)
            if (y > _lmax) break;
        _lmax = y;
        // -------------------
#endif // USE_JMPTRACE
    }

} // namespace jmp 
