
#include "bargraph.h"
#include "ringsimple.h"
#include "../view/page.h"
#include "../view/menu.h"
#include "../view/btn.h"
#include "../sys/clock.h"
#include <cmath>

static ring<float, 5> _shrt;
static ring<float, 256> _long;
static float _min = 0, _max = 0, _dp = 0;
static uint32_t _ms = 0;

typedef enum {
    BDIR_UNKNOWN    = -2,
    BDIR_DOWN       = -1,
    BDIR_FLOAT      = 0,
    BDIR_UP         = 1
} bdir_t;
static bdir_t _dir = BDIR_UNKNOWN;

static void _draw(DSPL_ARG) {
    Dspl::drawBatt(u8g2);
    Dspl::drawClock(u8g2);
    Dspl::drawServ(u8g2);

    if ((_min == 0) || (_max == 0) || (_min > _max) || _long.empty())
        return;
    
    char s[strsz];

    if (_dir == BDIR_UP) {
        DSPL_FONT(u8g2_font_unifont_t_86);
        DSPL_GLYPH(0, 40, 0x2b08);
    }
    else
    if (_dir == BDIR_DOWN) {
        DSPL_FONT(u8g2_font_unifont_t_86);
        DSPL_GLYPH(0, 40, 0x2b0a);
    }
    else
    if (_dir == BDIR_FLOAT) {
        DSPL_FONT(u8g2_font_iconquadpix_m_all);
        DSPL_GLYPH(0, 40, 0x3d);
    }
    DSPL_FONT(u8g2_font_6x13B_tr);

    if (_dp < -8) { // < -0.06 mmHg
        sprn("%0.1f mmHg", _dp * BARGRAPH_MMHG);
        DSPL_STR(20, 40, s);
    }
    else
    if (_dp > 8) { // > +0.06 mmHg
        sprn("+%0.1f mmHg", _dp * BARGRAPH_MMHG);
        DSPL_STR(20, 40, s);
    }

    int y = DSPL_DHEIGHT - 1 - (DSPL_S_HEIGHT+2)*3;
    
    auto p = _shrt[0];
    sprn("%0.2f hPa", p / 100);
    DSPL_STR(0, y, s);

    y += DSPL_S_HEIGHT+2;
    sprn("%0.1f mmHg", p * BARGRAPH_MMHG);
    DSPL_STR(0, y, s);
    
    auto
        min = std::round(std::floor(_min * BARGRAPH_MMHG / 4) * 4),
        max = std::round(std::ceil (_max * BARGRAPH_MMHG / 4) * 4),
        k   = DSPL_DHEIGHT / (max-min) * BARGRAPH_MMHG;
    
    // вертикальная шкала
    DSPL_LINE(DSPL_DWIDTH-5, 0,                 DSPL_DWIDTH, 0);
    DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT-1,    DSPL_DWIDTH, DSPL_DHEIGHT-1);
    DSPL_LINE(DSPL_DWIDTH-5, DSPL_DHEIGHT/2,    DSPL_DWIDTH, DSPL_DHEIGHT/2);

    DSPL_FONT(u8g2_font_blipfest_07_tn);
    sprn("%0.0f", min);
    DSPL_STR(DSPL_S_RIGHT(s)-6, DSPL_DHEIGHT-1, s);
    sprn("%0.0f", max);
    DSPL_STR(DSPL_S_RIGHT(s)-6, DSPL_S_HEIGHT, s);
    sprn("%0.0f", min+(max-min)/2);
    DSPL_STR(DSPL_S_RIGHT(s)-6, (DSPL_DHEIGHT+DSPL_S_HEIGHT-1)/2, s);

    // горизонтальная шкала
    DSPL_LINE(0,             DSPL_DHEIGHT-3,    0,              DSPL_DHEIGHT);
    DSPL_LINE(DSPL_DWIDTH/2, DSPL_DHEIGHT-3,    DSPL_DWIDTH/2,  DSPL_DHEIGHT);
    auto tm = tmNow();
    int32_t tt = tm.h * 60 + tm.m - (BARGRAPH_INTERVAL * (DSPL_DWIDTH/2) / 60000);
    while (tt < 0) tt += 24*60;
    sprn("%ld:%02ld", tt/60, tt%60);
    DSPL_STR(DSPL_S_CENTER(s), DSPL_DHEIGHT-DSPL_S_HEIGHT+1, s);
    tt = tm.h * 60 + tm.m - (BARGRAPH_INTERVAL * DSPL_DWIDTH / 60000);
    while (tt < 0) tt += 24*60;
    sprn("%ld:%02ld", tt/60, tt%60);
    DSPL_STR(0, DSPL_DHEIGHT-DSPL_S_HEIGHT+1, s);

    // график
    min /= BARGRAPH_MMHG;
    int x0 = DSPL_DWIDTH, y0 = std::round((_long[0] - min) * k);
    for (auto p: _long) {
        int x = x0 - 1;
        if (x < 0)
            break;
        int y = std::round((p - min) * k);
        if (
                (y0 >= 0) && (y0 < DSPL_DHEIGHT) &&
                (y  >= 0) && (y  < DSPL_DHEIGHT)
            )
            DSPL_LINE(x, DSPL_DHEIGHT-y-1, x0, DSPL_DHEIGHT-y0-1);
        x0 = x;
        y0 = y;
    }
};

