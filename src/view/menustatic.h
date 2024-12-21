/*
    Display: Menu Static
*/

#ifndef _view_menu_static_H
#define _view_menu_static_H

#include "menu.h"

#ifdef USE_MENU

#define MENU_STATIC(v, ...)  (new MenuStatic(v, sizeof(v)/sizeof(MenuStatic::el_t), ##__VA_ARGS__))

class MenuStatic : public Menu {
    size_t sz() { return _sz; }
    void title(char *s);
    void str(line_t &s, int16_t i);
    void onsel(int16_t i);
    uint32_t timeout() const { return _tout ? Menu::timeout() : 0; }

    public:
        typedef void (*hnd_t)();
        typedef void (*val_t)(char *txt);
        typedef struct {
            const char  *name;      // Текстовое название пункта
            hnd_t       enter;      // Обраотка нажатия на среднюю кнопку
            val_t       showval;    // как отображать значение, если требуется
        } el_t;

        MenuStatic(const el_t *m, int16_t sz, bool tout = true);
        ~MenuStatic();

        static void main();
    
    private:
        char *_title;
        const el_t *_m;
        int16_t _sz;
        bool _tout;
};

#endif // USE_MENU

#endif // _view_menu_static_H
