#include "zone_heap.h"
#include <universal/memory_services.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
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
unsigned int *Z_Malloc(int size, const char *name, int type)
{
    if (size < 0 || type < 0) fail("invalid allocation");
    void *p = std::calloc(size ? static_cast<std::size_t>(size) : 1, 1);
    if (!p) fail("out of memory");
    std::lock_guard<std::mutex> lock(heapMutex);
    if (!allocations.emplace(p, Allocation{static_cast<std::size_t>(size), type, name ? name : ""}).second)
        fail("duplicate allocation");
    allocatedBytes += static_cast<std::size_t>(size);
    return static_cast<unsigned int *>(p);
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
