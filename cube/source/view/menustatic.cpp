/*
    Display: Menu Static
*/

#include "menustatic.h"

#ifdef USE_MENU

#include "menumodal.h"
#include "text.h"
#include "btn.h"
#include "dspl.h"
#include "menulogbook.h"

#include "../sys/log.h"
#include "../sys/maincfg.h"

#include "../jump/proc.h"
#include "../sys/clock.h"
#include "../sys/power.h"
#include "../sys/batt.h"
#include "../sys/stm32drv.h"

#include "../sdcard/fshnd.h"

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

MenuStatic::MenuStatic(const el_t *m, int16_t sz, bool tout) :
    Menu(EXIT_TOP),
    _m(m),
    _sz(sz),
    _tout(tout)
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
    sprintf(s, "%d", v);
}
static void vyesno(char *s, bool v) {
    strcpy(s, v ? TXT_MENU_YES : TXT_MENU_NO);
}
static void vonoff(char *s, bool v) {
    strcpy(s, v ? TXT_MENU_YES : TXT_MENU_NO);
}
static void vok(char *s, bool v) {    // ok/fail
    strcpy(s, v ? TXT_MENU_OK : TXT_MENU_FAIL);
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
        .enter  = [] { new MenuConfirm([] () { jmp::resetgnd(); jmp::resetmode(); }); }
    },
    /*
    {
        .name   = TXT_ALT_RESETMODE,
        .enter  = [] { new MenuConfirm(jmp::resetmode); }
    }
    */
};

static const MenuStatic::el_t _hwtest[] {
#ifdef UID_BASE
    {
        .name = TXT_TEST_SERIAL,
        .enter = NULL,
        .showval = [] (char *txt) {
            auto s1 = *reinterpret_cast<uint32_t *>(UID_BASE);
            auto s2 = *reinterpret_cast<uint32_t *>(UID_BASE + 4);
            auto s3 = *reinterpret_cast<uint32_t *>(UID_BASE + 8);
            snprintf(txt, MENUSZ_VAL, "%08lx:%08lx:%08lx", s1, s2, s3);
        },
    },
#endif // UID_BASE
#ifdef FLASH_SIZE
    {
        .name = TXT_TEST_FLASHSZ,
        .enter = NULL,
        .showval = [] (char *txt) {
            double sz = FLASH_SIZE;
            snprintf(txt, MENUSZ_VAL, "%.1f kB", sz / 1024);
        },
    },
#endif // FLASH_SIZE
    {
        .name = TXT_TEST_CLOCK,
        .enter = NULL,
        .showval = [] (char *txt) {
            auto t = tmNow();
            snprintf(txt, MENUSZ_VAL, "%d:%02d:%02d", t.h, t.m, t.s);
        },
    },
    {
        .name = TXT_TEST_BATTERY,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[16];
            auto bval = batt::raw();
            vok(ok, (bval > 3000) && (bval < 3900));
            float v = vmap(static_cast<float>(bval), 3105, 3945, 3.4, 4.3);
            snprintf(txt, MENUSZ_VAL, "(%u = %0.1fv) %s", bval, v, ok);
        },
    },
    {
        .name = TXT_TEST_BATTCHARGE,
        .enter = NULL,
        .showval = [] (char *txt) { vyesno(txt, batt::charge()); },
    },
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
            char ok[16];
            float p;
            if (!jmp::press(p)) {
                strcpy(txt, "-");
                return;
            }
            
            vok(ok, (p > 60000) && (p < 150000));
            snprintf(txt, MENUSZ_VAL, TXT_TEST_PRESSVAL, p, ok);
        },
    },
    {
        .name = TXT_TEST_PTEMP,
        .enter = NULL,
        .showval = [] (char *txt) {
            float t;
            if (!jmp::temp(t)) {
                strcpy(txt, "-");
                return;
            }
            
            snprintf(txt, MENUSZ_VAL, TXT_TEST_PTEMPVAL, t);
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
    {
        .name = TXT_TEST_SDCARD,
        .enter = NULL,
        .showval = [] (char *txt) {
            static uint32_t _t = 0, t1;
            static uint8_t ct = 0;
            static double csz = 0;
            if ((t1 = HAL_GetTick()) > _t) {
                _t = t1+2000;

                FSMount fs("");
                if (fs) {
                    ct = fs.ctype();
                    csz = static_cast<double>((fs->n_fatent - 2) * fs->csize) / 2000;
                }
                else
                    ct = 0;
            }
            
            const char *t =
                ct == FSMount::CT_UNKNOWN   ? "unknown" :
                ct == FSMount::CT_NONE      ? "-" :
                ct == FSMount::CT_MMC3      ? "MMC3" :
                ct == FSMount::CT_MMC4      ? "MMC4" :
                ct == FSMount::CT_MMC       ? "MMC" :
                ct == FSMount::CT_SDC1      ? "SDC1" :
                ct == FSMount::CT_SDC2      ? "SDC2" :
                ct == FSMount::CT_SDC       ? "SDC" :
                ct == FSMount::CT_BLOCK     ? "BLOCK" :
                ct == FSMount::CT_SDC2BL    ? "SDC2BL" : "";
            
            if (ct == FSMount::CT_NONE) {
                strcpy(txt, "-");
            }
            else {
                auto sz = csz;
                char s = 'M';
                if (sz > 1000) {
                    sz = sz / 1000;
                    s = 'G';
                }

                sprintf(txt, "%s / %0.1f %c", t, sz, s);
            }
        },
    },
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

#include "../jump/logbook.h"
#include "../sdcard/saver.h"
#include "../sys/iflash.h"
extern "C"
void Error_Handler(void);

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
        .enter  = [] { MENU_STATIC(_hwtest, false); }
    },

#ifdef USE_DEVMENU
#if HWVER < 2
    {
        .name   = "led red",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)); }
    },
    {
        .name   = "led blue",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4)); }
    },
