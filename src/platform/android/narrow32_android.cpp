#include <universal/narrow32.h>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#if !defined(__ANDROID__) || !defined(__aarch64__) || !BO1_NATIVE_64BIT || !BO1_NARROW_HANDLES
#error This implementation requires Android ARM64 region-tagged handles
#endif

std::atomic<uintptr_t> g_bo1NarrowBase[256]{};
std::atomic<uint64_t> g_bo1NarrowKey[2048]{};
std::atomic<uint8_t> g_bo1NarrowIdx[2048]{};
static unsigned int s_narrowCount;
static std::mutex s_narrowLock;

void Bo1_NarrowFail(const void *p)
{
    std::fprintf(stderr, "Bo1_NarrowFail: region table exhausted for %p\n", p);
    std::abort();
}

unsigned int Bo1_NarrowEncodeSlow(const void *p)
{
    const uintptr_t a = reinterpret_cast<uintptr_t>(p);
    const uint64_t chunk = static_cast<uint64_t>(a >> 24) + 1;
    std::lock_guard<std::mutex> lock(s_narrowLock);
    unsigned int i = static_cast<unsigned int>((chunk * 0x9E3779B97F4A7C15ull) >> 53);
    for (unsigned int n = 0; n < 2048; ++n, i = (i + 1) & 2047)
    {
        const uint64_t k = g_bo1NarrowKey[i].load(std::memory_order_acquire);
        if (k == chunk)
            return (static_cast<unsigned int>(g_bo1NarrowIdx[i].load(std::memory_order_relaxed)) << 24)
                | static_cast<unsigned int>(a & 0xFFFFFF);
        if (!k)
        {
            if (s_narrowCount >= 255)
                Bo1_NarrowFail(p);
            const unsigned int idx = ++s_narrowCount;
            g_bo1NarrowBase[idx].store(static_cast<uintptr_t>(chunk - 1) << 24, std::memory_order_release);
            g_bo1NarrowIdx[i].store(static_cast<uint8_t>(idx), std::memory_order_relaxed);
            // Publish only after base/index are ready. Fast-path readers acquire
            // this key; decoding imported raw slots acquires the base directly.
            g_bo1NarrowKey[i].store(chunk, std::memory_order_release);
            return (idx << 24) | static_cast<unsigned int>(a & 0xFFFFFF);
        }
    }
    Bo1_NarrowFail(p);
    std::abort();
}
