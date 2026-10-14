#include "legacy_crt.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <ctime>

char *bo1::crt::OwnedCtime64(long long seconds) {
    static_assert(sizeof(std::time_t) == 8);
    const std::time_t value = static_cast<std::time_t>(seconds);
    std::tm local{};
    if (!::localtime_r(&value, &local)) return nullptr;
    if (local.tm_year < -1900 || local.tm_year > 8099) { errno = EOVERFLOW; return nullptr; }
    char text[26];
    if (!::asctime_r(&local, text)) return nullptr;
    char *owned = static_cast<char *>(std::malloc(sizeof(text)));
    if (!owned) { errno = ENOMEM; return nullptr; }
    std::memcpy(owned, text, sizeof(text));
    return owned;
}

int bo1::crt::Putenv(const char *assignment) {
    if (!assignment) { errno = EINVAL; return -1; }
    const char *separator = std::strchr(assignment, '=');
    if (!separator || separator == assignment) { errno = EINVAL; return -1; }
    const std::size_t length = static_cast<std::size_t>(separator - assignment);
    char *name = static_cast<char *>(std::malloc(length + 1));
    if (!name) { errno = ENOMEM; return -1; }
    std::memcpy(name, assignment, length);
    name[length] = 0;
    // MSVC copies the assignment and NAME= removes a variable. POSIX putenv
    // would instead retain the caller's buffer, which can be stack storage.
    const int result = separator[1] ? ::setenv(name, separator + 1, 1) : ::unsetenv(name);
    const int savedError = errno;
    std::free(name);
    errno = savedError;
    return result;
}

char *bo1::crt::Strdup(const char *text) {
    if (!text) { std::fputs("Native CRT _strdup: null input\n", stderr); std::abort(); }
    return ::strdup(text);
}
char *bo1::crt::Itoa(int value, char *buffer, int radix) {
    if (!buffer || radix < 2 || radix > 36) {
        std::fputs("Native CRT _itoa: invalid argument\n", stderr); std::abort();
    }
    char reversed[33];
    unsigned count = 0;
    const bool negative = radix == 10 && value < 0;
    std::uint32_t magnitude = static_cast<std::uint32_t>(value);
    if (negative) magnitude = 0u - magnitude;
    do {
        reversed[count++] = "0123456789abcdefghijklmnopqrstuvwxyz"[magnitude % radix];
        magnitude /= radix;
    } while (magnitude);
    char *out = buffer;
    if (negative) *out++ = '-';
    while (count) *out++ = reversed[--count];
    *out = 0;
    return buffer;
}

int bo1::crt::Stricmp(const char *first, const char *second) {
    if (!first || !second) {
        std::fputs("Native CRT _stricmp: null input\n", stderr);
        std::abort();
    }
    const auto *a = reinterpret_cast<const unsigned char *>(first);
    const auto *b = reinterpret_cast<const unsigned char *>(second);
    int left, right;
    do {
        left = std::tolower(*a++);
        right = std::tolower(*b++);
    } while (left && left == right);
    return left - right;
}

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
