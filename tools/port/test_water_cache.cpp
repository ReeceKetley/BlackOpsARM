#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static void require(bool value, const char *why) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}
#define iassert(x) require(!!(x), #x)
bool Assert_MyHandler(const char *, int, int, const char *, ...) {
    require(false, "production assertion"); return true;
}
constexpr int TRACK_WATERSIM = 0;
std::vector<void *> allocations;
void *_PMem_Alloc(unsigned size, unsigned alignment, unsigned pool, unsigned location,
                 int track, const char *, int) {
    require(alignment == 16 && pool == 4 && location == 7 && track == TRACK_WATERSIM,
        "production allocation parameters");
    static uintptr_t address = sizeof(void *) == 8 ? uintptr_t(0x180000000ull) : 0;
    size_t rounded = (size + 4095) & ~size_t(4095);
    auto *base = static_cast<unsigned char *>(VirtualAlloc(reinterpret_cast<void *>(address),
        rounded + 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(base != nullptr, "guarded high allocation");
    if (sizeof(void *) == 8) {
        require(uintptr_t(base) > 0x100000000ull, "channel above4GB"); address += 0x200000;
    }
    DWORD previous;
    require(VirtualProtect(base, 4096, PAGE_NOACCESS, &previous) != 0, "front guard");
    require(VirtualProtect(base + 4096 + rounded, 4096, PAGE_NOACCESS, &previous) != 0, "end guard");
    allocations.push_back(base);
    return base + 4096 + rounded - size;
}
struct alignas(16) F4 { uint32_t u[4]; };
struct Color { uint32_t packed; };
// PRODUCTION_CHANNEL

template<class T> void check(const char *name, size_t cacheSize, size_t yOffset, size_t channelSize) {
    using Channel = channel_t<T>;
    using Cache = typename Channel::cache_t;
    require(offsetof(Cache, data) == 16, "retail SIMD cache data offset");
    require(offsetof(Cache, y) == yOffset && sizeof(Cache) == cacheSize, "retail cache metadata/stride");
    require(offsetof(Channel, cache) == 16 && sizeof(Channel) == channelSize, "native channel header/stride");
    Channel channel;
    channel.Alloc(7);
    require(channel.bufferSize == sizeof(T) * 65536 + 128, "full channel allocation with padding");
    require(uintptr_t(channel.v) % 16 == 0, "allocated row alignment");
    std::vector<T> expected(65536);
    for (unsigned i = 0; i < 65536; ++i) {
        for (size_t b = 0; b < sizeof(T); ++b)
            reinterpret_cast<unsigned char *>(&expected[i])[b] = static_cast<unsigned char>(i * 31 + b * 7);
    }
    memcpy(channel.v, expected.data(), expected.size() * sizeof(T));
    memset(reinterpret_cast<unsigned char *>(channel.v) + expected.size() * sizeof(T), 0xcd, 128);
    for (unsigned y : {0u, 1u, 255u, 17u, 64u, 128u}) {
        T *row = channel.Cache(y, false, false);
        require(uintptr_t(row) % 16 == 0, "cached row SIMD alignment");
        require(memcmp(row, &expected[y * 256], sizeof(T) * 256) == 0, "row contents including last row");
        require(row == channel.Cache(y, false, false), "cache hit preserves pointer");
        auto *cache = reinterpret_cast<Cache *>(reinterpret_cast<unsigned char *>(row) - offsetof(Cache, data));
        const auto *leftHalo = reinterpret_cast<const unsigned char *>(cache)
                             + offsetof(Cache, data) - sizeof(T);
        require(memcmp(leftHalo, &row[255], sizeof(T)) == 0, "left wrapping halo immediately before aligned row");
        require(memcmp(&cache->pad1, &row[0], sizeof(T)) == 0, "right wrapping halo");
        require(cache->y == static_cast<int>(y) && !cache->modified, "read-only row metadata");
    }
    // More than four rows forces dirty writeback and repeated round-robin reuse.
    uint32_t random = 0x803517ab;
    for (unsigned operation = 0; operation < 4000; ++operation) {
        random = random * 1664525u + 1013904223u;
        unsigned y = (random >> 16) & 255, x = random & 255;
        T *row = channel.Cache(y, true, false);
        require(memcmp(row, &expected[y * 256], sizeof(T) * 256) == 0, "read current dirty/evicted row");
        for (size_t b = 0; b < sizeof(T); ++b)
            reinterpret_cast<unsigned char *>(&row[x])[b] = static_cast<unsigned char>(random >> (b % 4) * 8);
        expected[y * 256 + x] = row[x];
        require(channel.Cache(y, false, false) == row, "dirty hit remains cached");
    }
    for (unsigned i = 0; i < 4; ++i) require(channel.Cache(0, false, true) == nullptr, "flush-only return");
    require(memcmp(channel.v, expected.data(), sizeof(T) * expected.size()) == 0, "all dirty data written back");
    for (const auto &cache : channel.cache) require(cache.y == -1 && !cache.modified, "all caches invalidated");
    for (size_t i = sizeof(T) * expected.size(); i < channel.bufferSize; ++i)
        require(reinterpret_cast<unsigned char *>(channel.v)[i] == 0xcd, "channel padding untouched");
    channel.Free(); require(channel.v == nullptr, "channel free clears pointer");
    printf("PASS %s: cache data16 metadata%zu stride%zu channel%zu;4000 dirty operations\n",
        name, yOffset, cacheSize, channelSize);
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    check<F4>("float4", 0x1030, 0x1020, 0x40e0);
    check<short>("short", 0x220, 0x214, 0x8a0);
    check<char>("char", 0x120, 0x114, 0x4a0);
    check<Color>("color", 0x420, 0x414, 0x10a0);
    for (void *allocation : allocations) VirtualFree(allocation, 0, MEM_RELEASE);
    printf("PASS: complete production channel cache, pointer width%zu\n", sizeof(void *));
}
