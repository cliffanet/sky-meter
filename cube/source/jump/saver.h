#ifndef _sdcard_saver_H
#define _sdcard_saver_H

#define SDCARD_LOGBOOK  "logbook.csv"

namespace jsave {
    bool isactive();

#ifdef USE_JMPTRACE
    void trace();
#endif
    void full();
}

#endif /* _sdcard_saver_H */
