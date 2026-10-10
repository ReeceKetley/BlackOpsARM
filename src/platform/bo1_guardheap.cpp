// Opt-in guard-page heap for x64 bring-up (CMake option BO1_X64_GUARDHEAP). Replaces the global operator new/delete and,
// through the force-included bo1_guardheap_fi.h, the engine's malloc/calloc/realloc/free:
//   * every block ends flush against a PAGE_NOACCESS page, so a heap overflow faults at the offending instruction;
//   * freed blocks are made PAGE_NOACCESS and quarantined, so a use-after-free also faults at the offending instruction.
// A plain Windows heap only notices the damage later, in an unrelated free (0xC0000374).
#if defined(BO1_X64_GUARDHEAP)
#include <windows.h>
#include <malloc.h>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>

namespace
{
constexpr size_t kPage = 4096;
constexpr size_t kQuarantine = 4096;
constexpr size_t kTableSize = 1u << 21; // live blocks

struct Entry
{
    uintptr_t ptr; // user pointer (0 = empty, 1 = tombstone)
    uintptr_t base;
    size_t size;
    size_t pages;
};

Entry *g_table;
CRITICAL_SECTION g_lock;
bool g_init;
size_t g_liveBytes;
struct Quar
{
    uintptr_t base;
    size_t pages;
};
Quar g_quar[kQuarantine];
size_t g_quarHead, g_quarCount;
size_t g_fallbacks;

void Init()
{
    if (g_init)
        return;
    InitializeCriticalSection(&g_lock);
    g_table = static_cast<Entry *>(VirtualAlloc(nullptr, kTableSize * sizeof(Entry), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    g_init = true;
}

size_t Hash(uintptr_t p) { return static_cast<size_t>((p >> 4) * 0x9E3779B97F4A7C15ull >> 40) & (kTableSize - 1); }

Entry *Find(uintptr_t p, bool forInsert)
{
    size_t i = Hash(p);
    Entry *tomb = nullptr;
    for (size_t n = 0; n < kTableSize; ++n, i = (i + 1) & (kTableSize - 1))
    {
        Entry &e = g_table[i];
        if (e.ptr == p)
            return &e;
        if (e.ptr == 1 && !tomb)
            tomb = &e;
        if (e.ptr == 0)
            return forInsert ? (tomb ? tomb : &e) : nullptr;
    }
    return forInsert ? tomb : nullptr;
}

void ReleaseOldest()
{
    const Quar q = g_quar[g_quarHead];
    g_quarHead = (g_quarHead + 1) % kQuarantine;
    --g_quarCount;
    VirtualFree(reinterpret_cast<void *>(q.base), 0, MEM_RELEASE);
}

void *GuardAlloc(size_t size, size_t align)
{
    Init();
    if (size == 0)
        size = 1;
    const size_t dataPages = (size + kPage - 1) / kPage;
    uint8_t *base = static_cast<uint8_t *>(VirtualAlloc(nullptr, (dataPages + 1) * kPage, MEM_RESERVE, PAGE_NOACCESS));
    if (!base || !VirtualAlloc(base, dataPages * kPage, MEM_COMMIT, PAGE_READWRITE))
    {
        if (base)
            VirtualFree(base, 0, MEM_RELEASE);
        return nullptr;
    }
    if (align < 16)
        align = 16;
    uintptr_t user = (reinterpret_cast<uintptr_t>(base) + dataPages * kPage - size) & ~(align - 1);
    EnterCriticalSection(&g_lock);
    Entry *e = Find(user, true);
    if (!e)
    {
        LeaveCriticalSection(&g_lock);
        VirtualFree(base, 0, MEM_RELEASE);
        return nullptr;
    }
    e->ptr = user;
    e->base = reinterpret_cast<uintptr_t>(base);
    e->size = size;
    e->pages = dataPages + 1;
    g_liveBytes += (dataPages + 1) * kPage;
    LeaveCriticalSection(&g_lock);
    return reinterpret_cast<void *>(user);
}

bool GuardFree(void *p)
{
    if (!g_init || !p)
        return false;
    EnterCriticalSection(&g_lock);
    Entry *e = Find(reinterpret_cast<uintptr_t>(p), false);
    if (!e)
    {
        LeaveCriticalSection(&g_lock);
        return false; // not ours (allocated by the CRT itself)
    }
    const uintptr_t base = e->base;
    const size_t pages = e->pages;
    e->ptr = 1;
    DWORD old;
    VirtualProtect(reinterpret_cast<void *>(base), (pages - 1) * kPage, PAGE_NOACCESS, &old);
    if (g_quarCount == kQuarantine)
        ReleaseOldest();
    g_quar[(g_quarHead + g_quarCount) % kQuarantine] = Quar{base, pages};
    ++g_quarCount;
    g_liveBytes -= pages * kPage;
    LeaveCriticalSection(&g_lock);
    return true;
}

size_t GuardSize(void *p)
{
    if (!g_init || !p)
        return 0;
    EnterCriticalSection(&g_lock);
    Entry *e = Find(reinterpret_cast<uintptr_t>(p), false);
    const size_t s = e ? e->size : 0;
    LeaveCriticalSection(&g_lock);
    return s;
}
} // namespace

extern "C" {
// CRT-internal allocations (fopen buffers, ...) arrive here too; blocks that are not ours are handed to the CRT's own *_base functions.
void *__cdecl malloc(size_t n)
{
    void *p = GuardAlloc(n, 16);
    return p ? p : (++g_fallbacks, _malloc_base(n));
}
void *__cdecl calloc(size_t a, size_t b)
{
    const size_t n = a * b;
    void *p = GuardAlloc(n, 16);
    if (p)
    {
        memset(p, 0, n);
        return p;
    }
    ++g_fallbacks;
    return _calloc_base(a, b);
}
void __cdecl free(void *p)
{
    if (!p)
        return;
    if (!GuardFree(p))
        _free_base(p);
}
void *__cdecl realloc(void *p, size_t n)
{
    if (!p)
        return malloc(n);
    const size_t old = GuardSize(p);
    if (!old)
        return _realloc_base(p, n); // CRT-owned block
    void *q = malloc(n);
    if (q)
        memcpy(q, p, old < n ? old : n);
    free(p);
    return q;
}
size_t __cdecl _msize(void *p)
{
    const size_t s = GuardSize(p);
    return s ? s : HeapSize(GetProcessHeap(), 0, p); // foreign (UCRT-owned) block: the UCRT allocates from the process heap
}
}

void *operator new(size_t n)
{
    void *p = malloc(n);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void *operator new[](size_t n) { return operator new(n); }
void *operator new(size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void *operator new[](size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { free(p); }
#endif
