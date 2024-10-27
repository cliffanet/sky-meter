/*
    Display: Menu Modal
*/

#ifndef _view_menu_modal_H
#define _view_menu_modal_H

#include "dspl.h"

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define MENUSZ_MODAL    128
#else
#define MENUSZ_MODAL    64
#endif


class MenuModal {
    uint8_t _lncnt;
    public:
        MenuModal(uint8_t lncnt = 1);
        void close();

        void draw(DSPL_ARG);
        virtual void str(char *s, uint8_t n) = 0; // n = 1 .. _lncnt
        virtual void smplup() {}
        virtual void smpldn() {}
        virtual void smplsel() { close(); }
};

class MenuConfirm : public MenuModal {
    public:
        typedef void (*hnd_t)();
        MenuConfirm(const char *s, hnd_t ok);
        void str(char *s, uint8_t n);
        void smplup();
        void smpldn();
        void smplsel();
    private:
        char _txt[MENUSZ_MODAL];
        hnd_t _ok;
};

#endif // _view_menu_modal_H
