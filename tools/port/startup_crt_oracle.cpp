// Validation-only: obtain actual MSVC legacy CRT truncation bytes and returns.
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cfloat>
#if defined(BO1_STARTUP_CRT_NATIVE_ORACLE)
#include "platform/android/legacy_crt.h"
#if !defined(__ANDROID__)
#define _snprintf bo1::crt::Snprintf
#define _vsnprintf bo1::crt::Vsnprintf
#define _atoi64 bo1::crt::Atoi64
#endif
#endif

static int indirect(char *out, size_t capacity, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = _vsnprintf(out, capacity, format, args);
    va_end(args);
    return result;
}

int main() {
    const char *inputs[] = {"", "a", "abcd", "abcdefghijk"};
    for (unsigned input = 0; input != 4; ++input) {
        for (unsigned capacity = 0; capacity != 14; ++capacity) {
            for (unsigned mode = 0; mode != 2; ++mode) {
                unsigned char bytes[24];
                std::memset(bytes, 0xA5, sizeof(bytes));
                char *out = reinterpret_cast<char *>(bytes + 4);
                int result = mode ? indirect(out, capacity, "%s", inputs[input])
                                  : _snprintf(out, capacity, "%s", inputs[input]);
                std::printf("{\"input\":%u,\"capacity\":%u,\"mode\":%u,\"result\":%d,\"bytes\":\"",
                            input, capacity, mode, result);
                for (unsigned char value : bytes) std::printf("%02x", value);
                std::puts("\"}");
                const size_t length = std::strlen(inputs[input]);
                const int expected = length > capacity ? -1 : static_cast<int>(length);
                if (result != expected) return 1;
                for (unsigned offset = 0; offset != sizeof(bytes); ++offset) {
                    unsigned char expectedByte = 0xA5;
                    if (offset >= 4 && offset < 4 + capacity) {
                        const size_t index = offset - 4;
                        if (index < length) expectedByte = inputs[input][index];
                        else if (index == length) expectedByte = 0;
                    }
                    if (bytes[offset] != expectedByte) return 2;
                }
            }
        }
    }
    const char *mixed = "-7|9223372036854775807|1.25|-2.5|3.75|text";
    for (unsigned capacity = 0; capacity != 49; ++capacity) {
        for (unsigned mode = 0; mode != 2; ++mode) {
            unsigned char bytes[64];
            std::memset(bytes, 0xA5, sizeof(bytes));
            char *out = reinterpret_cast<char *>(bytes + 4);
            int result = mode ? indirect(out, capacity, "%i|%lli|%g|%g|%g|%s",
                                        -7, 9223372036854775807LL, 1.25, -2.5, 3.75, "text")
                              : _snprintf(out, capacity, "%i|%lli|%g|%g|%g|%s",
                                          -7, 9223372036854775807LL, 1.25, -2.5, 3.75, "text");
            std::printf("{\"mixed_capacity\":%u,\"mode\":%u,\"result\":%d,\"bytes\":\"", capacity, mode, result);
            for (unsigned char value : bytes) std::printf("%02x", value);
            std::puts("\"}");
            const size_t length = std::strlen(mixed);
            if (result != (length > capacity ? -1 : static_cast<int>(length))) return 5;
            for (unsigned offset = 0; offset != sizeof(bytes); ++offset) {
                unsigned char expectedByte = 0xA5;
                if (offset >= 4 && offset < 4 + capacity) {
                    const size_t index = offset - 4;
                    if (index < length) expectedByte = mixed[index];
                    else if (index == length) expectedByte = 0;
                }
                if (bytes[offset] != expectedByte) return 6;
            }
        }
    }
    const double floats[] = {static_cast<double>(FLT_MAX), -static_cast<double>(FLT_MAX),
                             static_cast<double>(FLT_MIN), 1e-9, 1e10, -0.0};
    for (unsigned index = 0; index != 6; ++index) {
        for (unsigned mode = 0; mode != 2; ++mode) {
            unsigned char bytes[64];
            std::memset(bytes, 0xA5, sizeof(bytes));
            char *out = reinterpret_cast<char *>(bytes + 4);
            const int result = mode ? indirect(out, 48, "%g", floats[index])
                                    : _snprintf(out, 48, "%g", floats[index]);
            if (result < 0 || result >= 48 || out[result] != 0) return 7;
            for (unsigned offset = 0; offset != 4; ++offset) if (bytes[offset] != 0xA5) return 8;
            for (unsigned offset = 4 + result + 1; offset != sizeof(bytes); ++offset) if (bytes[offset] != 0xA5) return 9;
            std::printf("{\"float_case\":%u,\"mode\":%u,\"result\":%d,\"bytes\":\"", index, mode, result);
            for (unsigned char value : bytes) std::printf("%02x", value);
            std::puts("\"}");
        }
    }
    const char *integers[] = {"0", "-1", "  +42suffix", "not-a-number",
                             "9223372036854775807", "-9223372036854775808",
                             "9223372036854775808", "-9223372036854775809"};
    const __int64 expected[] = {0, -1, 42, 0, 9223372036854775807LL,
                               (-9223372036854775807LL - 1), 9223372036854775807LL,
                               (-9223372036854775807LL - 1)};
    for (unsigned index = 0; index != 8; ++index) {
        errno = 0;
        const __int64 value = _atoi64(integers[index]);
        std::printf("{\"integer_case\":%u,\"value\":%lld,\"range_error\":%s}\n", index, value,
                    errno == ERANGE ? "true" : "false");
        if (value != expected[index]) return 3;
        if ((errno == ERANGE) != (index >= 6)) return 4;
    }
    return 0;
}
