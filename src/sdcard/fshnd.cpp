#include "fshnd.h"
#include "sys/log.h"

#include <string.h>
#include "fshnd.h"

extern "C" {
    int32_t MX_FATFS_Init(void);
    extern char USERPath[4];
    uint8_t FATFS_UnLinkDriver(char *path);
};

FSMount::FSMount(const char *path) {
    strncpy(_path, path, sizeof(_path));
    _path[sizeof(_path)-1] = '\0';

    /*
    FATFS_UnLinkDriver(USERPath);
    _ok = MX_FATFS_Init() == 0;
    if (!_ok) {
        // это нужно, чтобы сбросить глобальные переменные FATFS,
        // иначе после выдёргивании sd-карты она больше уже не
        // подключается, пока не перезагрузим устройство.
        CONSOLE("MX_FATFS_Init fail");
        return;
    }
    */

    // mount the default drive
    FRESULT r = f_mount(&_fs, _path, 1);
    _ok = r == FR_OK;
    CONSOLE("mount: %d", r);
    if (!_ok)
        CONSOLE("f_mount(\"%s\") failed: %d", _path, r);
}

FSMount::~FSMount() {
    FRESULT r = f_mount(NULL, _path, 0);
    if (r != FR_OK)
        CONSOLE("Unmount failed, res = %d", r);
    
    CONSOLE("unmount: %d", r);
}

FSFile::FSFile(const TCHAR *path, BYTE mode) {
    CONSOLE("open file: %s", path);
    FR(f_open(&_fh, path, mode), _ok=false; return);
    _ok = true;
}

FSFile::~FSFile() {
    if (!_ok)
        return;
    F(f_close(&_fh));
}

bool FSFile::write(const void *buff, UINT btw) {
    UINT szw;
    
    FR(f_write(&_fh, buff, btw, &szw), return false);
    if (btw != szw) {
        CONSOLE("file write fail: %u => %u", btw, szw);
        return false;
    }

    return true;
}
