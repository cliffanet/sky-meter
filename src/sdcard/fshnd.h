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
        FSMount(const char *path);
        ~FSMount();
        operator bool() const { return _ok; };
        FATFS * operator&() { return &_fs; }
        const FATFS * operator->() const { return &_fs; }
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
