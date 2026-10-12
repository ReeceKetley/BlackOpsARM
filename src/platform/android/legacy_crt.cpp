#include "legacy_crt.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

int bo1::crt::Vsnprintf(char *buffer, std::size_t count, const char *format, va_list arguments) {
    va_list measure;
    va_copy(measure, arguments);
    const int required = std::vsnprintf(nullptr, 0, format, measure);
    va_end(measure);
    if (required < 0) return -1;
    const std::size_t written = std::min(count, static_cast<std::size_t>(required));
    // The original caller owns exactly count bytes. An extra scratch byte lets
    // POSIX format the final character without overrunning that caller buffer.
    char *scratch = static_cast<char *>(std::malloc(written + 1));
    if (!scratch) { errno = ENOMEM; return -1; }
    va_list render;
    va_copy(render, arguments);
    const int result = std::vsnprintf(scratch, written + 1, format, render);
    va_end(render);
    if (result < 0) { std::free(scratch); return -1; }
    if (written) std::memcpy(buffer, scratch, written);
    if (static_cast<std::size_t>(required) < count) buffer[written] = '\0';
    std::free(scratch);
    return static_cast<std::size_t>(required) > count ? -1 : required;
}
int bo1::crt::Snprintf(char *buffer, std::size_t count, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const int result = Vsnprintf(buffer, count, format, arguments);
    va_end(arguments);
    return result;
}
long long bo1::crt::Atoi64(const char *text) { return std::strtoll(text, nullptr, 10); }