namespace bar {

    void tick(float press, uint32_t ms) {
        _shrt.push(press);
        _ms += ms;
        while (_ms >= BARGRAPH_INTERVAL) {
            // среднее в _shrt
            float ps = 0;
            for (auto p: _shrt)
                ps += p;
            ps /= _shrt.size();

            // добавляем в _long
            _long.push(ps);
            _ms -= BARGRAPH_INTERVAL;

            // _min/_max в _long
            _min = _long[0];
            _max = _min;
            for (auto p: _long) {
                if (_min > p)
                    _min = p;
                if (_max < p)
                    _max = p;
            }

            // _dir
            if (_long.size() > BARGRAPH_BDIR_CNT) {
                // абсолютное изменение давление за этот период
                // порядок обратный (0 - самый свежий)
                _dp = _long[0] - _long[BARGRAPH_BDIR_CNT];

                // в pp заносим среднее за каждые 6 замеров
                // порядок обратный (0 - самый свежий, 5 - самый старый)
                const int ppsz = BARGRAPH_BDIR_CNT/6;
                float pp[ppsz], pn = 0;
                int n = 0, c = 0;
                for (auto p: _long) {
                    c++;
                    if (c > 6) {
                        n++;
                        if (n >= ppsz)
                            break;
                        c = 1;
                        pn = 0;
                    }
                    pn += p;
                    pp[n] = pn / c;
                }

                // теперь идём от самого свежего pp[0] к pp[BARGRAPH_BDIR_CNT]
                // и на каждой итерации смотрим изменение - "вверх", "вниз" или "не изменилось"
                int cdn = 0, cup = 0, cft = 0;
                pn = pp[0];
                for (auto p: pp) {
                    auto pd = pn - p;
                    pn = p;
                    // 26 Pa = 0.2 mmHg - это величина за 30 минут,
                    // 0.2*6 = 1.2 mmHg за 3 часа
                    if (pd < -15)
                        cdn ++;
                    else
                    if (pd > 15)
                        cup ++;
                    else
                        cft ++;
                }
                // если каких-то движений более половины, значит эта тенденция - основная
                _dir =
                    cdn >= BARGRAPH_BDIR_CNT/2 ?
                        BDIR_DOWN :
                    cup >= BARGRAPH_BDIR_CNT/2 ?
                        BDIR_UP :
                    cft >= BARGRAPH_BDIR_CNT/2 ?
                        BDIR_FLOAT :
                        BDIR_UNKNOWN; // если нет стабильной тенденции
            }
        }
    }

    void setdraw() {
        Menu::clear();

        Dspl::set(_draw);

        Btn::set(
            Btn::UP,
            Dspl::lightTgl
        );
        Btn::set(
            Btn::SEL,
            Dspl::page
        );
        Btn::set(
            Btn::DN,
            Dspl::page
        );
    }

} // namespace bar 
