#include "stack_trace.h"
#include <unwind.h>
#include <cstdint>
#include <cstdio>
#include <cerrno>

namespace bo1::android {
namespace {
struct Capture { char *out; std::size_t capacity, used; unsigned skip, count; };
_Unwind_Reason_Code Frame(_Unwind_Context *frame, void *context) {
    auto &state = *static_cast<Capture *>(context);
    const std::uintptr_t address = _Unwind_GetIP(frame);
    if (!address) return _URC_NO_REASON;
    if (state.skip) { --state.skip; return _URC_NO_REASON; }
    const int size = std::snprintf(state.out + state.used, state.capacity - state.used,
        "  #%u 0x%llx\n", state.count, static_cast<unsigned long long>(address));
    if (size < 0) {
        state.out[state.used] = 0;
        return _URC_END_OF_STACK;
    }
    if (static_cast<std::size_t>(size) >= state.capacity - state.used) {
        state.used = state.capacity - 1;
        return _URC_END_OF_STACK;
    }
    state.used += static_cast<std::size_t>(size);
    return ++state.count >= 32 ? _URC_END_OF_STACK : _URC_NO_REASON;
}
}
std::size_t CaptureStackTrace(char *output, std::size_t capacity, unsigned skipFrames) {
    if (!output || !capacity) { errno = EINVAL; return 0; }
    output[0] = 0;
    Capture state{output, capacity, 0, skipFrames, 0};
    _Unwind_Backtrace(Frame, &state);
    output[capacity - 1] = 0;
    return state.used;
}
}
