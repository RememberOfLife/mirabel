#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <mutex>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "mirabel/alloc.h"
#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO this entire impl is one big hack, in reality we'd likely route this through the client

static std::mutex log_lock;
static const size_t LOG_FORMAT_BUF_SIZE = 1024;
static char log_format_buf[LOG_FORMAT_BUF_SIZE];

void mirabel_slog(LOGS status, const char* str, const char* str_end)
{
    if (str_end == NULL) {
        mirabel_slogf(status, "%s", str);
    } else {
        mirabel_slogf(status, "%.*s", str_end - str, str);
    }
}

void mirabel_slogf(LOGS status, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mirabel_svlogf(status, fmt, args);
    va_end(args);
}

void mirabel_svlogf(LOGS status, const char* fmt, va_list args)
{
    log_lock.lock();
    {
        size_t req_len = vsnprintf(NULL, 0, fmt, args) + 1;
        char* target_buf = log_format_buf;
        if (req_len > LOG_FORMAT_BUF_SIZE) {
            target_buf = (char*)mirabel_malloc(req_len);
        }
        vsnprintf(target_buf, req_len, fmt, args);

        // process and output
        bool bold = status & LOGS_STYLE_BOLD;
        bool line_colored = status & LOGS_STYLE_LINE_COLORED;
        bool invert = status & LOGS_STYLE_INVERT;
        status = (LOGS)(status & LOGS_TYPE_MASK);
        //TODO log to the interface
#ifndef __EMSCRIPTEN__
        const char* status_map[LOGS_COUNT] = {
            [LOGS_LESS] = "       ",
            [LOGS_NORM] = "     > ",
            [LOGS_OK] = "[   OK]",
            [LOGS_INFO] = "[ INFO]",
            [LOGS_WARN] = "[ WARN]",
            [LOGS_ERR] = "[ERROR]",
            [LOGS_FATAL] = "[FATAL]",
        };
        fprintf(stdout, "%s: %s\n", status_map[status], target_buf);
#endif
        // log to the web js, //TODO normally the client would do this
        // clang-format off
#ifdef __EMSCRIPTEN__
        EM_ASM({
            log(LOGS.from_int($0), UTF8ToString($1), $2, $3, $4);
        }, status, target_buf, bold, line_colored, invert);
#endif
        // clang-format on

        if (target_buf != log_format_buf) {
            mirabel_free(target_buf);
        }
    }
    log_lock.unlock();
}

#ifdef __cplusplus
}
#endif
