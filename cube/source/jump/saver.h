#ifndef _sdcard_saver_H
#define _sdcard_saver_H

#include "../def.h"

#define SDCARD_LOGBOOK  "logbook.csv"
#define SDCARD_SYSLOG   "syslog.txt"

#include "logbook.h"
#include <stdlib.h>

namespace jsave {
    bool isactive();

    int logbook2csv(char *s, size_t sz, LogBook::item_t &l);

#ifdef USE_JMPTRACE
    void trace();
#endif
    void full();
}

#endif /* _sdcard_saver_H */
