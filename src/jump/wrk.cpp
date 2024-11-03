
#include "wrk.h"
#include "bmp280.h"
#include "altcalc.h"
#include "../sys/trwire.h"
#include "../sys/stm32drv.h"
#include "../sys/worker.h"
#include "../sys/maincfg.h"
#include "../sys/log.h"
#include "../view/page.h"

#include <cmath>
#include "string.h"

class _jmpWrk;
static _jmpWrk *_w = NULL;
static uint8_t page = 0;

class _jmpWrk : public Wrk {
    TransSPI _spi;
    BMP280 _bmp;

    uint32_t _tck;
    AltCalc _ac;
    AltJmp _jmp = AltJmp(true);
    AltSqBig _sq;
    AltStrict _jstr;

    typedef struct {
        int alt;
        bool ismode;
        bool mchg;
    } log_t;
    ring<log_t, 3*60*10> _log;
    int _lmin = 0, _lmax = 0;

public:
    _jmpWrk() :
        _spi(GPIOB, GPIO_PIN_0),
        _bmp(_spi),
        _jmp(true)
    {
        if (_w != NULL)
            delete _w;
        _w = this;
        optset(O_AUTODELETE);

        if (_bmp.init())
            CONSOLE("BMP280 ok: chipid=0x%02x, version=0x%02x", _bmp.chipid(), _bmp.version());
        else
            CONSOLE("BMP280 fail (chipid: 0x%02x)", _bmp.chipid());
        
        _tck = HAL_GetTick();
    }
    ~_jmpWrk() {
        if (_w == this)
            _w = NULL;
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

    Wrk::state_t run() {
        auto tck = HAL_GetTick();
        auto interval = tck - _tck;
        _tck = tck;

        _ac.tick(_bmp.press(), interval);
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
        
        return DLY;
    }

    //-----------------------------------

    int16_t alt() const {
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
    static void draw(DSPL_ARG) {
        Dspl::drawBatt(u8g2);
        Dspl::drawClock(u8g2);

        if (_w == NULL)
            return;
        
        char s[strsz], m[strsz], t[strsz];

        // info
        DSPL_FONT(u8g2_font_6x13B_tr);

        uint8_t y = 40;
        sprn("%.1f m/s", _w->_ac.avg().speed());
        DSPL_STR(static_cast<int>(DSPL_S_RIGHT(s))+60-DSPL_DWIDTH, y, s);

        y += DSPL_S_HEIGHT+2;
        sprn("%s (%s)", modestr(m, _w->_jmp.mode()), strtm(t, _w->_jmp.tm()));
        DSPL_STR(0, y, s);

        y += DSPL_S_HEIGHT+2;
        if (_w->_jmp.newtm() > 0) {
            sprn("new: %s (%d)", strtm(t, _w->_jmp.newtm()), _w->_jmp.newcnt());
            DSPL_STR(0, y, s);
        }

        y += DSPL_S_HEIGHT+2;
        if (_w->_jmp.ff().active()) {
            sprn("ff: %d", _w->_jmp.ff().num());
            DSPL_STR(0, y, s);
        }

        y += DSPL_S_HEIGHT+2;
        sprn("q:%0.1f(%s)", _w->_sq.val(), strtm(t, _w->_sq.tm()));
        DSPL_STR(0, y, s);

        sprn("s[%d]:%s(%s)", _w->_jstr.prof().num(), modestr(m, _w->_jstr.mode()), strtm(t, _w->_jstr.tm()));
        DSPL_STR(DSPL_S_RIGHT(s)-15, DSPL_DHEIGHT-1, s);
        
        switch (page) {
            case 0: {
                // alt
                DSPL_FONT(u8g2_font_logisoso62_tn);
                sprn("%d", _w->alt());
                DSPL_STR(DSPL_S_RIGHT(s), 80, s);

            }
            break;
            case 1: {
                // alt
                DSPL_FONT(u8g2_font_fub20_tr);

                int y = DSPL_S_HEIGHT;
                sprn("%d", _w->alt());
                DSPL_STR(90-DSPL_S_WIDTH(s), y, s);

                // вертикальная шкала
                DSPL_LINE(DSPL_DWIDTH-5, 0,                 DSPL_DWIDTH, 0);
                DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT-1,    DSPL_DWIDTH, DSPL_DHEIGHT-1);
                DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT/2,    DSPL_DWIDTH, DSPL_DHEIGHT/2);

                DSPL_FONT(u8g2_font_b10_b_t_japanese1);
                sprn("%d", _w->_lmin);
                DSPL_STR(DSPL_S_RIGHT(s)-5, DSPL_DHEIGHT-1, s);
                sprn("%d", _w->_lmax);
                DSPL_STR(DSPL_S_RIGHT(s)-5, DSPL_S_HEIGHT, s);
                sprn("%d", _w->_lmin+(_w->_lmax-_w->_lmin)/2);
                DSPL_STR(DSPL_S_RIGHT(s)-5, (DSPL_DHEIGHT+DSPL_S_HEIGHT-1)/2, s);

                // рисуем графики
#define LPADD 12
                double dx = static_cast<double>(DSPL_DWIDTH-LPADD) / _w->_log.capacity();
                double xd = DSPL_DWIDTH;
                double dy = static_cast<double>(DSPL_DHEIGHT) / (_w->_lmax-_w->_lmin);
                for (const auto &l : _w->_log) {
                    // точки графика
                    int x = static_cast<int>(round(xd));
                    int y = static_cast<int>(round(dy * (l.alt - _w->_lmin)));
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
        }
    }

};

namespace jmp {

void init() {
    new _jmpWrk();
}

void stop() {
    if (_w != NULL)
        delete _w;
}

void setdraw() {
    Dspl::set(_jmpWrk::draw);
}

void pagenxt() {
    page++; page %= 2;
}

void resetgnd() {
    if (_w != NULL)
        _w->resetgnd();
}

void resetmode() {
    if (_w != NULL)
        _w->resetmode();
}

uint8_t chipid() {
    return _w != NULL ? _w->chipid() : 0;
}

float press() {
    return _w != NULL ? _w->press() : 0;
}

} // namespace jmp 
