/*
    Display: Menu Modal
*/

#include "menumodal.h"
#include "menu.h"
#include <string.h>

MenuModal::MenuModal(uint8_t lncnt) :
    _lncnt(lncnt)
{
    Menu::modalset(this);
}

void MenuModal::close()
{
    Menu::modalclose();
}

void MenuModal::draw(DSPL_ARG) {
    int w = DSPL_DWIDTH - 30;
    int h = _lncnt*12+20;
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

    int y = (DSPL_DHEIGHT-h) / 2 + 2 + 11; // +10 - координата текста в нижнем левом углу
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

MenuConfirm::MenuConfirm(const char *s, hnd_t ok) :
    MenuModal(3),
    _ok(ok)
{
    strncpy(_txt, s, sizeof(_txt));
    _txt[sizeof(_txt)-1] = '\0';
}

void MenuConfirm::str(char *s, uint8_t n) {
    if (n == 2)
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
