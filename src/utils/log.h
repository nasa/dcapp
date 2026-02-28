#ifndef _DC_UTILS_LOG_
#define _DC_UTILS_LOG_

#ifdef __cplusplus
extern "C" {
#endif

// Log levels
typedef enum {
    DC_LOG_LEVEL_DEBUG,
    DC_LOG_LEVEL_INFO,
    DC_LOG_LEVEL_WARN,
    DC_LOG_LEVEL_ERROR,
} DcLogLevel;

// Set minimum log level (messages below this level are ignored)
void dc_log_set_level(DcLogLevel level);

// Get current log level
DcLogLevel dc_log_get_level(void);

// Enable/disable colored output (-1 = auto, 0 = off, 1 = on)
void dc_log_set_colors(int enabled);

// Internal logging function - use macros below instead
void dc_log(DcLogLevel level, const char *tag, const char *fmt, ...);

// Convenience macros
#define DC_LOG_DEBUG(tag, fmt, ...) dc_log(DC_LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_INFO(tag, fmt, ...) dc_log(DC_LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_WARN(tag, fmt, ...) dc_log(DC_LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define DC_LOG_ERROR(tag, fmt, ...) dc_log(DC_LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // _DC_UTILS_LOG_
