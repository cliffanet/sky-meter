/*
    Log
*/

#ifndef _sys_log_H
#define _sys_log_H

#include "../../def.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifdef USE_CONSOLE

const char * __extrfname(const char * path);
#define LOG_FMT(s)        "[%s:%u] %s(): " s, __extrfname(__FILE__), __LINE__, __FUNCTION__

void conslog(const char *s, ...);
#define CONSOLE(txt, ...) conslog(LOG_FMT(txt), ##__VA_ARGS__)

#else // USE_CONSOLE

#define CONSOLE(txt, ...)

#endif // USE_CONSOLE

#ifdef __cplusplus
}
#endif

#endif // _sys_log_H
