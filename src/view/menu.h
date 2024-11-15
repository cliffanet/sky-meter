/*
    Display: Menu
*/

#ifndef _view_menu_H
#define _view_menu_H

#include "dspl.h"

#ifdef USE_MENU

#define MENU_STR_COUNT  8

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define MENUSZ_TITLE    128
#define MENUSZ_NAME     96
#define MENUSZ_VAL      32
#else
#define MENUSZ_TITLE    64
#define MENUSZ_NAME     48
#define MENUSZ_VAL      16
#endif

#define MENU_TIMEOUT    150

class MenuModal;

class Menu {
    Menu *_prv = NULL, *_nxt = NULL;
    public:
        typedef struct {
            char name[MENUSZ_NAME] = { '\0' };
            char val[MENUSZ_VAL] = { '\0' };
        } line_t;

        typedef enum {
            EXIT_NONE,
            EXIT_TOP,
            EXIT_BOTTOM,
        } exit_t;

        Menu(exit_t _exit = EXIT_TOP);
        ~Menu();

        void close();

        void draw(DSPL_ARG);
        void smplup();
        void smpldn();
        void smplsel();

        static Menu *prev(uint8_t n);
        static bool prevstr(line_t &s, uint8_t n);

        static bool isactive();
        static void clear();

        static void modalset(MenuModal *m);
        static void modaldel(MenuModal *m);

    private:
        virtual size_t sz() { return 0; }
        virtual void title(char *s) { }
        virtual void str(line_t &s, int16_t i) { }
        virtual void onsel(int16_t i) { }

    protected:
        int16_t _itop = 0, _isel = 0;
        exit_t _exit;
};

#endif // USE_MENU

#endif // _view_menu_H
