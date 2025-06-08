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

    sz = jsave::logbook2csv(s, sizeof(s), _l);
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

static void _trace_name_jmp(char *fname, size_t sz) {
    auto _l = jmp::last();
    const auto &tm = _l.tm;

    snprintf(fname, sz, "jump_%lu_%u-%02u-%04u_%u-%02u.csv", _l.num, tm.day, tm.mon, tm.year, tm.h, tm.m);
}

static void _trace_name_man(char *fname, size_t sz) {
    auto tm = tmNow();

    snprintf(fname, sz, "jump_%lu_%u-%02u-%04u_%u-%02u-%02d_man.csv", cfg->jmpcnt, tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s);
}

static void _save_trace(const char *fname) {
    if (!fs::mount())
        return;

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

    int logbook2csv(char *s, size_t sz, LogBook::item_t &_l) {

        const auto &tm = _l.tm;
        return snprintf(s, sz, 
            "\"%2u.%02u.%04u %2u:%02u:%02u\",%lu,\"" TXT_LOGBOOK_MINSEC "\","
            "%u,%u,%u,\"" TXT_LOGBOOK_MINSEC "\"\n",
            tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s,
            _l.num,
            _l.toffsec / 60, _l.toffsec % 60,
            _l.begalt, _l.ffsec,
            _l.cnpalt,
            _l.cnpsec / 60, _l.cnpsec % 60
        );
    }

#ifdef USE_JMPTRACE
    void trace() {
        char fname[64];
        _trace_name_man(fname, sizeof(fname));
        _save_trace(fname);
    }
#endif // USE_JMPTRACE

    void full() {
#ifdef USE_JMPTRACE
        char fname[64];
        // получить имя файла по текущему прыжку нам надо до выполнения _save_logbook()
        // т.к. она сбрасывает данные о прыжке после себя
        _trace_name_jmp(fname, sizeof(fname));
#endif // USE_JMPTRACE

#ifdef USE_LOGBOOK
        _save_logbook();
#endif // USE_LOGBOOK

#ifdef USE_JMPTRACE
        _save_trace(fname);
#endif // USE_JMPTRACE
    }

}
