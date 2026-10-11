#include "virtual_memory.h"
#include <universal/memory_services.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>

#if !defined(__ANDROID__) || !defined(__aarch64__)
#error This virtual memory implementation requires Android ARM64
#endif

namespace {
struct Reservation { std::size_t bytes; int type = -1; };
std::map<std::uintptr_t, Reservation> reservations;
std::mutex reservationMutex;
[[noreturn]] void fail(const char *operation)
{
    std::fprintf(stderr, "Android virtual memory failure: %s\n", operation);
    std::abort();
}
std::size_t roundUp(std::size_t size)
{
    const auto page = bo1::android::VirtualPageSize();
    if (size > std::numeric_limits<std::size_t>::max() - (page - 1)) fail("size overflow");
    return (size + page - 1) & ~(page - 1);
}
struct Range { void *start; std::size_t bytes; };
// Caller holds the registry lock through the OS operation, preventing release races.
Range range(void *ptr, int size, int type)
{
    if (!ptr || size <= 0 || type < 0) fail("invalid range");
    const auto address = reinterpret_cast<std::uintptr_t>(ptr);
    auto it = reservations.upper_bound(address);
    if (it == reservations.begin()) fail("unknown reservation");
    --it;
    auto &r = it->second;
    if (address < it->first || address - it->first >= r.bytes
        || static_cast<std::size_t>(size) > r.bytes - (address - it->first)) fail("range outside reservation");
    if (r.type != -1 && r.type != type) fail("allocation type mismatch");
    r.type = type;
    const auto page = bo1::android::VirtualPageSize();
    const auto first = address & ~(static_cast<std::uintptr_t>(page) - 1);
    const auto last = first + roundUp(address - first + static_cast<std::size_t>(size));
    if (last - it->first > r.bytes) fail("rounded range outside reservation");
    return {reinterpret_cast<void *>(first), last - first};
}
}

std::size_t bo1::android::VirtualPageSize()
{
    static const auto page = [] {
        const long value = sysconf(_SC_PAGESIZE);
        if (value <= 0 || (value & (value - 1))) fail("native page size");
        return static_cast<std::size_t>(value);
    }();
    return page;
}
std::size_t bo1::android::VirtualReservationCount()
{
    std::lock_guard<std::mutex> lock(reservationMutex);
    return reservations.size();
}
void *Z_VirtualReserve(int size)
{
    if (size <= 0) fail("invalid reserve size");
    const auto bytes = roundUp(static_cast<std::size_t>(size));
    void *ptr = mmap(nullptr, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) fail("mmap reserve");
    std::lock_guard<std::mutex> lock(reservationMutex);
    if (!reservations.emplace(reinterpret_cast<std::uintptr_t>(ptr), Reservation{bytes}).second) fail("duplicate reservation");
    return ptr;
}
void Z_VirtualCommit(char *ptr, int size, int type)
{
    std::lock_guard<std::mutex> lock(reservationMutex);
    const auto r = range(ptr, size, type);
    if (mprotect(r.start, r.bytes, PROT_READ | PROT_WRITE)) fail("mprotect commit");
}
void Z_VirtualDecommit(char *ptr, int size, int type)
{
    std::lock_guard<std::mutex> lock(reservationMutex);
    const auto r = range(ptr, size, type);
    if (mprotect(r.start, r.bytes, PROT_NONE)) fail("mprotect decommit");
    if (madvise(r.start, r.bytes, MADV_DONTNEED)) fail("madvise discard");
}
void Z_VirtualFree(void *ptr, int type)
{
    std::lock_guard<std::mutex> lock(reservationMutex);
    const auto it = reservations.find(reinterpret_cast<std::uintptr_t>(ptr));
    if (it == reservations.end() || (it->second.type != -1 && it->second.type != type)) fail("invalid release");
    if (munmap(ptr, it->second.bytes)) fail("munmap release");
    reservations.erase(it);
}
