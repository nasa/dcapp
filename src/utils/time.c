#include "time.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif
#endif

void dc_utils_sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

static int    _time_initialized = 0;
static double _time_origin      = 0.0;

#ifdef _WIN32

static double _time_frequency = 0.0;

static double _time_get_raw(void) {
    if (_time_frequency == 0.0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        _time_frequency = (double)freq.QuadPart;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / _time_frequency;
}

#elif defined(__APPLE__)

static double _time_mach_scale = 0.0;

static double _time_get_raw(void) {
    if (_time_mach_scale == 0.0) {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        _time_mach_scale = (double)info.numer / (double)info.denom / 1e9;
    }
    return (double)mach_absolute_time() * _time_mach_scale;
}

#else // Linux / POSIX

static double _time_get_raw(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#endif

double dc_utils_time_get(void) {
    if (!_time_initialized) {
        _time_origin      = _time_get_raw();
        _time_initialized = 1;
    }
    return _time_get_raw() - _time_origin;
}

void dc_utils_time_reset(void) {
    _time_origin      = _time_get_raw();
    _time_initialized = 1;
}
