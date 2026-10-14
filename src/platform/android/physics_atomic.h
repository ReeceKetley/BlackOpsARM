#pragma once
#include <platform/atomic32.h>
#include <cstdint>

namespace bo1::android::physics_atomic {
static_assert(sizeof(unsigned int) == 4);
inline unsigned int load(const volatile unsigned int *value) {
    return bo1::atomic32::AtomicLoad32(value);
}
inline unsigned int compareExchange(volatile unsigned int *value,
                                    unsigned int exchange, unsigned int expected) {
    return bo1::atomic32::CmpExchange32(value, exchange, expected);
}
inline void compilerBarrier() { __atomic_signal_fence(__ATOMIC_SEQ_CST); }
inline void memoryBarrier() { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
}
