/*
    Display: Menu LogBook
*/

#include "menulogbook.h"
#include "../sys/maincfg.h"

#include <string.h>

#if defined(USE_MENU) && defined(USE_LOGBOOK)

void MenuLogBook::title(char *s) {
    strncpy(s, TXT_MAIN_LOGBOOK, MENUSZ_TITLE);
}

void MenuLogBook::str(line_t &s, int16_t i) {
    LogBook::item_t l = { 0 };
    if ((i >= 0) && (i < MENU_LOGBOOK_SIZE))
        l = _d[i];
    
    auto &tm = l.tm;
    snprintf(s.name, sizeof(s.name), "%2d.%02d.%02d %2d:%02d",
                    tm.day, tm.mon, tm.year % 100, tm.h, tm.m);
    snprintf(s.val, sizeof(s.val), "%d", l.num);
}

void MenuLogBook::onsel(int16_t i) {
    new MenuLogBookInfo(*this);
}

MenuLogBook::MenuLogBook() :
    _prv(0),
    _nxt(0),
    _sz(0)
{
    if (cfg->lbaddr > 0) {
        auto addr = cfg->lbaddr;
        while (
                (_sz < MENU_LOGBOOK_SIZE) && 
                ((addr = LogBook::findprv(addr, _d[_sz])) > 0)
            )
            _sz++;
    }
}



void MenuLogBook::MenuLogBookInfo::updinf() {
    int i = _s.ipos(_s._isel);
    _l =
        (i >= 0) && (i < MENU_LOGBOOK_SIZE) ?
            _s._d[i] :
            LogBook::item_t();
}

MenuLogBook::MenuLogBookInfo::MenuLogBookInfo(MenuLogBook &s) : _s(s)
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
    snprintf(s, sizeof(s), "%d:%02d", _l.toffsec / 60, _l.toffsec % 60);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TIMEFF);
    snprintf(s, sizeof(s), TXT_LOGBOOK_TIMESEC, _l.ffsec);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);

    y += 10;
    DSPL_STRU(0, y, TXT_LOGBOOK_TIMECNP);
    snprintf(s, sizeof(s), TXT_LOGBOOK_TIMESEC, _l.cnpsec);
    DSPL_STRU(DSPL_S_RIGHT(s), y, s);
}

void MenuLogBook::MenuLogBookInfo::smplup() {
    _s.smplup();
    int i = _s.ipos(_s._isel);
    if ((i < 0) || (i >= MENU_LOGBOOK_SIZE))
        _s.smplup(); // попали на exit, кликнем ещё
    updinf();
}

void MenuLogBook::MenuLogBookInfo::smpldn() {
    _s.smpldn();
    int i = _s.ipos(_s._isel);
    if ((i < 0) || (i >= MENU_LOGBOOK_SIZE))
        _s.smpldn(); // попали на exit, кликнем ещё
    updinf();
}

void MenuLogBook::MenuLogBookInfo::smplsel() {
    close();
}

#endif // defined(USE_MENU) && defined(USE_LOGBOOK)
