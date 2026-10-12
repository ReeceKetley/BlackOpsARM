#pragma once
#include <cstdarg>
#include <cstddef>

namespace bo1::crt {
// Preserve the original MSVC underscore formatter contract: write count bytes
// without a terminator when output fills the buffer, -1 when output exceeds it.
int Vsnprintf(char *buffer, std::size_t count, const char *format, va_list arguments);
int Snprintf(char *buffer, std::size_t count, const char *format, ...);
long long Atoi64(const char *text);
}
#if defined(__ANDROID__)
#define _vsnprintf bo1::crt::Vsnprintf
#define _snprintf bo1::crt::Snprintf
#define _atoi64 bo1::crt::Atoi64
#endif
