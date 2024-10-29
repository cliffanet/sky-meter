/*
    View: Text lang
*/

#ifndef _view_text_lang_H
#define _view_text_lang_H

#include "../../def.h"
#include <stdint.h>
#include <../u8g2/u8g2.h>
extern const uint8_t u8g2_font_robotobd_08_tx[] U8G2_FONT_SECTION("u8g2_font_robotobd_08_tx");

//#define menuFont        u8g2_font_6x12_t_cyrillic
#define menuFont        u8g2_font_robotobd_08_tx

#define TXT_MENU_EXIT               "> выход"
#define TXT_MENU_CONFIRM            "Подтверждаете?"
#define TXT_MENU_YES                "Да"
#define TXT_MENU_NO                 "Нет"

#define TXT_MAIN_JMPCNT             "Кол-во прыжков"
#define TXT_MAIN_FLIP180            "Развернуть на 180"
#define TXT_MAIN_CONTRAST           "Контраст"
#define TXT_MAIN_AUTOGND            "Автоподстройка 0-выс."
#define TXT_MAIN_ALTMETER           "Высота (<1000) в метрах"
#define TXT_MAIN_ALTCORRECT         "Превышение площадки"

#endif // _view_text_lang_H
