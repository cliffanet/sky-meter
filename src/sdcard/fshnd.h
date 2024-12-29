#ifndef _sdcard_fshnd_H
#define _sdcard_fshnd_H

#include "ff.h"

#define FR(func, fail)      { CONSOLE(TOSTRING(func)); auto st = func; if (st != FR_OK) { CONSOLE(TOSTRING(func) "failed: %d", st); fail; } }
#define F(func)            FR(func, ;)

class FSMount {
    FATFS _fs = { 0 };
    bool _ok;
    char _path[16];

    public:
        typedef enum {
            CT_UNKNOWN  = -1,
            CT_NONE     = 0,
            CT_MMC3     = 0x01,		/* MMC ver 3 */
            CT_MMC4     = 0x02,		/* MMC ver 4+ */
            CT_MMC      = 0x03,		/* MMC */
            CT_SDC1     = 0x04,		/* SD ver 1 */
            CT_SDC2     = 0x08,		/* SD ver 2+ */
            CT_SDC      = 0x0C,		/* SD */
            CT_BLOCK    = 0x10,		/* Block addressing */
            CT_SDC2BL   = 0x10|0x08 /* SD ver 2+, Block addressing */
        } ctype_t;

        FSMount(const char *path);
        ~FSMount();
        operator bool() const { return _ok; };
        FATFS * operator&() { return &_fs; }
        const FATFS * operator->() const { return &_fs; }

        ctype_t ctype();
};

class FSFile {
    FIL _fh = { 0 };
    bool _ok;

    public:
        FSFile(const TCHAR *path, BYTE mode = FA_READ);
        ~FSFile();
        operator bool() const { return _ok; };
        FIL * operator&() { return &_fh; }
        const FIL * operator->() const { return &_fh; }
        bool write(const void *buff, UINT btw);
};

#endif // _sdcard_fshnd_H
