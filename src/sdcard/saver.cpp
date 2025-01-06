#include "saver.h"
#include "../def.h"
#include "fshnd.h"
#include "../sys/log.h"
#include "../jump/proc.h"
#include "../jump/altcalc.h"
#include "../view/text.h"

#include "ff.h"
#include <string>
#include <string.h>

#include "../ff/diskio.h"


#ifdef FWVER_DEBUG
static void _dumpdir() {
    DIR dh;
    FR(f_opendir(&dh, "/"), return);

    FILINFO finf;
    uint32_t dcnt = 0, fcnt = 0;
    CONSOLE("Root directory:");
    for (;;) {
        if (f_readdir(&dh, &finf) != FR_OK)
            break;
        if (finf.fname[0] == '\0')
            break;
        
        if (finf.fattrib & AM_DIR) {
            CONSOLE("  DIR  %s", finf.fname);
            dcnt++;
        } else {
            CONSOLE("  FILE %s", finf.fname);
            fcnt++;
        }
    }

    CONSOLE("total: %lu dirs, %lu files", dcnt, fcnt);

    F(f_closedir(&dh))
}
#endif

#ifdef USE_LOGBOOK
static void _save_logbook() {
    CONSOLE("Writing to " SDCARD_LOGBOOK "...");

    auto _l = jmp::last();

    FILINFO fi;
    bool ex = f_stat(SDCARD_LOGBOOK, &fi) == FR_OK;

    FSFile fh(SDCARD_LOGBOOK, FA_OPEN_APPEND | FA_WRITE);
    if (!fh)
        return;

    char s[128];
    unsigned int sz;
    if (!ex) {
        sz = snprintf(s, sizeof(s), 
            "\"Date/time\",\"Jump number\",\"Takeoff time\","
            "\"Begin alt\",\"FreeFall time\",\"Canopy alt\",\"Canopy time\"\n");
        if (!fh.write(s, sz))
            return;
    }

    const auto &tm = _l.tm;
    sz = snprintf(s, sizeof(s), 
            "\"%2u.%02u.%04u %2u:%02u:%02u\",%lu,\"" TXT_LOGBOOK_MINSEC "\","
            "%u,%u,%u,\"" TXT_LOGBOOK_MINSEC "\"\n",
            tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s,
            _l.num,
            _l.toffsec / 60, _l.toffsec % 60,
            _l.begalt, _l.ffsec,
            _l.cnpalt,
            _l.cnpsec / 60, _l.cnpsec % 60
        );
    if (!fh.write(s, sz))
        return;
}
#endif // USE_LOGBOOK

#ifdef USE_JMPTRACE
static void _save_trace() {
    auto _l = jmp::last();
    const auto &tm = _l.tm;

    char fname[64];
    snprintf(fname, sizeof(fname), "jump_%u_%u-%02u-%04u_%u-%02u.csv", _l.num, tm.day, tm.mon, tm.year, tm.h, tm.m);

    FSFile fh(fname, FA_CREATE_ALWAYS | FA_WRITE);
    if (!fh)
        return;
    
    char s[128];
    unsigned int sz, szf = 0, cnt = 0;
    
    sz = snprintf(s, sizeof(s), 
        "\"alt\",\"calculated change\",\"factical mode\"\n");
    if (!fh.write(s, sz))
        return;
    szf += sz;
    
    auto &_log = jmp::trace();
    for (int i = _log.size()-1; i >= 0; i--) {
        auto &l = _log[i];
        
        char mclc[10], mchg[10];
        if (l.mclc)
            snprintf(mclc, sizeof(mclc), "\"%c\"", l.mclc);
        else
            mclc[0] = '\0';
        if (l.mchg)
            snprintf(mchg, sizeof(mchg), "\"%c\"", l.mchg);
        else
            mchg[0] = '\0';
        
        sz = snprintf(s, sizeof(s), "%d,%s,%s\n", l.alt, mclc, mchg);
        if (!fh.write(s, sz))
            return;
        szf += sz;
        cnt ++;
    }

    CONSOLE("writed: %d lines, %d bytes", cnt, szf);
}
#endif // USE_JMPTRACE

void sdcard_save() {
    FSMount fs("");

    if (!fs)
        return;

    uint32_t fre;
    FATFS* fsi = NULL;
    FR(f_getfree("", &fre, &fsi), return); // Warning! This fills fs.n_fatent and fs.csize!
    CONSOLE("SD mount OK: total = %lu Mb; free = %lu Mb", (fs->n_fatent - 2) * fs->csize / 2048, fre * fs->csize / 2048);

#ifdef FWVER_DEBUG
    _dumpdir();
#endif

#ifdef USE_LOGBOOK
    _save_logbook();
#endif // USE_LOGBOOK

#ifdef USE_JMPTRACE
    _save_trace();
#endif // USE_JMPTRACE

    CONSOLE("Done!");
}
