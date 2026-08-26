#ifndef _DC_UTILS_LOG_
#define _DC_UTILS_LOG_

#ifdef __cplusplus
extern "C" {
#endif

//~ log levels

typedef enum {
    DC_LOG_LEVEL_DEBUG,
    DC_LOG_LEVEL_INFO,
    DC_LOG_LEVEL_WARN,
    DC_LOG_LEVEL_ERROR,
} DcLogLevel;

//~ logging

// ignore messages below the selected level
void dc_log_set_level(DcLogLevel level);

DcLogLevel dc_log_get_level(void);

// use a negative value to detect terminal color support
void dc_log_set_colors(int enabled);

// direct log entry point used by the convenience macros
void dc_log(DcLogLevel level, const char *tag, const char *fmt, ...);

//~ convenience macros

#define DC_LOG_DEBUG(tag, fmt, ...) dc_log(DC_LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_INFO(tag, fmt, ...) dc_log(DC_LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_WARN(tag, fmt, ...) dc_log(DC_LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_ERROR(tag, fmt, ...) dc_log(DC_LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
