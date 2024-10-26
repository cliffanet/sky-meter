/*
    Display: Menu
*/

#include "menu.h"
#include "page.h"
#include "btn.h"
#include "text.h"
#include "../sys/log.h"

#include <string.h>

static Menu *_m = NULL;

static bool _e = false;
void chk_e() {
    if (_e) {
        if (_m != NULL) {
            CONSOLE("menu exit: 0x%08x", _m);
            delete _m;
            _m = NULL;
            Dspl::page();
        }
        _e = false;
    }
}

void Menu::activate() {
    _m = new Menu();
    CONSOLE("menu enter: 0x%08x", _m);

    Dspl::set([] (DSPL_ARG) {
        if (_m != NULL)
            _m->draw(u8g2);
    });

    Btn::set(
        Btn::UP,
        [] () {
            if (_m != NULL)
                _m->smplup();
            chk_e();
        }
    );
    Btn::set(
        Btn::SEL,
        [] () {
            if (_m != NULL)
                _m->smplsel();
            chk_e();
        }
    );
    Btn::set(
        Btn::DN,
        [] () {
            if (_m != NULL)
                _m->smpldn();
            chk_e();
        }
    );
}

Menu::Menu(exit_t _exit) :
    _exit(_exit)
{ }

#define _sz     (_exit == EXIT_NONE ? sz()+1 : sz())

void Menu::draw(DSPL_ARG) {
    DSPL_FONT(menuFont);
    
    // Заголовок
    char t[MENUSZ_TITLE];
    title(t);
    DSPL_BOX(0, 0, DSPL_DWIDTH, 12);
    DSPL_COLOR(0);
    DSPL_STRU(DSPL_S_CENTER(t), 10, t);
    
    // выделение пункта меню, текст будем писать поверх
    DSPL_BOX(0, (_isel-_itop)*10+14, DSPL_DWIDTH, 10);
    DSPL_COLOR(1);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        uint8_t i = n + _itop;          // i - индекс отрисовываемого пункта меню
        if (i >= _sz) break;           // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        DSPL_COLOR(_isel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;       // координата y для отрисовки текста (в нескольких местах используется)
        
        // Получение текстовых данных текущей строки
        // Раньше мы эти строчки кешировали в массиве, обновляя
        // этот массив каждый раз при смещении меню, но
        // в итоге от этого кеширования отказались, т.к. часто
        // данные требуют оперативного обновления, а затраты на
        // их получение минимальны, при этом очень много гемора,
        // связанного с обновлением кеша
        line_t m;
        bool isexit = ((_exit == EXIT_TOP) && (i == 0)) || ((_exit == EXIT_BOTTOM) && (i+1 >= _sz));
        if (isexit) {
            strncpy(m.name, TXT_MENU_EXIT, sizeof(m.name));
            m.name[sizeof(m.name)-1] = '\0';
            m.val[0] = '\0';
        }
        else {
            str(m, _exit == EXIT_TOP ? i-1 : i);
            m.name[sizeof(m.name)-1] = '\0';
            m.val[sizeof(m.val)-1] = '\0';
        }

        DSPL_STRU(2, y, m.name);
        if (m.val[0] != '\0')   // вывод значения
            DSPL_STRU(DSPL_S_RIGHT(m.val)-2, y, m.val);
    }
    DSPL_COLOR(1);
}

void Menu::smplup() {
    _isel --;
    if (_isel >= 0) {
        if (_itop > _isel)  // если вылезли вверх за видимую область,
            _itop = _isel;  // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else {                  // если упёрлись в самый верх, перескакиваем в конец меню
        _isel = _sz-1;
        if (_sz > MENU_STR_COUNT)
            _itop = _sz-MENU_STR_COUNT;
    }
}

void Menu::smpldn() {
    _isel ++;
    if (_isel < _sz) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((_isel > _itop) && ((_isel-_itop) >= MENU_STR_COUNT))
            _itop = _isel-MENU_STR_COUNT+1;
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        _isel = 0;
        if (_sz > MENU_STR_COUNT)
            _itop = 0;
    }
}

void Menu::smplsel() {
    bool isexit = ((_exit == EXIT_TOP) && (_isel == 0)) || ((_exit == EXIT_BOTTOM) && (_isel+1 >= _sz));
    if (isexit)
        _e = true;
}
