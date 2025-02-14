/*
    Display: Menu LogBook
*/

#ifndef _view_menu_logbook_H
#define _view_menu_logbook_H

#include "menu.h"
#include "../jump/logbook.h"

#if defined(USE_MENU) && defined(USE_LOGBOOK)

#define MENU_LOGBOOK_SIZE   10
#define MENU_LOGBOOK_NEXT   (MENU_LOGBOOK_SIZE / 2)

class MenuLogBook : public Menu {
    size_t sz() { return _sz + (_prv > 0 ? 1 : 0) + (_nxt > 0 ? 1 : 0); }
    void title(char *s);
    void str(line_t &s, int16_t i);
    void onsel(int16_t i);

    size_t _sz;
    LogBook::item_t _d[MENU_LOGBOOK_SIZE];
    uint32_t _prv, _nxt;

    class MenuLogBookInfo : public Menu {
        MenuLogBook &_s;
        LogBook::item_t _l;
        void updinf();

        public:
            MenuLogBookInfo(MenuLogBook &s);
            void draw(DSPL_ARG);
            void smplup();
            void smpldn();
            void smplsel();
    };

    public:
        MenuLogBook();
};

#endif // defined(USE_MENU) && defined(USE_LOGBOOK)

#endif // _view_menu_logbook_H