#else // if HWVER < 2
{
    .name   = "led red",
    .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0); },
    .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)); }
},
{
    .name   = "led blue",
    .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1); },
    .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)); }
},
#endif // if HWVER < 2
    {
        .name   = "logbook test",
        .enter  = [] {
            new MenuConfirm([] () {
                LogBook::beg_cnp(15500, 1200 + (((tmRand()/1000)%16) * 100));
                LogBook::end(0);
            });
        },
        //.showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6)); }
    },
#if HWVER < 2
    {
        .name   = "chg hi",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15)); }
    },
    {
        .name   = "hwen",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6)); }
    },
#else // if HWVER < 2
    {
        .name   = "chg hi",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13)); }
    },
    {
        .name   = "sdcard en",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4)); }
    },
    {
        .name   = "sdcard cs",
        .enter  = [] { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3); },
        .showval= [] (char *v) { vyesno(v, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)); }
    },
#endif // if HWVER < 2
    {
        .name   = "test fatal",
        .enter  = [] { new MenuConfirm(Error_Handler); },
    },
    {
        .name   = "test devide zero",
        .enter  = [] { new MenuConfirm([] () { volatile int n = 0; volatile int t = 5 / n; CONSOLE("n: %d", t); }); },
    },
    {
        .name   = "test memory fail",
        .enter  = [] { new MenuConfirm([] () { char s[] = ""; CONSOLE("s: %s", s+20); free(s); }); },
    },
/*
    {
        .name   = "flash fill",
        .enter  = [] { new MenuConfirm([] () { 
            uint32_t n = 0;
            while (auto r = iflash::last(iflash::TYPE_ANY)) {
                if (r.ispgend())
                    break;
                auto a = r.addr() - r.page().addr(0);
                if (a >= _FLASH_PAGE_SIZE - 50)
                    break;
                (*cfg)->jmpcnt ++;
                cfg.save();
                n++;
                if (n>=500)
                    break;
            }
        }); },
    },
*/
    {
        .name   = "sdcard",
        .enter  = [] { sdcard_save(); },
    }
#endif // USE_DEVMENU
};

static const MenuStatic::el_t _main[] = {
    {
        .name   = TXT_MAIN_JMPCNT,
        .enter  = [] { new MenuValInt(cfg->jmpcnt, [] (int v) { (*cfg)->jmpcnt = v; }, 0, 99999, 10); },
        .showval= [] (char *v) { vnum(v, cfg->jmpcnt); }
    },
    {
        .name   = TXT_MAIN_LOGBOOK,
        .enter  = [] { new MenuLogBook(); },
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

#endif // USE_MENU
