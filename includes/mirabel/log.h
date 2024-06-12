#pragma once

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LOGS_E {
    LOGS_LESS = 0,
    LOGS_NORM,
    LOGS_OK,
    LOGS_INFO,
    LOGS_WARN,
    LOGS_ERR,
    LOGS_FATAL,
    LOGS_COUNT,

    LOGS_TYPE_MASK = 0b111,

    LOGS_STYLE_BOLD = 0b001000,
    LOGS_STYLE_LINE_COLORED = 0b010000,
    LOGS_STYLE_INVERT = 0b100000,
} LOGS;

// uint32_t mirabel_log_register(const char* name);
// void mirabel_log_unregister(uint32_t log_id);

// void mirabel_log(uint32_t log_id, LOGS status, const char* str, const char* str_end);
// void mirabel_logf(uint32_t log_id, LOGS status, const char* fmt, ...);
// void mirabel_vlogf(uint32_t log_id, LOGS status, const char* fmt, va_list args);

void mirabel_slog(LOGS status, const char* str, const char* str_end);
void mirabel_slogf(LOGS status, const char* fmt, ...);
void mirabel_svlogf(LOGS status, const char* fmt, va_list args);

//TODO multi logging works via threadlocal, you can only have one active multi log, but you CAN still log with the single line functions
// time_now, true to register time and ordering directly as opposed to at stop
// void mirabel_mlog_start(uint32_t log_id, LOGS status, bool time_now);
// void mirabel_mlog(const char* str, const char* str_end);
// void mirabel_mlogf(const char* fmt, ...);
// void mirabel_mvlogf(const char* fmt, va_list args);
// void mirabel_mlog_stop();

//TODO "complex log objects", which are really just notifications via a separate api, so you can e.g. have a loading bar on the notify popup

#ifdef __cplusplus
}
#endif
