/*
    Display: Menu Modal
*/

#include "menumodal.h"
#include "menu.h"
#include "../sys/log.h"
#include <string.h>

MenuModal::MenuModal(uint8_t lncnt) :
    _lncnt(lncnt)
{
    Menu::line_t l;
    if (Menu::prevstr(l, 0))
        strncpy(_title, l.name, sizeof(_title));
    else
        _title[0] = '\0';
    _title[sizeof(_title)-1] = '\0';
    CONSOLE("lncnt: %d, title: %s", _lncnt, _title);

    Menu::modalset(this);
}

void MenuModal::close()
{
    Menu::modalclose();
}

void MenuModal::draw(DSPL_ARG) {
    int w = DSPL_DWIDTH - 30;
    int h = _lncnt*12+28;
    if (_title[0] != '\0')
        h += 18;
    if (h > DSPL_DHEIGHT)
        h = DSPL_DHEIGHT;
    
    DSPL_COLOR(0);
    DSPL_BOX((DSPL_DWIDTH-w) / 2, (DSPL_DHEIGHT-h) / 2, w, h);

    DSPL_COLOR(1);
    w -= 8;
    h -= 8;
    DSPL_RFRAME((DSPL_DWIDTH-w) / 2, (DSPL_DHEIGHT-h) / 2, w, h, 8);

    w -= 4;
    h -= 4;
    DSPL_RFRAME((DSPL_DWIDTH-w) / 2, (DSPL_DHEIGHT-h) / 2, w, h, 8);

    int y = (DSPL_DHEIGHT-h) / 2 + 6 + 11; // +10 - координата текста в нижнем левом углу

    if (_title[0] != '\0') {
        DSPL_STRU(DSPL_S_CENTER(_title), y, _title);
        y+=18;
    }

    for (uint8_t n=1; n<=_lncnt; n++, y+=12) {
        char s[MENUSZ_MODAL];
        s[0] = '\n';
        str(s, n);
        if (s[0] == '\0')
            continue;
        s[MENUSZ_MODAL-1] = '\0';

        DSPL_STRU(DSPL_S_CENTER(s), y, s);
    }
}

/*
    Confirm
*/

MenuConfirm::MenuConfirm(hnd_t ok, const char *s) :
    MenuModal(1),
    _ok(ok)
{
    strncpy(_txt, s ? s : TXT_MENU_CONFIRM, sizeof(_txt));
    _txt[sizeof(_txt)-1] = '\0';
}

void MenuConfirm::str(char *s, uint8_t n) {
    if (n == 1)
        strncpy(s, _txt, MENUSZ_MODAL);
}

void MenuConfirm::smplup() {
    close();
}

void MenuConfirm::smpldn() {
    close();
}

void MenuConfirm::smplsel() {
    if (_ok != NULL)
        _ok();
    close();
}

/*
    ValBool
*/

MenuValBool::MenuValBool(bool v, hnd_t ok) :
    MenuModal(2),
    _val(v),
    _ok(ok)
{ }

void MenuValBool::draw(DSPL_ARG) {
    MenuModal::draw(u8g2);

    int y = 45;
    if (_val)
        y += 12;
    const int w = 64;
    
    DSPL_FRAME((DSPL_DWIDTH-w) / 2, y, w, 11);
}

void MenuValBool::str(char *s, uint8_t n) {
    strncpy(s, n == 1 ? TXT_MENU_NO : TXT_MENU_YES, MENUSZ_MODAL);
}

void MenuValBool::smplup() {
    _val = !_val;
    if (_ok != NULL)
        _ok(_val);
}

void MenuValBool::smpldn() {
    _val = !_val;
    if (_ok != NULL)
        _ok(_val);
}

/*
    ValInt
*/

MenuValInt::MenuValInt(int v, hnd_t ok, int min, int max) :
    MenuModal(1),
    _val(v),
    _ok(ok),
    _min(min),
    _max(max)
{ }

void MenuValInt::str(char *s, uint8_t n) {
    if (n == 1)
        snprintf(s, MENUSZ_MODAL, "%d", _val);
}

void MenuValInt::smplup() {
    if (_val >= _max)
        return;
    _val ++;
    if (_ok != NULL)
        _ok(_val);
}

void MenuValInt::smpldn() {
    if (_val <= _min)
        return;
    _val --;
    if (_ok != NULL)
        _ok(_val);
}