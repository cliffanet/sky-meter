/*
    Display: Menu LogBook
*/

#ifndef _view_menu_logbook_H
#define _view_menu_logbook_H

#include "menu.h"
#include "../jump/logbook.h"
#if HWVER >= 2
#include "../sys/iflash.h"
#endif

#if defined(USE_MENU) && defined(USE_LOGBOOK)

#define MENU_LOGBOOK_SIZE   MENU_STR_COUNT
#define MENU_LOGBOOK_NEXT   (MENU_LOGBOOK_SIZE / 2)

class MenuLogBook : public Menu {
    size_t sz() { return _sz; }
    void title(char *s);
    void str(line_t &s, int16_t i);
    void onsel(int16_t i);

    size_t _sz;
#if HWVER < 2
    LogBook::item_t _d[MENU_LOGBOOK_SIZE];
#else
    struct {
        iflash::Rec     r;
        LogBook::item_t l;
    } _d[MENU_LOGBOOK_SIZE];
#endif

    class MenuLogBookInfo : public Menu {
        MenuLogBook &_s;
        LogBook::item_t _l;
        uint8_t _trc_cnt;
        bool _trc_nat;
        void updtrc();
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
#if HWVER >= 2
        void smplup();
        void smpldn();
#endif
};

#endif // defined(USE_MENU) && defined(USE_LOGBOOK)

#endif // _view_menu_logbook_H
