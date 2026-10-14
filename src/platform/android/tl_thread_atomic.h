#pragma once
#include <platform/atomic32.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sched.h>
#include <unistd.h>

namespace bo1::android::tl_atomic {
inline unsigned long long threadId() {
    const auto id = gettid();
    if (id <= 0) { std::fputs("Native TL: invalid thread ID\n", stderr); std::abort(); }
    return static_cast<unsigned long long>(id);
}
inline void yield() {
    if (sched_yield()) { std::perror("Native TL sched_yield"); std::abort(); }
}
inline long long compareExchange64(volatile long long *value,
                                   unsigned long long desired, unsigned long long comparand) {
    static_assert(sizeof(long long) == 8);
    auto expected = static_cast<long long>(comparand);
    __atomic_compare_exchange_n(value, &expected, static_cast<long long>(desired),
                                false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}
inline std::int32_t exchangeBarrier(volatile std::int32_t *value, std::int32_t desired) {
    return __atomic_exchange_n(value, desired, __ATOMIC_SEQ_CST);
}
inline void readUnlock(volatile unsigned int *count) {
    const auto previous = bo1::atomic32::ExchangeAdd32(count, ~std::uint32_t(0));
    if (!previous) { std::fputs("Native TL: unbalanced read unlock\n", stderr); std::abort(); }
}
}
