#ifndef CC_LOGGER_H
#define CC_LOGGER_H

#pragma clang diagnostic ignored "-Wformat-nonliteral"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h> // for strerror
#include <assert.h>
#include <stdint.h>

#define MAX_LOG_CHARS 2048

// Colors
#define TEXT_COLOR_BLACK 			"\x1b[30m"
#define TEXT_COLOR_RED 				"\x1b[31m"
#define TEXT_COLOR_GREEN 			"\x1b[32m"
#define TEXT_COLOR_YELLOW 			"\x1b[33m"
#define TEXT_COLOR_BLUE 			"\x1b[34m"
#define TEXT_COLOR_MAGENTA 			"\x1b[35m"
#define TEXT_COLOR_CYAN 			"\x1b[36m"
#define TEXT_COLOR_WHITE 			"\x1b[37m"
#define TEXT_COLOR_BRIGHT_BLACK 	"\x1b[90m"
#define TEXT_COLOR_BRIGHT_RED 		"\x1b[91m"
#define TEXT_COLOR_BRIGHT_GREEN 	"\x1b[92m"
#define TEXT_COLOR_BRIGHT_YELLOW 	"\x1b[93m"
#define TEXT_COLOR_BRIGHT_BLUE 		"\x1b[94m"
#define TEXT_COLOR_BRIGHT_MAGENTA 	"\x1b[95m"
#define TEXT_COLOR_BRIGHT_CYAN 		"\x1b[96m"
#define TEXT_COLOR_BRIGHT_WHITE 	"\x1b[97m"

//enum LogType : uint8_t {
typedef enum {
    CC_VERBOSE,
    CC_INFO,
    CC_IMPORTANT,
    CC_WARNING,
    CC_ERROR
} LogType;

static inline void log_formatted(const char* prefix, const char* text_color, const char* msg, va_list args) {

    char buffer_format[MAX_LOG_CHARS] = { 0 };
    int ret0 = snprintf(buffer_format, MAX_LOG_CHARS, "%s %s %s \033[0m", text_color, prefix, msg);
    if (ret0 > MAX_LOG_CHARS) {
        puts("Too many characters to write to format buffer");
        return;
    }

    char buffer_text[MAX_LOG_CHARS] = { 0 };
    int ret1 = vsnprintf(buffer_text, MAX_LOG_CHARS, buffer_format, args);
    if (ret1 > MAX_LOG_CHARS) {
        puts("Too many characters to write to text buffer");
        return;
    }

    printf("%s\n", buffer_text);
    fflush(stdout);
}

static inline void CC_LOG(LogType level, const char* msg, ...) {
    va_list args = NULL;
    va_start(args, msg);

    switch (level) {
        case CC_VERBOSE:   log_formatted("Log:     ", TEXT_COLOR_WHITE,        msg, args); break;
        case CC_INFO:      log_formatted("Log:     ", TEXT_COLOR_BRIGHT_WHITE, msg, args); break;
        case CC_IMPORTANT: log_formatted("Log:     ", TEXT_COLOR_GREEN,        msg, args); break;
        case CC_WARNING:   log_formatted("WARNING: ", TEXT_COLOR_YELLOW,       msg, args); break;
        case CC_ERROR:     log_formatted("ERROR:   ", TEXT_COLOR_RED,          msg, args); break;
    }

    va_end(args);
}

static inline void CC_PRINT(LogType level, const char* msg, ...) {
    va_list args = NULL;
    va_start(args, msg);

    switch (level) {
        case CC_VERBOSE:   log_formatted("", TEXT_COLOR_WHITE,  msg, args); break;
        case CC_INFO:      log_formatted("", TEXT_COLOR_WHITE,  msg, args); break;
        case CC_IMPORTANT: log_formatted("", TEXT_COLOR_GREEN,  msg, args); break;
        case CC_WARNING:   log_formatted("", TEXT_COLOR_YELLOW, msg, args); break;
        case CC_ERROR:     log_formatted("", TEXT_COLOR_RED,    msg, args); break;
    }

    va_end(args);
}


static inline void CC_LOG_SYS_ERROR(void) {
    char buffer[MAX_LOG_CHARS];

#ifdef WIN32
    strerror_s(buffer, MAX_LOG_CHARS, errno);
#else
    strerror_r(errno, buffer, MAX_LOG_CHARS);
#endif
    printf("[%d] %s\n", errno, buffer);
}


#define CC_ASSERT(x, msg, ...) { if(!(x)) { CC_LOG_SYS_ERROR(); CC_LOG(CC_ERROR, msg, ##__VA_ARGS__); } assert(x); }
#define CC_EXIT(x, msg, ...) { CC_LOG(CC_ERROR, msg, ##__VA_ARGS__); exit(x); }

static inline void format_size(uint64_t size, char* buffer, uint32_t buffer_size) {
    const char* suffix[] = { "B", "KB", "MB", "GB", "TB" };

    uint64_t magnitude = 1;
    for (int i = 0; i < 5; ++i) {
        if (size / (magnitude * 1024) == 0) {
            snprintf(buffer, buffer_size, "%3.2f%s", (double)size / (double)magnitude, suffix[i]);
            return;
        }
        magnitude *= 1024;
    }
    CC_ASSERT(0, "More than TB of GPU memory?")
}

static inline void format_time(uint64_t size, char* buffer, uint32_t buffer_size) {
    const char* suffix[] = { "ns", "nns", "ms", "s" };

    uint64_t magnitude = 1;
    for (int i = 0; i < 4; ++i) {
        if (size / (magnitude * 1000) == 0) {
            snprintf(buffer, buffer_size, "%9.3f%s", (double)size / (double)magnitude, suffix[i]);
            return;
        }
        magnitude *= 1000;
    }
    CC_ASSERT(0, "TODO format minutes and hours")
}

#endif // CC_LOGGER_H
