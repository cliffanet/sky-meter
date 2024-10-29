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
    char _title[MENUSZ_MODAL];
    public:
        MenuModal(uint8_t lncnt = 1);
        void close();

        virtual void draw(DSPL_ARG);
        virtual void str(char *s, uint8_t n) = 0; // n = 1 .. _lncnt
        virtual void smplup() {}
        virtual void smpldn() {}
        virtual void smplsel() { close(); }
};

class MenuConfirm : public MenuModal {
    public:
        typedef void (*hnd_t)();
        MenuConfirm(hnd_t ok, const char *s = NULL);
        void str(char *s, uint8_t n);
        void smplup();
        void smpldn();
        void smplsel();
    private:
        char _txt[MENUSZ_MODAL];
        hnd_t _ok;
};

class MenuValBool : public MenuModal {
    public:
        typedef void (*hnd_t)(bool v);
        MenuValBool(bool v, hnd_t ok);
        void draw(DSPL_ARG);
        void str(char *s, uint8_t n);
        void smplup();
        void smpldn();
    private:
        bool _val;
        hnd_t _ok;
};

class MenuValInt : public MenuModal {
    public:
        typedef void (*hnd_t)(int v);
        MenuValInt(int v, hnd_t ok, int min = 0, int max = 10);
        void str(char *s, uint8_t n);
        void smplup();
        void smpldn();
    private:
        int _val, _min, _max;
        hnd_t _ok;
};

#endif // _view_menu_modal_H
