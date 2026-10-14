#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unwind.h>
#include <cstdint>

namespace {
struct FatalTrace { unsigned count; };
_Unwind_Reason_Code TraceFrame(_Unwind_Context *frame, void *context) {
    auto &trace = *static_cast<FatalTrace *>(context);
    const std::uintptr_t address = _Unwind_GetIP(frame);
    if (address) {
        std::fprintf(stderr, "  #%u 0x%llx\n", trace.count,
            static_cast<unsigned long long>(address));
        ++trace.count;
    }
    return trace.count >= 32 ? _URC_END_OF_STACK : _URC_NO_REASON;
}
}

void Sys_Error(char *format, ...) {
    char message[4096]{};
    if (format) {
        va_list args;
        va_start(args, format);
        const int count = std::vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        if (count < 0) std::snprintf(message, sizeof(message), "Fatal error message formatting failed");
    } else {
        std::snprintf(message, sizeof(message), "Fatal error without a message format");
    }
    message[sizeof(message) - 1] = 0;
    std::fprintf(stderr, "Native engine fatal error: %s\n", message);
    std::fputs("Native fatal stack (program counters):\n", stderr);
    FatalTrace trace{};
    _Unwind_Backtrace(TraceFrame, &trace);
    if (!trace.count) std::fputs("  No unwind frames available\n", stderr);
    std::fflush(stderr);
    // Like the reference headless fatal path, end every thread immediately.
    // This is fatal termination, not a successful engine cleanup or recovery.
    std::_Exit(2);
}
