#ifndef _jump_main_H
#define _jump_main_H

#ifdef __cplusplus

#if _WIN32
#define FFI_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define FFI_PLUGIN_EXPORT extern "C"
#endif

#else

#define FFI_PLUGIN_EXPORT

#endif

typedef struct {
    unsigned long interval;
    float   alt;
    double  sqdiff;

    struct {
        float   alt;
        float   speed;
    } avg;
    struct {
        float   alt;
        float   speed;
    } app;
    struct {
        float   alt;
        float   speed;
    } a05;
    struct {
        float   alt;
        float   speed;
    } a10;

    struct {
        char mode;
        unsigned long cnt, tm;
        unsigned long newcnt, newtm;
    } jmp;

    struct {
        double val;
        unsigned char isbig;
        unsigned long cnt, tm;
    } sq;
} JumpInf;

FFI_PLUGIN_EXPORT void jump_add(float alt);
FFI_PLUGIN_EXPORT JumpInf jump_info();
FFI_PLUGIN_EXPORT void jump_clear();

#endif // _jump_main_H
