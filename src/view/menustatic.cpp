/*
    Display: Menu Static
*/

#include "menustatic.h"
#include "menumodal.h"
#include "text.h"
#include "../sys/log.h"

#include "../sys/maincfg.h"
#include "../view/btn.h"
#include "../view/dspl.h"

#include "../jump/proc.h"
#include "../sys/clock.h"
#include "../sys/power.h"
#include "../sys/stm32drv.h"

#include <stdlib.h>
#include <string.h>

void MenuStatic::title(char *s) {
    strncpy(s, _title, MENUSZ_TITLE);
}

void MenuStatic::str(line_t &s, int16_t i) {
    auto &m = _m[i];

    strncpy(s.name, m.name, sizeof(s.name));
    
    if (m.showval == NULL)
        s.val[0] = '\0';
    else
        m.showval(s.val);
}

void MenuStatic::onsel(int16_t i) {
    auto &m = _m[i];

    if (m.enter != NULL)
        m.enter();
}

MenuStatic::MenuStatic(const el_t *m, int16_t sz) :
    _m(m),
    _sz(sz)
{
    line_t l;
    _title =
        prevstr(l, 1) ?
            strdup(l.name) :
            NULL;
    CONSOLE("sz: %d, title: %s", _sz, _title != NULL ? _title : "-null-");
}

MenuStatic::~MenuStatic() {
    if (_title != NULL)
        free(_title);
}

static void vnum(char *s, int v) {
    snprintf(s, MENUSZ_VAL, "%d", v);
}
static void vyesno(char *s, bool v) {
    strncpy(s, v ? TXT_MENU_YES : TXT_MENU_NO, MENUSZ_VAL);
}
static void vonoff(char *s, bool v) {
    strncpy(s, v ? TXT_MENU_YES : TXT_MENU_NO, MENUSZ_VAL);
}
static void vok(char *s, bool v) {    // ok/fail
    strncpy(s, v ? TXT_MENU_OK : TXT_MENU_FAIL, MENUSZ_VAL);
}

static const MenuStatic::el_t _alt[] = {
    {
        .name   = TXT_ALT_AUTOGND,
        .enter  = [] { new MenuValBool(cfg->autognd, [] (bool v) { (*cfg)->autognd = v; }); },
        .showval= [] (char *v) { vyesno(v, cfg->autognd); }
    },
    {
        .name   = TXT_ALT_METER,
        .enter  = [] { new MenuValBool(cfg->altmeter, [] (bool v) { (*cfg)->altmeter = v; }); },
        .showval= [] (char *v) { vyesno(v, cfg->altmeter); }
    },
    {
        .name   = TXT_ALT_CORRECT,
        .enter  = [] { new MenuValInt(cfg->altcorrect, [] (int v) { (*cfg)->altcorrect = v; }, -5000, 10000); },
        .showval= [] (char *v) { vnum(v, cfg->altcorrect); }
    },
    {
        .name   = TXT_ALT_RESETGND,
        .enter  = [] { new MenuConfirm(jmp::resetgnd); }
    },
    {
        .name   = TXT_ALT_RESETMODE,
        .enter  = [] { new MenuConfirm(jmp::resetmode); }
    }
};

static const MenuStatic::el_t _hwtest[] {
    {
        .name = TXT_TEST_CLOCK,
        .enter = NULL,
        .showval = [] (char *txt) {
            auto t = tmNow();
            snprintf(txt, MENUSZ_VAL, "%d:%02d:%02d", t.h, t.m, t.s);
        },
    },
/*
    {
        .name = TXT_TEST_BATTERY,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];
            uint16_t bval = pwrBattRaw();
            valOk(ok, (bval > 2400) && (bval < 3450));
            sprintf_P(txt, PSTR("(%0.2fv) %s"), pwrBattValue(), ok);
        },
    },
    {
        .name = TXT_TEST_BATTCHARGE,
        .enter = NULL,
        .showval = [] (char *txt) { valYes(txt, pwrBattCharge()); },
    },
*/
    {
        .name = TXT_TEST_PRESSID,
        .enter = NULL,
        .showval = [] (char *txt) {
            snprintf(txt, MENUSZ_VAL, "0x%02x", jmp::chipid());
        },
    },
    {
        .name = TXT_TEST_PRESSURE,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];
            float press = jmp::press();
            
            vok(ok, (press > 60000) && (press < 150000));
            snprintf(txt, MENUSZ_VAL, TXT_TEST_PRESSVAL, press, ok);
        },
    },
    {
        .name = TXT_TEST_LIGHT,
        .enter = [] () {
            Dspl::lightTgl();
            HAL_Delay(2000);
            Dspl::lightTgl();
        },
        .showval = [] (char *txt) { vonoff(txt, Dspl::light()); },
    },
