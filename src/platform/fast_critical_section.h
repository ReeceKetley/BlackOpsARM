#pragma once
#include <cstddef>
#include <cstdint>

#if (defined(__ANDROID__) && defined(__aarch64__)) || defined(BO1_FAST_CRIT_SECT_NATIVE_HOST_TEST)
#define BO1_NATIVE_FAST_CRIT_SECT 1
#include <platform/atomic32.h>
#else
#define BO1_NATIVE_FAST_CRIT_SECT 0
#endif

struct FastCriticalSection // sizeof=0x8
{                                                                             // XREF: .data:g_dvarCritSect/r
#if BO1_NATIVE_FAST_CRIT_SECT
        volatile std::uint32_t readCount;
        volatile std::uint32_t writeCount;
#else
        volatile long readCount;
        volatile long writeCount;                        // XREF: Dvar_FindMalleableVar_0:loc_7BD2E2/r
#endif
};
static_assert(sizeof(FastCriticalSection) == 8 && alignof(FastCriticalSection) == 4);
static_assert(offsetof(FastCriticalSection, readCount) == 0 && offsetof(FastCriticalSection, writeCount) == 4);

void __cdecl Sys_LockWrite(FastCriticalSection *critSect);
void __cdecl Sys_UnlockWrite(FastCriticalSection *critSect);
void __cdecl Sys_UnlockRead(FastCriticalSection *critSect);

#if BO1_NATIVE_FAST_CRIT_SECT
namespace bo1::fcs {
// Live, writable, aligned engine counters only. All native concurrent counter
// accesses use the existing fixed32 full-barrier primitives, including polls.
inline std::uint32_t LoadRead(const FastCriticalSection *s) { return atomic32::AtomicLoad32(&s->readCount); }
inline std::uint32_t LoadWrite(const FastCriticalSection *s) { return atomic32::AtomicLoad32(&s->writeCount); }
inline void AddRead(FastCriticalSection *s) { atomic32::ExchangeAdd32(&s->readCount, 1u); }
inline std::uint32_t IncrementWrite(FastCriticalSection *s) { return atomic32::Increment32(&s->writeCount); }
inline void SubtractRead(FastCriticalSection *s) { atomic32::ExchangeAdd32(&s->readCount, UINT32_MAX); }
inline void SubtractWrite(FastCriticalSection *s) { atomic32::ExchangeAdd32(&s->writeCount, UINT32_MAX); }
inline bool SignedPositive(std::uint32_t value) { return value != 0 && value <= INT32_MAX; }
void YieldThread();
}
#endif
