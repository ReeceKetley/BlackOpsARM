#include "zone_memory.h"
#include <universal/mem_userhunk.h>
#include <universal/physicalmemory.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>
#if defined(BO1_CHECKED_ZONE_GATE)
bool Bo1_TestZoneRegistrationFailure(); // Standalone fault injection, never APK.
#endif
#if defined(__ANDROID__)
#include "hunk_services.h"
#elif defined(BO1_ZONE_MEMORY_HOST_TEST)
bool Sys_IsMainThread();
#else
#error Native zone owner is Android-only or an explicit standalone host test
#endif
namespace {
[[noreturn]] void fail(const char *why) { std::fprintf(stderr, "Native zone memory: %s\n", why); std::abort(); }
void mainThread()
{
#if defined(__ANDROID__)
    if (!bo1::android::IsHunkMainThread()) fail("main thread not registered or wrong owner");
#else
    if (!Sys_IsMainThread()) fail("wrong owner");
#endif
}
struct Zone { std::string name; HunkUser *hunk; unsigned type; bool open; std::size_t count{}, bytes{}; };
using Key = std::pair<std::string, unsigned>;
std::map<Key, std::unique_ptr<Zone>> zones;
Zone *active;
}
void bo1::android::ZoneMemoryBegin(const char *name, unsigned type, int capacity)
{
    mainThread();
    if (!name || !*name || type > 1 || capacity < 65536 || capacity > INT32_MAX - 8192 || active) fail("invalid begin");
    Key key{name, type};
    if (zones.count(key)) fail("zone already owned");
    auto zone = std::make_unique<Zone>();
    zone->name = name; zone->type = type; zone->open = true;
    zone->hunk = Hunk_UserCreate(capacity, HU_SCHEME_DEBUG, 0, nullptr, zone->name.c_str(), TRACK_FASTFILE);
    HunkUser *created = zone->hunk;
    try {
#if defined(BO1_CHECKED_ZONE_GATE)
        if (Bo1_TestZoneRegistrationFailure()) throw std::bad_alloc();
#endif
        auto entry = zones.emplace(std::move(key), std::move(zone));
        active = entry.first->second.get();
    } catch (...) {
        Hunk_UserDestroy(created); throw;
    }
}
void bo1::android::ZoneMemoryEnd()
{
    mainThread(); if (!active) fail("no active zone");
    active->open = false; active = nullptr;
}
void bo1::android::ZoneMemoryRelease(const char *name, unsigned type)
{
    mainThread(); if (!name) fail("null release");
    // Rollback after metadata bad_alloc must not allocate another string key.
    auto it = zones.begin();
    while (it != zones.end() && (it->first.second != type || it->first.first != name)) ++it;
    if (it == zones.end() || it->second->open) fail("unknown or open zone release");
    Hunk_UserDestroy(it->second->hunk); zones.erase(it);
}
std::size_t bo1::android::ZoneMemoryCount() { mainThread(); return zones.size(); }
std::size_t bo1::android::ZoneMemoryAllocationCount()
{
    mainThread(); std::size_t result = 0; for (const auto &entry : zones) result += entry.second->count; return result;
}
std::size_t bo1::android::ZoneMemoryRequestedBytes()
{
    mainThread(); std::size_t result = 0; for (const auto &entry : zones) result += entry.second->bytes; return result;
}
bool bo1::android::ZoneMemoryContains(const char *name, unsigned type)
{
    mainThread(); return name && zones.count(Key{name, type}) != 0;
}
bool bo1::android::ZoneMemoryHasOpenOwner() { mainThread(); return active != nullptr; }
unsigned __int8 *_PMem_AllocNamed(unsigned size, unsigned alignment, int type,
    unsigned allocType, const char *name, EMemTrack track)
{
    mainThread();
    if (!active || !name || active->name != name || active->type != allocType || track != TRACK_FASTFILE)
        fail("allocation outside matching named zone");
    if ((type != 4 && type != -2147482620) || !size || size > INT32_MAX - 8192u
        || !alignment || alignment > 4096 || (alignment & (alignment - 1))) fail("unsupported allocation request");
    auto *p = static_cast<unsigned __int8 *>(Hunk_UserAlloc(active->hunk, static_cast<int>(size), static_cast<int>(alignment), name));
    if (!p) fail("allocation failed");
    std::memset(p, 0, size); ++active->count; active->bytes += size;
    return p;
}
