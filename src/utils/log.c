#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
    #include <sys/time.h>
#endif

// ANSI color codes
#define ANSI_RESET   "\033[0m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_RED     "\033[31m"

// Current minimum log level (messages below this level are ignored)
static DcLogLevel _dc_log_level = DC_LOG_LEVEL_INFO;
static int _dc_log_colors_enabled = -1; // -1 = auto, 0 = off, 1 = on

#ifdef _WIN32
static int _dc_win_console_initialized = 0;

static void _dc_init_win_console(void) {
    if (_dc_win_console_initialized) return;
    _dc_win_console_initialized = 1;

    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;

    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    if (hErr != INVALID_HANDLE_VALUE && GetConsoleMode(hErr, &mode)) {
        SetConsoleMode(hErr, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}
#endif

static int _dc_should_use_colors(FILE *out) {
    if (_dc_log_colors_enabled == 0) return 0;
    if (_dc_log_colors_enabled == 1) return 1;
    // Auto: only colorize if writing to a terminal
    return isatty(fileno(out));
}

void dc_log_set_level(DcLogLevel level) {
    _dc_log_level = level;
}

DcLogLevel dc_log_get_level(void) {
    return _dc_log_level;
}

void dc_log_set_colors(int enabled) {
    _dc_log_colors_enabled = enabled;
}

void dc_log(DcLogLevel level, const char *tag, const char *fmt, ...) {
    if (level < _dc_log_level) return;

    const char *level_str;
    const char *color = "";
    FILE *out = stderr;

#ifdef _WIN32
    _dc_init_win_console();
#endif

    switch (level) {
        case DC_LOG_LEVEL_DEBUG: level_str = "DEBUG"; color = ANSI_CYAN;   break;
        case DC_LOG_LEVEL_INFO:  level_str = "INFO";  color = ANSI_GREEN;  out = stdout; break;
        case DC_LOG_LEVEL_WARN:  level_str = "WARN";  color = ANSI_YELLOW; break;
        case DC_LOG_LEVEL_ERROR: level_str = "ERROR"; color = ANSI_RED;    break;
        default:                 level_str = "???";   break;
    }

    // timestamp
    char timestamp[16];
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d.%03d",
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_buf;
    localtime_r(&tv.tv_sec, &tm_buf);
    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d.%03d",
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)(tv.tv_usec / 1000));
#endif

    int use_colors = _dc_should_use_colors(out);

    if (use_colors) {
        fprintf(out, "%s %s[DCAPP %s]%s %s: ", timestamp, color, level_str, ANSI_RESET, tag);
    } else {
        fprintf(out, "%s [DCAPP %s] %s: ", timestamp, level_str, tag);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
}
