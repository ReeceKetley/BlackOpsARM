// Standalone production PMem ownership regression; no game/application stubs.
#include <platform/bo1_native_width.h>
#include <universal/physicalmemory.h>
#include <Windows.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern bool g_physicalMemoryInit;
static void require(bool condition, const char *why)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); }
}
int main()
{
    constexpr unsigned capacity = 2 * 1024 * 1024;
    auto *memory = static_cast<unsigned char *>(VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(memory != nullptr, "test pool allocated");
    std::array<std::array<char, 32>, 32> names{};
    std::array<unsigned, 33> cursor{};
    std::array<unsigned char *, 32> payload{};
    unsigned released = 0;
    for (unsigned type = 0; type != 2; ++type) {
        for (unsigned count : {3u, 8u, 32u}) {
            PMem_InitPhysicalMemory(&g_mem, "owner-stack-test", memory, capacity);
            g_physicalMemoryInit = true;
            cursor[0] = g_mem.prim[type].pos;
            for (unsigned i = 0; i != count; ++i) {
                std::snprintf(names[i].data(), names[i].size(), "owner-%u-%u", type, i);
                PMem_BeginAlloc(names[i].data(), type, TRACK_FASTFILE);
                const unsigned bytes = 17 + i * 13;
                payload[i] = _PMem_AllocNamed(bytes, 64, 4, type, names[i].data(), TRACK_FASTFILE);
                std::memset(payload[i], static_cast<int>(i + 1), bytes);
                PMem_EndAlloc(names[i].data(), type);
                cursor[i + 1] = type ? (cursor[i] - bytes) & ~63u : ((cursor[i] + 63u) & ~63u) + bytes;
                require(g_mem.prim[type].pos == cursor[i + 1], "independent allocation cursor");
            }
            for (unsigned remaining = count; remaining; --remaining) {
                PMem_Free(names[remaining - 1].data());
                ++released;
                require(g_mem.prim[type].allocListCount == remaining - 1, "exact one owner removed");
                require(g_mem.prim[type].pos == cursor[remaining - 1], "independent stack cursor restored");
                for (unsigned i = 0; i + 1 < remaining; ++i) {
                    require(g_mem.prim[type].allocList[i].name == names[i].data(), "retained name identity");
                    require(g_mem.prim[type].allocList[i].pos == cursor[i], "retained owner starting cursor");
                    for (unsigned byte = 0; byte != 17 + i * 13; ++byte)
                        require(payload[i][byte] == i + 1, "retained payload bytes");
                }
            }
            require(!g_mem.prim[0].allocListCount && !g_mem.prim[1].allocListCount
                && !g_mem.prim[0].pos && g_mem.prim[1].pos == capacity, "both primitive baselines restored");
        }
    }
    require(VirtualFree(memory, 0, MEM_RELEASE) != FALSE, "test mapping released");
    std::printf("{\"pmem_owner_stack\":{\"pointer_bytes\":%zu,\"released\":%u,\"passed\":true}}\n", sizeof(void *), released);
}
