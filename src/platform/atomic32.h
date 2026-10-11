#pragma once
#include <cstdint>
#if defined(_WIN32)
#include <Windows.h>
#elif !defined(__ANDROID__) || !defined(__aarch64__)
#error "Atomic32 currently supports Windows and Android ARM64 only."
#endif

// Packed engine counters remain four bytes on both LLP64 and LP64. These
// operations preserve the full-barrier contract of Windows Interlocked APIs.
// Every pointer names live, writable, naturally four-byte-aligned uint32 storage;
// use atomic operations consistently during concurrent access. AtomicLoad32's
// const parameter is a view, not permission to pass immutable storage: Windows
// CompareExchange performs a write-capable operation even for the load. Never
// apply these operations to const objects or mapped disk metadata.
namespace bo1::atomic32 {
static_assert(sizeof(std::uint32_t) == 4 && alignof(std::uint32_t) == 4);
#if defined(_WIN32)
static_assert(sizeof(LONG) == 4);
inline volatile LONG *native(volatile std::uint32_t *p) {
    return reinterpret_cast<volatile LONG *>(p);
}
#endif
inline std::uint32_t AtomicLoad32(const volatile std::uint32_t *p) {
#if defined(_WIN32)
    return static_cast<std::uint32_t>(InterlockedCompareExchange(native(const_cast<volatile std::uint32_t *>(p)), 0, 0));
#else
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
#endif
}
inline std::uint32_t CmpExchange32(volatile std::uint32_t *p, std::uint32_t desired, std::uint32_t expected) {
#if defined(_WIN32)
    return static_cast<std::uint32_t>(InterlockedCompareExchange(native(p), static_cast<LONG>(desired), static_cast<LONG>(expected)));
#else
    __atomic_compare_exchange_n(p, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
#endif
}
inline std::uint32_t ExchangeAdd32(volatile std::uint32_t *p, std::uint32_t amount) {
#if defined(_WIN32)
    return static_cast<std::uint32_t>(InterlockedExchangeAdd(native(p), static_cast<LONG>(amount)));
#else
    return __atomic_fetch_add(p, amount, __ATOMIC_SEQ_CST);
#endif
}
inline std::uint32_t Increment32(volatile std::uint32_t *p) {
#if defined(_WIN32)
    return static_cast<std::uint32_t>(InterlockedIncrement(native(p)));
#else
    return __atomic_add_fetch(p, std::uint32_t(1), __ATOMIC_SEQ_CST);
#endif
}
inline std::uint32_t Decrement32(volatile std::uint32_t *p) {
#if defined(_WIN32)
    return static_cast<std::uint32_t>(InterlockedDecrement(native(p)));
#else
    return __atomic_sub_fetch(p, std::uint32_t(1), __ATOMIC_SEQ_CST);
#endif
}
}
