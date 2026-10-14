#pragma once
#include <cstdarg>
#include <cstddef>

namespace bo1::crt {
// Preserve the original MSVC underscore formatter contract: write count bytes
// without a terminator when output fills the buffer, -1 when output exceeds it.
int Vsnprintf(char *buffer, std::size_t count, const char *format, va_list arguments);
int Snprintf(char *buffer, std::size_t count, const char *format, ...);
long long Atoi64(const char *text);
int Stricmp(const char *first, const char *second);
char *Strdup(const char *text);
char *Itoa(int value, char *buffer, int radix);
int Putenv(const char *assignment);
// Independent heap-owned ctime text for engine callers that free the result.
char *OwnedCtime64(long long seconds);
}
#if defined(__ANDROID__)
#define _vsnprintf bo1::crt::Vsnprintf
#define _snprintf bo1::crt::Snprintf
#define _atoi64 bo1::crt::Atoi64
#define _stricmp bo1::crt::Stricmp
#define _strdup bo1::crt::Strdup
#define _itoa bo1::crt::Itoa
#define _putenv bo1::crt::Putenv
#endif
