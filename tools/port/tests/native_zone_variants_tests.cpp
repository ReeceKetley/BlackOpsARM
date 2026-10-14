#include <platform/android/zone_heap.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
unsigned int *Z_TryMalloc(int, const char *, int);
char *Z_TryMallocGarbage(int, const char *, int);
char *Z_MallocGarbage(int, const char *, int);
void Z_Free(char *, int);
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    const auto before = bo1::android::ZoneAllocationCount(), bytes = bo1::android::ZoneAllocatedBytes();
    auto *zeroed = Z_TryMalloc(1009, "zeroed-variant", 11);
    auto *garbage = Z_TryMallocGarbage(37, "try-variant", 12);
    auto *fatal = Z_MallocGarbage(73, "fatal-variant", 13);
    require(zeroed && garbage && fatal, "real variants allocate");
    require(!(reinterpret_cast<std::uintptr_t>(zeroed) % alignof(std::max_align_t))
        && !(reinterpret_cast<std::uintptr_t>(garbage) % alignof(std::max_align_t)), "native alignment");
    for (unsigned i = 0; i < 1009; ++i) require(reinterpret_cast<unsigned char *>(zeroed)[i] == 0, "zeroed contract");
    std::memset(garbage, 0x5a, 37); std::memset(fatal, 0x3c, 73);
    require(garbage[36] == 0x5a && fatal[72] == 0x3c, "complete writable ranges");
    require(bo1::android::ZoneAllocationCount() == before + 3 && bo1::android::ZoneAllocatedBytes() == bytes + 1119, "shared owner accounting");
    Z_Free(reinterpret_cast<char *>(zeroed), 11); Z_Free(garbage, 12); Z_Free(fatal, 13);
    auto *empty = Z_TryMalloc(0, nullptr, 11); require(empty != nullptr, "zero-size owned allocation");
    Z_Free(reinterpret_cast<char *>(empty), 11);
    require(bo1::android::ZoneAllocationCount() == before && bo1::android::ZoneAllocatedBytes() == bytes, "all variants reclaim ownership");
    std::puts("PASS real native zone try/garbage variants, alignment, zeroing, accounting and cleanup");
}
