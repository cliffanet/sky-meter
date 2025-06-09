/*
    Display: Menu LogBook
*/

#include "menulogbook.h"
#include "../sys/maincfg.h"
#include "../view/text.h"
#include "../sdcard/fshnd.h"
#include "../ff/diskio.h"

#include <string.h>

#if defined(USE_MENU) && defined(USE_LOGBOOK)

void MenuLogBook::title(char *s) {
    strncpy(s, TXT_MAIN_LOGBOOK, MENUSZ_TITLE);
}

void MenuLogBook::str(line_t &s, int16_t i) {
    LogBook::item_t l = { 0 };
    if ((i >= 0) && (i < MENU_LOGBOOK_SIZE))
#if HWVER < 2
        l = _d[i];
#else
        l = _d[i].l;
#endif
    
    auto &tm = l.tm;
    snprintf(s.name, sizeof(s.name), "%2d.%02d.%02d %2d:%02d",
                    tm.day, tm.mon, tm.year % 100, tm.h, tm.m);
    snprintf(s.val, sizeof(s.val), "%ld", l.num);
}

void MenuLogBook::onsel(int16_t i) {
    new MenuLogBookInfo(*this);
}

MenuLogBook::MenuLogBook() :
    _sz(0)
{
#if HWVER < 2

    if (cfg->lbaddr > 0) {
        auto addr = cfg->lbaddr;
        while (
                (_sz < MENU_LOGBOOK_SIZE) && 
                ((addr = LogBook::findprv(addr, _d[_sz])) > 0)
            )
            _sz++;
    }

#else // if HWVER < 2

    for (auto r = iflash::last(iflash::TYPE_LOGBOOK); r && (_sz < MENU_LOGBOOK_SIZE); r = r.prevme()) {
        _d[_sz].r = r;
        r.read(_d[_sz].l);
        _sz++;
    }

#endif // if HWVER < 2
}

#if HWVER >= 2

void MenuLogBook::smplup() {
    updtout();

    if (ipos(_isel) > 0)
        Menu::smplup();
    else
    if (_sz > 0) {
        auto &fst = _d[0];
        auto r = fst.r.nextme();
        if (r) {
            for (auto i=MENU_LOGBOOK_SIZE-1; i > 0; i--)
                _d[i] = _d[i-1];
            fst.r = r;
            r.read(fst.l);
        }
        else
        if (_isel > 0)
            Menu::smplup();
    }
}

void MenuLogBook::smpldn() {
    updtout();
    
    if (ipos(_isel) < static_cast<int>(_sz)-1)
        Menu::smpldn();
    else
    if (_sz >= MENU_LOGBOOK_SIZE) {
        auto &lst = _d[_sz-1];
        auto r = lst.r.prevme();
        if (r) {
            for (auto i=1; i < MENU_LOGBOOK_SIZE; i++)
                _d[i-1] = _d[i];
            lst.r = r;
            r.read(lst.l);
        }
    }
}

#endif // if HWVER >= 2

void MenuLogBook::MenuLogBookInfo::updtrc() {
    _trc_cnt = 0;
    _trc_nat = false;

    fs::mounter m;
    if (!m)
        return;

    char fprf[15];
    auto l = snprintf(fprf, sizeof(fprf), "jump_%lu_", _l.num);
    
    DIR dh;
    if (f_opendir(&dh, "/") != FR_OK)
        return;

    FILINFO finf;
    for (;;) {
        if (f_readdir(&dh, &finf) != FR_OK)
            break;
        if (finf.fname[0] == '\0')
            break;
        
        if (finf.fattrib & AM_DIR)
            continue;
        
        if (strncmp(finf.fname, fprf, l) != 0)
            continue;
        
        _trc_cnt++;
        if (!_trc_nat) {
            auto l1 = strlen(finf.fname);
            if ((l1 > 10) && (strncmp(finf.fname + l1 - 8, "_man.csv", 8) != 0))
                _trc_nat = true;
        }
    }

    f_closedir(&dh);
}

void MenuLogBook::MenuLogBookInfo::updinf() {
    int i = _s.ipos(_s._isel);
    _l =
        (i >= 0) && (i < MENU_LOGBOOK_SIZE) ?
#if HWVER < 2
            _s._d[i] :
#else
            _s._d[i].l :
#endif
            LogBook::item_t();
    updtrc();
}

MenuLogBook::MenuLogBookInfo::MenuLogBookInfo(MenuLogBook &s) :
    _s(s)
{
    updinf();
}

void MenuLogBook::MenuLogBookInfo::draw(DSPL_ARG) {
    DSPL_COLOR(1);
    DSPL_BOX(0, 0, DSPL_DWIDTH, 12);
    
    DSPL_FONT(menuFont);

    // Заголовок
    char s[64];
    snprintf(s, sizeof(s), TXT_LOGBOOK_JUMPNUM, _l.num);
    DSPL_COLOR(0);
    DSPL_STRU(DSPL_S_CENTER(s), 10, s);

    DSPL_COLOR(1);
    int8_t y = 10-1+14;

    const auto &tm = _l.tm;
    snprintf(s, sizeof(s), "%2d.%02d.%04d", tm.day, tm.mon, tm.year);
    DSPL_STR(0, y, s);
    snprintf(s, sizeof(s), "%2d:%02d", tm.h, tm.m);
    DSPL_STR(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_ALTEXIT);
    snprintf(s, sizeof(s), "%d", _l.begalt);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_ALTCNP);
    snprintf(s, sizeof(s), "%d", _l.cnpalt);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TIMETOFF);
    snprintf(s, sizeof(s), TXT_LOGBOOK_MINSEC, _l.toffsec / 60, _l.toffsec % 60);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TIMEFF);
    snprintf(s, sizeof(s), TXT_LOGBOOK_TIMESEC, _l.ffsec);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TIMECNP);
    snprintf(s, sizeof(s), TXT_LOGBOOK_MINSEC, _l.cnpsec / 60, _l.cnpsec % 60);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TRACE);
    if (_trc_cnt)
        snprintf(s, sizeof(s), _trc_nat ? TXT_MENU_YES " (%u)" : "man (%u)", _trc_cnt);
    else
        strcpy(s, "-");
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);
}

void MenuLogBook::MenuLogBookInfo::smplup() {
    _s.smplup();
    int i = _s.ipos(_s._isel);
    if ((i < 0) || (i >= MENU_LOGBOOK_SIZE))
        _s.smpldn(); // попали на exit, вернём обратно
    updinf();
}

void MenuLogBook::MenuLogBookInfo::smpldn() {
    _s.smpldn();
    int i = _s.ipos(_s._isel);
    if ((i < 0) || (i >= MENU_LOGBOOK_SIZE))
        _s.smplup(); // попали на exit, вернём обратно
    updinf();
}

void MenuLogBook::MenuLogBookInfo::smplsel() {
    close();
}

#endif // defined(USE_MENU) && defined(USE_LOGBOOK)
