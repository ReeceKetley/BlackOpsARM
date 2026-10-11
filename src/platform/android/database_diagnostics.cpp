#include "database_diagnostics.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <mutex>
#include <unwind.h>
#if !defined(__ANDROID__)
#error Native Android database diagnostics only
#endif
namespace {
std::mutex logMutex;
struct Stack { std::uintptr_t frames[32]{}; unsigned count{}; };
_Unwind_Reason_Code capture(_Unwind_Context *context, void *arg)
{
    auto &stack = *static_cast<Stack *>(arg);
    if (stack.count == 32) return _URC_END_OF_STACK;
    stack.frames[stack.count++] = _Unwind_GetIP(context); return _URC_NO_REASON;
}
}
void bo1::android::DatabaseDiag(const char *format, ...)
{
    std::lock_guard<std::mutex> lock(logMutex);
    const char *path = std::getenv("BO1_DIAG_LOG");
    FILE *file = path && *path ? std::fopen(path, "a") : nullptr;
    if (path && *path && !file) std::fprintf(stderr, "Cannot open database diagnostic log: %s\n", path);
    FILE *sink = file ? file : stderr;
    va_list args; va_start(args, format); std::vfprintf(sink, format, args); va_end(args);
    std::fputc('\n', sink); if (file) std::fclose(file);
}
void bo1::android::DatabaseDiagStack(const char *tag)
{
    Stack stack; _Unwind_Backtrace(capture, &stack);
    char line[1024]; int used = std::snprintf(line, sizeof(line), "STACK %s:", tag ? tag : "");
    for (unsigned i = 0; i < stack.count && used >= 0 && used < 980; ++i)
        used += std::snprintf(line + used, sizeof(line) - static_cast<unsigned>(used), " %llx", static_cast<unsigned long long>(stack.frames[i]));
    DatabaseDiag("%s", line);
}
