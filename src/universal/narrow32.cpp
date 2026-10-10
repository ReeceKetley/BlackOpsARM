#include "narrow32.h"

#if defined(BO1_PORT_X64)
#include <Windows.h>

void Bo1_Diag(const char *fmt, ...);
void Bo1_DiagStack(const char *tag);

// A pointer could not be narrowed: with BO1_NARROW_HANDLES = 0 it is above 4 GB, with handles on the chunk table is full.
void Bo1_NarrowFail(const void *p)
{
    Bo1_Diag("Bo1_NarrowFail: cannot narrow pointer %p", p);
    Bo1_DiagStack("NARROW");
    __debugbreak();
}

#if BO1_NARROW_HANDLES
uintptr_t g_bo1NarrowBase[256];
volatile uint64_t g_bo1NarrowKey[2048];
volatile uint8_t g_bo1NarrowIdx[2048];
static unsigned int s_narrowCount;
static SRWLOCK s_narrowLock = SRWLOCK_INIT;

unsigned int Bo1_NarrowEncodeSlow(const void *p)
{
    const uintptr_t a = reinterpret_cast<uintptr_t>(p);
    const uint64_t chunk = static_cast<uint64_t>(a >> 24) + 1;
    AcquireSRWLockExclusive(&s_narrowLock);
    unsigned int i = static_cast<unsigned int>((chunk * 0x9E3779B97F4A7C15ull) >> 53);
    for (unsigned int n = 0; n < 2048; ++n, i = (i + 1) & 2047)
    {
        const uint64_t k = g_bo1NarrowKey[i];
        if (k == chunk)
        {
            ReleaseSRWLockExclusive(&s_narrowLock);
            return (static_cast<unsigned int>(g_bo1NarrowIdx[i]) << 24) | static_cast<unsigned int>(a & 0xFFFFFF);
        }
        if (!k)
        {
            if (s_narrowCount >= 255)
            {
                ReleaseSRWLockExclusive(&s_narrowLock);
                Bo1_NarrowFail(p);
                return 0;
            }
            const unsigned int idx = ++s_narrowCount;
            g_bo1NarrowBase[idx] = static_cast<uintptr_t>(chunk - 1) << 24;
            g_bo1NarrowIdx[i] = static_cast<uint8_t>(idx);
            MemoryBarrier();
            g_bo1NarrowKey[i] = chunk;
            ReleaseSRWLockExclusive(&s_narrowLock);
            return (idx << 24) | static_cast<unsigned int>(a & 0xFFFFFF);
        }
    }
    ReleaseSRWLockExclusive(&s_narrowLock);
    Bo1_NarrowFail(p);
    return 0;
}
#endif
#endif
