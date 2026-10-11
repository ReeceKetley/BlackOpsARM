#pragma once

#include <cstddef>
#include <climits>

// Config fields address native objects. Keep the retail Win32 offset as a
// compile-time check while deriving the active offset from the member itself.
template <size_t NativeOffset, size_t Win32Offset>
constexpr int CspFieldOffset()
{
    static_assert(sizeof(void *) != 4 || NativeOffset == Win32Offset,
        "Config field no longer matches its retail Win32 member");
    static_assert(NativeOffset <= INT_MAX, "Config field offset exceeds int");
    return static_cast<int>(NativeOffset);
}

#define CSP_FIELD_OFFSET(type, member, win32Offset) \
    CspFieldOffset<offsetof(type, member), win32Offset>()
