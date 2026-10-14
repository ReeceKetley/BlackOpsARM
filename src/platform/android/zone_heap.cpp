#include "zone_heap.h"
#include <universal/memory_services.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <new>
#if !defined(__ANDROID__) && !defined(BO1_ZONE_HEAP_HOST_TEST)
#error Native Android heap or explicit standalone host oracle only
#endif
namespace {
struct Allocation { std::size_t bytes; int type; std::string name; };
std::map<void *, Allocation> allocations;
std::mutex heapMutex;
std::size_t allocatedBytes;
[[noreturn]] void fail(const char *message)
{
    std::fprintf(stderr, "Android zone heap failure: %s\n", message);
    std::abort();
}
}
static void *tryAllocate(int size, const char *name, int type, bool clear)
{
    if (size < 0 || type < 0) fail("invalid allocation");
    const auto bytes = size ? static_cast<std::size_t>(size) : 1;
    void *p = clear ? std::calloc(bytes, 1) : std::malloc(bytes);
    if (!p) return nullptr;
    std::lock_guard<std::mutex> lock(heapMutex);
    try {
        if (!allocations.emplace(p, Allocation{static_cast<std::size_t>(size), type, name ? name : ""}).second)
            fail("duplicate allocation");
    } catch (const std::bad_alloc &) { std::free(p); return nullptr; }
    allocatedBytes += static_cast<std::size_t>(size);
    return p;
}
unsigned int *Z_TryMalloc(int size, const char *name, int type)
{
    return static_cast<unsigned int *>(tryAllocate(size, name, type, true));
}
char *Z_TryMallocGarbage(int size, const char *name, int type)
{
    return static_cast<char *>(tryAllocate(size, name, type, false));
}
unsigned int *Z_Malloc(int size, const char *name, int type)
{
    auto *p = Z_TryMalloc(size, name, type);
    if (!p) fail("out of memory");
    return static_cast<unsigned int *>(p);
}
char *Z_MallocGarbage(int size, const char *name, int type)
{
    auto *p = Z_TryMallocGarbage(size, name, type);
    if (!p) fail("out of memory");
    return p;
}
void Z_Free(char *ptr, int type)
{
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(heapMutex);
    const auto it = allocations.find(ptr);
    if (it == allocations.end()) fail("unowned or already released allocation");
    if (it->second.type != type) fail("allocation type mismatch");
    allocatedBytes -= it->second.bytes;
    std::free(ptr);
    allocations.erase(it);
}
std::size_t bo1::android::ZoneAllocationCount()
{
    std::lock_guard<std::mutex> lock(heapMutex);
    return allocations.size();
}
std::size_t bo1::android::ZoneAllocatedBytes()
{
    std::lock_guard<std::mutex> lock(heapMutex);
    return allocatedBytes;
}
