/*
    View: Text lang
*/

#ifndef _view_text_lang_H
#define _view_text_lang_H

#include "../def.h"
#include <stdint.h>
#include "../u8g2/u8g2.h"
extern const uint8_t u8g2_font_robotobd_08_tx[] U8G2_FONT_SECTION("u8g2_font_robotobd_08_tx");

//#define menuFont        u8g2_font_6x12_t_cyrillic
#define menuFont        u8g2_font_robotobd_08_tx

#define TXT_MENU_EXIT               "> выход"
#define TXT_MENU_CONFIRM            "Подтверждаете?"
#define TXT_MENU_ON                 "вкл"
#define TXT_MENU_OFF                "выкл"
#define TXT_MENU_YES                "Да"
#define TXT_MENU_NO                 "Нет"
#define TXT_MENU_OK                 "OK"
#define TXT_MENU_FAIL               "ошибка"

#define TXT_MAIN_JMPCNT             "Кол-во прыжков"
#define TXT_MAIN_LOGBOOK            "LogBook"
#define TXT_MAIN_FLIP180            "Развернуть на 180"
#define TXT_MAIN_CONTRAST           "Контраст"
#define TXT_MAIN_ALT                "Высотомер"
#define TXT_MAIN_BARGRAPH           "Барометр"
#define TXT_MAIN_SYSTEM             "Система"

#define TXT_ALT_AUTOGND             "Автоподстройка Земли"
#define TXT_ALT_METER               "Высота (<1000) в метрах"
#define TXT_ALT_CORRECT             "Превышение площадки"
#define TXT_ALT_RESETGND            "Сбросить уровень Земли"
#define TXT_ALT_RESETMODE           "Сбросить режим работы"
#define TXT_ALT_SAVETRACE           "Сохранить трассировку"

#define TXT_SYSTEM_DATETIME         "Изменить часы"
#define TXT_SYSTEM_LOPWRONJMP       "Энергосбер. в прыжке"
#define TXT_SYSTEM_RESET            "Перезагрузить"
#define TXT_SYSTEM_POWEROFF         "Выкл. питания"
#define TXT_SYSTEM_CFGDEFAULT       "Сбросить настройки"
#define TXT_SYSTEM_HWTEST           "Тестирование аппарат."

#define TXT_TEST_SERIAL             "с/н"
#define TXT_TEST_FLASHSZ            "Размер flash"
#define TXT_TEST_FWVER              "FW ver"
#define TXT_TEST_CLOCK              "Часы"
#define TXT_TEST_BATTERY            "Батарея"
#define TXT_TEST_BATTCHARGE         "Подкл. зарядка"
#define TXT_TEST_PRESSID            "ID барометра"
#define TXT_TEST_PRESSURE           "Давление"
#define TXT_TEST_PRESSVAL           "(%0.0f Па) %s"
#define TXT_TEST_PTEMP              "Температура"
#define TXT_TEST_PTEMPVAL           "%0.1f"
#define TXT_TEST_LIGHT              "Подсветка"
#define TXT_TEST_SDCARD             "SD карта"

#define TXT_LOGBOOK_JUMPNUM         "Прыжок # %ld"
#define TXT_LOGBOOK_ALTEXIT         "Отделение"
#define TXT_LOGBOOK_ALTCNP          "Раскрытие"
#define TXT_LOGBOOK_TIMETOFF        "Длит. взлёта"
#define TXT_LOGBOOK_TIMEFF          "Длит. своб.п."
#define TXT_LOGBOOK_TIMESEC         "%d с"
#define TXT_LOGBOOK_MINSEC          "%d:%02d"
#define TXT_LOGBOOK_TIMECNP         "Длит. пилотир"
#define TXT_LOGBOOK_TRACE           "Трассировка"

#endif // _view_text_lang_H