/*
    {
        .name = TXT_TEST_SDCARD,
        .enter = NULL,
        .showval = [] (char *txt) {
            CONSOLE("sdcard test beg");
            fileExtInit();
            
            char t[15];
            auto type = SD.cardType();
            switch (type) {
                case CARD_NONE:     strcpy_P(t, PSTR("none"));  break;
                case CARD_MMC:      strcpy_P(t, PSTR("MMC"));   break;
                case CARD_SD:       strcpy_P(t, PSTR("SD"));    break;
                case CARD_SDHC:     strcpy_P(t, PSTR("SDHC"));  break;
                case CARD_UNKNOWN:  strcpy_P(t, PSTR("UNK"));   break;
                default:            sprintf_P(t, PSTR("[%d]"), type);
            }
            
            if (type != CARD_NONE) {
                double sz = static_cast<double>(SD.cardSize());
                char s = 'b';
                if (sz > 1024) {
                    sz = sz / 1024;
                    s = 'k';
                    if (sz > 1024) {
                        sz = sz / 1024;
                        s = 'M';
                    }
                    if (sz > 1024) {
                        sz = sz / 1024;
                        s = 'G';
                    }
                }

                sprintf_P(txt, PSTR("%s / %0.1f %c"), t, sz, s);
            }
            else {
                strcpy_P(txt, "none");
            }
            fileExtStop();
            CONSOLE("sdcard test end");
        },
    },
*/
};

class MenuTimeEdit : public MenuModal {
    enum echg {
        CH_DAY,
        CH_MON,
        CH_YEAR,
        CH_HOUR,
        CH_MIN,
        CH_OK
    } _chg = CH_DAY;
    tm_t _tm = tmNow();

    public:
        MenuTimeEdit() :
            MenuModal(2)
        {}
        void str(char *s, uint8_t n) {
            switch (n) {
                case 1:
                    sprintf(s, "%02d  .  %02d  .  % 4d", _tm.day, _tm.mon, _tm.year);
                    break;
                case 2:
                    sprintf(s, "%02d  :  %02d        OK  ", _tm.h, _tm.m);
                    break;
            }
        }
        void smplup() {
            switch (_chg) {
                case CH_DAY:
                    if (_tm.day < 31)
                        _tm.day ++;
                    else
                        _tm.day = 1;
                    break;
                case CH_MON:
                    if (_tm.mon < 12)
                        _tm.mon ++;
                    else
                        _tm.mon = 1;
                    break;
                case CH_YEAR:
                    _tm.year ++;
                    break;
                case CH_HOUR:
                    if (_tm.h < 24)
                        _tm.h ++;
                    else
                        _tm.h = 1;
                    break;
                case CH_MIN:
                    if (_tm.m < 59)
                        _tm.m ++;
                    else
                        _tm.m = 1;
                    break;
                case CH_OK:
                    _chg = CH_MIN;
                    break;
            }
        }
        void smpldn() {
            switch (_chg) {
                case CH_DAY:
                    if (_tm.day > 1)
                        _tm.day --;
                    else
                        _tm.day = 31;
                    break;
                case CH_MON:
                    if (_tm.mon > 1)
                        _tm.mon --;
                    else
                        _tm.mon = 12;
                    break;
                case CH_YEAR:
                    _tm.year --;
                    break;
                case CH_HOUR:
                    if (_tm.h > 1)
                        _tm.h --;
                    else
                        _tm.h = 23;
                    break;
                case CH_MIN:
                    if (_tm.m > 1)
                        _tm.m --;
                    else
                        _tm.m = 59;
                    break;
                case CH_OK:
                    _chg = CH_DAY;
                    break;
            }
        }
        void smplsel() {
            if (_chg < CH_OK) {
                _chg = static_cast<echg>(_chg + 1);
            }
            else {
                _tm.s = 0;
                if (tmAdjust(_tm, 0))
                    close();
            }
        }

