#ifndef _jump_bargraph_H
#define _jump_bargraph_H

#include <stdint.h>

// Интервал обнуления высоты (ms)
#define BARGRAPH_INTERVAL       300000
// коэфициент перевода в мм.рт.ст.
#define BARGRAPH_MMHG           0.00750063755419211
// количество позиций, за которые высчитывается общая тенденция
// общий интервал тенденции в мс = BARGRAPH_BDIR_CNT * BARGRAPH_INTERVAL
#define BARGRAPH_BDIR_CNT       36


namespace bar {
    void tick(float press, uint32_t ms);
    void setdraw();
} // namespace bar

#endif // _jump_bargraph_H
