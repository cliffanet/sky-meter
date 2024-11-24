/*
    common
*/

#ifndef _def_H
#define _def_H

#define STRINGIFY(x)        #x
#define TOSTRING(x)         STRINGIFY(x)
#define PPCAT_NX(A, B)      A ## B

#define vmap(value, low1, high1, low2, high2) \
                            (low2 + (value - low1) * (high2 - low2) / (high1 - low1))

#ifndef HWVER
#define HWVER               1
#endif

#if !defined(FWVER_NUM1) && !defined(FWVER_NUM2) && !defined(FWVER_NUM3)
#define FWVER_NUM1          1
#define FWVER_NUM2          0
#define FWVER_NUM3          0
#define FWVER_DEBUG
#endif

#ifndef FWVER_LANG
#define FWVER_LANG          'R'
#endif

/* FW version name */
#if defined(FWVER_NUM1) && defined(FWVER_NUM2) && defined(FWVER_NUM3)

#define FWVER_NUM           FWVER_NUM1.FWVER_NUM2.FWVER_NUM3

#ifdef FWVER_DEBUG
#define FWVER_TYPE_NAME     ".debug"
#define FWVER_TYPE_DSPL     "debug"
#define FWVER_TYPE_CODE     'd'
#else
#define FWVER_TYPE_NAME     ""
#define FWVER_TYPE_DSPL     "prod"
#define FWVER_TYPE_CODE     'p'
#endif

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define FWVER_LANG_NAME     ".ru"
#else
#define FWVER_LANG_NAME     ".en"
#endif

#define FWVER_NAME          TOSTRING(FWVER_NUM) ".hw" TOSTRING(HWVER) FWVER_TYPE_NAME FWVER_LANG_NAME
#define FWVER_FILENAME      "sky-meter.v" FWVER_NAME

#endif

#define USE_CONSOLE
#define USE_DEVMENU
#define USE_MENU
#define USE_JMPINFO
#define USE_JMPTRACE
#define USE_LOGBOOK

#endif // _def_H
