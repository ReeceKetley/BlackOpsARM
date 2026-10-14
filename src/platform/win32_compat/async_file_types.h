#pragma once
#if defined(__ANDROID__)
#include <cstddef>
#include <cstdint>
struct _OVERLAPPED {
    std::uintptr_t Internal;
    std::uintptr_t InternalHigh;
    union {
        struct { std::uint32_t Offset; std::uint32_t OffsetHigh; };
        void *Pointer;
    };
    void *hEvent;
};
using OVERLAPPED = _OVERLAPPED;
using LPOVERLAPPED = OVERLAPPED *;
static_assert(sizeof(OVERLAPPED) == (sizeof(void *) == 8 ? 32 : 20));
static_assert(offsetof(OVERLAPPED, InternalHigh) == sizeof(void *));
static_assert(offsetof(OVERLAPPED, Offset) == 2 * sizeof(void *));
static_assert(offsetof(OVERLAPPED, hEvent) == (sizeof(void *) == 8 ? 24 : 16));
#endif
