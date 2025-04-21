#include "saver.h"
#include "../def.h"
#include "../sys/log.h"
#include "../sdcard/fshnd.h"
#include "../jump/proc.h"
#include "../jump/altcalc.h"
#include "../view/text.h"
#include "../sys/maincfg.h"
#include "../sys/clock.h"
#include "../sys/sproc.h"

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

    fs::File fh(SDCARD_LOGBOOK, FA_OPEN_APPEND | FA_WRITE);
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
static fs::File _ftr;
static jmp::log_ring_t::snapcursor _log;
static unsigned int _log_sz = 0, _log_cnt = 0;

static void _saving_close() {
    _ftr.close();
    fs::stop();
}

static bool _saving_trace() {
    if (!_ftr)
        return false;
    
    auto &l = *_log;
    
    char mclc[10], mchg[10];
    if (l.mclc)
        snprintf(mclc, sizeof(mclc), "\"%c\"", l.mclc);
    else
        mclc[0] = '\0';
    if (l.mchg)
        snprintf(mchg, sizeof(mchg), "\"%c\"", l.mchg);
    else
        mchg[0] = '\0';
    
    char s[32];
    auto sz = snprintf(s, sizeof(s), "%d,%s,%s\n", l.alt, mclc, mchg);
    if (!_ftr.write(s, sz)) {
        _saving_close();
        return false;
    }

    _log_sz += sz;
    _log_cnt ++;
    
    ++_log;
    if (_log.availr() <= 0) {
        _saving_close();
        CONSOLE("writed: %d lines, %d bytes", _log_cnt, _log_sz);
        return false;
    }

    return true;
}

static void _save_trace(bool byjmp = true) {
    char fname[64];

    if (byjmp) {
        auto _l = jmp::last();
        const auto &tm = _l.tm;

        snprintf(fname, sizeof(fname), "jump_%lu_%u-%02u-%04u_%u-%02u.csv", _l.num, tm.day, tm.mon, tm.year, tm.h, tm.m);
    }
    else {
        auto tm = tmNow();

        snprintf(fname, sizeof(fname), "jump_%lu_%u-%02u-%04u_%u-%02u-%02d_man.csv", cfg->jmpcnt, tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s);
    }

    if (!_ftr.open(fname, FA_CREATE_ALWAYS | FA_WRITE)) {
        fs::stop();
        return;
    }
    
    char s[128];
    
    auto sz = snprintf(s, sizeof(s), 
        "\"alt\",\"calculated change\",\"factical mode\"\n");
    if (!_ftr.write(s, sz)) {
        _saving_close();
        return;
    }
    
    _log = jmp::trace().snapbeg();
    _log_sz = sz, _log_cnt = 0;

    proc::add(_saving_trace);
}
#endif // USE_JMPTRACE

namespace jsave {
    bool isactive() {
        return _ftr;
    }

#ifdef USE_JMPTRACE
    void trace() {
        if (!fs::mount())
            return;

        _save_trace(false);
    }
#endif // USE_JMPTRACE

    void full() {
        if (!fs::mount())
            return;

#ifdef FWVER_DEBUG
        uint32_t fre;
        FATFS* fsi = NULL;
        FR(f_getfree("", &fre, &fsi), return); // Warning! This fills fs.n_fatent and fs.csize!
        CONSOLE("SD mount OK: total = %lu Mb; free = %lu Mb", (fs::inf().n_fatent - 2) * fs::inf().csize / 2048, fre * fs::inf().csize / 2048);

        _dumpdir();
#endif

#ifdef USE_LOGBOOK
        _save_logbook();
#endif // USE_LOGBOOK

#ifdef USE_JMPTRACE
        _save_trace(true);
#endif // USE_JMPTRACE
    }

}