        void draw(DSPL_ARG) {
            MenuModal::draw(u8g2);

            int x = 
                (_chg == CH_DAY)    || (_chg == CH_HOUR)    ? 50 :
                (_chg == CH_MON)    || (_chg == CH_MIN)     ? 78 :
                (_chg == CH_YEAR)   || (_chg == CH_OK)      ? 106 : 0;
            int y =
                (_chg == CH_DAY)    || (_chg == CH_MON)     || (_chg == CH_YEAR)    ? 42 :
                (_chg == CH_HOUR)   || (_chg == CH_MIN)     || (_chg == CH_OK)      ? 55 : 0;
            int w =
                (_chg == CH_YEAR)   || (_chg == CH_OK)      ? 35 : 20;
            DSPL_RFRAME(x, y, w, 15, 7);
        }
};


static const MenuStatic::el_t _system[] = {
    {
        .name   = TXT_SYSTEM_DATETIME,
        .enter  = [] { new MenuTimeEdit(); }
    },
    {
        .name   = TXT_SYSTEM_LOPWRONJMP,
        .enter  = [] { new MenuValBool(cfg->lopwronjmp, [] (bool v) { (*cfg)->lopwronjmp = v; }); },
        .showval= [] (char *v) { vyesno(v, cfg->lopwronjmp); }
    },
    {
        .name   = TXT_SYSTEM_RESET,
        .enter  = [] { new MenuConfirm(HAL_NVIC_SystemReset); }
    },
    {
        .name   = TXT_SYSTEM_POWEROFF,
        .enter  = [] { new MenuConfirm(pwr::off); }
    },
    {
        .name   = TXT_SYSTEM_CFGDEFAULT,
        .enter  = [] {
            new MenuConfirm([] () {
                cfg.resetdefault();
                Btn::flip180(cfg->flip180);
                Dspl::flip180(cfg->flip180);
                Dspl::contrast(cfg->contrast);
            });
        }
    },
    {
        .name   = TXT_SYSTEM_HWTEST,
        .enter  = [] { MENU_STATIC(_hwtest); }
    },
};

static const MenuStatic::el_t _main[] = {
    {
        .name   = TXT_MAIN_JMPCNT,
        .enter  = [] { new MenuValInt(cfg->jmpcnt, [] (int v) { (*cfg)->jmpcnt = v; }, 0, 99999); },
        .showval= [] (char *v) { vnum(v, cfg->jmpcnt); }
    },
    {
        .name   = TXT_MAIN_FLIP180,
        .enter  = [] { new MenuValBool(cfg->flip180, [] (bool v) { Btn::flip180(v); Dspl::flip180(v); }); },
        .showval= [] (char *v) { vyesno(v, cfg->flip180); }
    },
    {
        .name   = TXT_MAIN_CONTRAST,
        .enter  = [] { new MenuValInt(cfg->contrast, [] (int v) { Dspl::contrast(v); }, 0, 20); },
        .showval= [] (char *v) { vnum(v, cfg->contrast); }
    },
    {
        .name   = TXT_MAIN_ALT,
        .enter  = [] { MENU_STATIC(_alt); },
    },
    {
        .name   = TXT_MAIN_SYSTEM,
        .enter  = [] { MENU_STATIC(_system); },
    }
};

void MenuStatic::main() {
    MENU_STATIC(_main);
}
