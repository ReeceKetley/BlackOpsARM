#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <vector>
#include <algorithm>
#include <new>

static void require(bool condition, const char *reason) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", reason); std::exit(1); }
}
bool Assert_MyHandler(const char *file, int line, int, const char *format, ...) {
    std::fprintf(stderr, "Production assertion: %s:%d %s\n", file, line, format);
    std::exit(2);
}
struct GlassesClient {
    static void *Allocate(size_t size, const char *, int) { return std::malloc(size); }
    static void Free(char *ptr) { std::free(ptr); }
};
// PRODUCTION_ALLOCATOR

struct Live { unsigned int *ptr = nullptr; size_t bytes = 0; unsigned char fill = 0; };
static void checkLists(Allocator &a, const std::vector<Live> &live) {
    auto previous = a.head;
    size_t total = 0, freeBytes = 0;
    unsigned count = 0;
    for (auto p = a.head; p != a.tail; p = p->next) {
        require(++count < 4096, "block chain terminates");
        require(p->next && p->next->prev == p, "bidirectional block links");
        require((uintptr_t)p % 16 == 0, "aligned block header");
        require((char *)p->next > (char *)p, "positive block span");
        require(p == a.head || p->prev == previous, "previous block identity");
        total += (char *)p->next - (char *)p;
        previous = p;
    }
    require(total == size_t((char *)a.tail - (char *)a.head), "block span accounting");
    count = 0;
    Allocator::Memory *prevFree = nullptr;
    for (auto p = a.freeHead; p != a.tail; p = p->nextFree) {
        require(p && ++count < 4096, "free chain terminates at sentinel");
        require(p->prevFree == prevFree, "free previous link");
        require(p->nextFree->prevFree == p, "free next link");
        if (prevFree) require((char *)prevFree < (char *)p, "free address ordering");
        freeBytes += (char *)p->next - (char *)p;
        prevFree = p;
    }
    require(a.GetFree() == int(freeBytes), "free byte accounting");
    for (const auto &x : live) if (x.ptr) {
        const auto p = (const unsigned char *)x.ptr;
        require((uintptr_t)p % 16 == 0, "aligned payload");
        require(p >= (unsigned char *)a.head && p + x.bytes <= (unsigned char *)a.tail,
                "payload within arena");
        require(a.GetMemorySize(x.ptr) >= x.bytes + offsetof(Allocator::Memory, prevFree),
                "native header and requested bytes fit");
        for (size_t i = 0; i < x.bytes; ++i) require(p[i] == x.fill, "live payload retained");
        for (const auto &y : live) if (y.ptr && y.ptr != x.ptr) {
            auto q = (const unsigned char *)y.ptr;
            require(p + x.bytes <= q || q + y.bytes <= p, "live blocks do not overlap");
        }
    }
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    constexpr size_t reservation = 3 * 65536;
    void *requested = sizeof(void *) == 8 ? (void *)uintptr_t(0x180000000) : nullptr;
    auto base = (unsigned char *)VirtualAlloc(requested, reservation, MEM_RESERVE, PAGE_NOACCESS);
    require(base != nullptr, "guarded arena reservation");
    auto arena = (unsigned char *)VirtualAlloc(base + 65536, 65536, MEM_COMMIT, PAGE_READWRITE);
    require(arena != nullptr, "guarded arena commit");
    if (sizeof(void *) == 8) require((uintptr_t)arena > 0x100000000ull, "arena above 4 GB");
    Allocator a{};
    // Unaligned bounds exercise both full-width alignment masks.
    a.Init(arena + 3, 65530);
    int originalFree = a.GetFree();
    require(originalFree > 65000 && (char *)a.tail + sizeof(*a.tail) <= (char *)arena + 65533,
            "native tail header within supplied bounds");
    std::vector<Live> live(64);
    uint32_t random = 0x5a17831;
    for (unsigned n = 0; n < 6000; ++n) {
        random = random * 1664525u + 1013904223u;
        auto &x = live[(random >> 16) % live.size()];
        if (x.ptr) { a.Free(x.ptr); x.ptr = nullptr; }
        else {
            size_t bytes = (random >> 8) % 1536 + 1;
            auto p = (unsigned int *)a.Allocate(int(bytes), (void *)uintptr_t(n + 1));
            if (p) { x = {p, bytes, (unsigned char)(n % 251)}; std::memset(p, x.fill, bytes); }
        }
        checkLists(a, live);
    }
    for (auto &x : live) if (x.ptr) { a.Free(x.ptr); x.ptr = nullptr; }
    checkLists(a, live);
    require(a.GetFree() == originalFree && a.freeHead == a.head && a.head->next == a.tail,
            "all blocks coalesce and arena is reusable");
    auto zero = (unsigned int *)a.Allocate(0, nullptr);
    require(zero != nullptr, "zero byte allocation has a valid native header");
    a.Free(zero);
    require(a.GetFree() == originalFree, "zero allocation round trip");
    require(!a.Allocate(-1, nullptr) && !a.Allocate(INT_MAX, nullptr), "invalid sizes rejected");
    // Exact exhaustion followed by free tests the empty-free-list branch.
    auto all = (unsigned int *)a.Allocate(originalFree - int(offsetof(Allocator::Memory, prevFree)), nullptr);
    require(all && a.freeHead == a.tail && a.GetFree() == 0, "arena exhaustion");
    require(!a.Allocate(64, nullptr), "exhausted arena returns null");
    std::memset(all, 0xa7, originalFree - offsetof(Allocator::Memory, prevFree));
    a.Free(all);
    require(a.GetFree() == originalFree, "exhausted arena recovered");
    a.FreeAll();
    require(a.GetLargestFree() == originalFree, "largest free after reset");
    // Production allocator rebound to real std::list nodes, including sentinels.
    SmallAllocator small{};
    constexpr unsigned blockSize = (sizeof(void *) * 3 + 15) & ~15u;
    small.Init(arena, blockSize, 128);
    require(!small.IsValidPointer(arena - 1) && !small.IsValidPointer(arena + 1)
        && !small.IsValidPointer(arena + blockSize * 128), "small pool bounds and alignment");
    int objects[24]{};
    {
        FixedSizeAllocator<int *> pool(objects, 24, &small);
        std::vector<int *> allocated;
        for (unsigned i = 0; i < 24; ++i) {
            auto p = pool.Allocate(); require(p != nullptr, "fixed pool allocation");
            require(std::find(allocated.begin(), allocated.end(), p) == allocated.end(), "unique objects");
            allocated.push_back(p);
        }
        require(!pool.Allocate(), "fixed pool exhaustion");
        for (auto p : allocated) pool.Free(p);
        require(pool.used.empty() && pool.free.size() == 24, "fixed pool reuse");
        auto p = pool.Allocate(); require(p != nullptr, "fixed pool reallocate");
        pool.FreeAll();
    }
    require(small.numUsed == 0, "std list destruction returns every node");
    VirtualFree(base, 0, MEM_RELEASE);
    std::printf("PASS: glass allocator %zu-bit, arena=%p, 6000 guarded allocations/frees, exhaustion, coalescing, std::list reuse\n", sizeof(void *) * 8, arena);
}
