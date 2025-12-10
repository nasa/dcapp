#include "sleep.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void dc_utils_sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}
