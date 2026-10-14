#include <platform/android/stack_trace.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <climits>
#include <initializer_list>

int main() {
    char text[4096];
    const auto length = bo1::android::CaptureStackTrace(text, sizeof(text), 0);
    if (!length || length >= sizeof(text) || std::strlen(text) != length ||
        !std::strstr(text, "  #0 0x")) return 1;
    for (std::size_t capacity: {std::size_t(1), std::size_t(2), std::size_t(9)}) {
        char bounded[16];
        std::memset(bounded, 'Q', sizeof(bounded));
        const auto used = bo1::android::CaptureStackTrace(bounded, capacity, 0);
        if (used >= capacity || std::strlen(bounded) != used || bounded[capacity] != 'Q') return 2;
    }
    errno = 0;
    if (bo1::android::CaptureStackTrace(nullptr, 20, 0) || errno != EINVAL) return 3;
    text[0] = 'Q';
    errno = 0;
    if (bo1::android::CaptureStackTrace(text, 0, 0) || errno != EINVAL || text[0] != 'Q') return 4;
    if (bo1::android::CaptureStackTrace(text, sizeof(text), UINT_MAX) || text[0]) return 5;
    std::puts("actual native stack capture: frames/length/bounds/invalid-output/skip passed");
}
