#include "native_zone_file.h"
#include <platform/bo1_native_width.h>
#include <database/db_memory.h>
#include <database/db_stream.h>
#include <universal/physicalmemory.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#if defined(__ANDROID__)
#include "android/zone_memory.h"
#endif

namespace bo1::fastfile {
NativeZoneFile::~NativeZoneFile()
{
    // A dangling stream/registered graph must never be silently freed.
    if (Close() != NativeZoneResult::Ok) std::abort();
}
bool NativeZoneFile::Begin(void *context, const ZonePlan &plan)
{
    auto &self = *static_cast<NativeZoneFile *>(context);
#if defined(__ANDROID__)
    if (bo1::android::ZoneMemoryHasOpenOwner()
        || bo1::android::ZoneMemoryContains(self.name_.data(), plan.limits.allocType)) return false;
    bo1::android::ZoneMemoryBegin(self.name_.data(), plan.limits.allocType,
        static_cast<int>(plan.firstFitCapacity));
#else
    const auto type = plan.limits.allocType;
    if (!g_mem.buf || g_mem.prim[type].allocName) return false;
    if (g_mem.prim[type].allocListCount >= 32) return false;
#if BO1_NATIVE_64BIT
    // The legacy PMem third-record native stride is not verified. Contain this
    // component to the already verified two-owner boundary; do not alter PMem.
    if (g_mem.prim[type].allocListCount >= 2) return false;
#endif
    if (g_mem.prim[1].pos > g_mem.size || g_mem.prim[0].pos > g_mem.prim[1].pos) return false;
    // Match _PMem_AllocNamed's actual starting cursors; another retained owner
    // can leave low unaligned. No allocator policy or allocation-list change.
    std::uint64_t cursor = g_mem.prim[type].pos;
    for (const auto request : plan.request) {
        if (!request) continue;
        if (type) {
            if (cursor < request) return false;
            cursor = (cursor - request) & ~std::uint64_t(4095);
            if (cursor < g_mem.prim[0].pos) return false;
        } else {
            cursor = ((cursor + 4095) & ~std::uint64_t(4095)) + request;
            if (cursor > g_mem.prim[1].pos) return false;
        }
    }
    for (const auto &prim : g_mem.prim)
        for (unsigned i = 0; i < prim.allocListCount; ++i)
            if (prim.allocList[i].name && !std::strcmp(prim.allocList[i].name, self.name_.data())) return false;
    PMem_BeginAlloc(self.name_.data(), type, TRACK_FASTFILE);
#endif
    return true;
}
unsigned char *NativeZoneFile::Allocate(void *context, unsigned bytes, unsigned block)
{
    auto &self = *static_cast<NativeZoneFile *>(context);
    return DB_MemAlloc(bytes, block == 3 || block == 6 ? 2 : block == 0 ? 0 : 1,
        self.plan_.limits.allocType, self.name_.data(), self.plan_.limits.flags);
}
void NativeZoneFile::End(void *context)
{
    auto &self = *static_cast<NativeZoneFile *>(context);
#if defined(__ANDROID__)
    (void)self; bo1::android::ZoneMemoryEnd();
#else
    PMem_EndAlloc(self.name_.data(), self.plan_.limits.allocType);
#endif
}
void NativeZoneFile::Release(void *context)
{
    auto &self = *static_cast<NativeZoneFile *>(context);
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryRelease(self.name_.data(), self.plan_.limits.allocType);
#else
    PMem_Free(self.name_.data());
#endif
}
NativeZoneResult NativeZoneFile::Open(const char *path, const char *ownerName,
    const Bo1InflateLimits &inflate, const ZoneLimits &limits, std::size_t page,
    std::uint64_t workingSetLimit)
{
    if (open_) return NativeZoneResult::Occupied;
    if (!path || !ownerName || !*ownerName || !workingSetLimit) return NativeZoneResult::InvalidArgument;
    // Engine zone names fit the original 64-byte queue field. Bound this owned
    // metadata allocation before decoding; budget includes its retained storage.
    std::size_t nameBytes = 0;
    while (nameBytes < 64 && ownerName[nameBytes]) ++nameBytes;
    if (nameBytes == 64 || workingSetLimit <= 65) return NativeZoneResult::InvalidArgument;
    auto boundedInflate = inflate;
    if (!boundedInflate.maxWorkingBytes || boundedInflate.maxWorkingBytes > workingSetLimit - 65)
        boundedInflate.maxWorkingBytes = workingSetLimit - 65;
    decodeStatus_ = Bo1_InflateFastFilePath(path, boundedInflate, &decoded_);
    if (decodeStatus_ != Bo1InflateResult::Ok) return NativeZoneResult::DecodeFailed;
    zoneStatus_ = PlanZoneHeader(decoded_.decoded.data(), decoded_.decoded.size(),
        decoded_.decoded.size(), limits, page, plan_);
    if (zoneStatus_ != ZoneResult::Ok) { decoded_ = {}; return NativeZoneResult::PlanFailed; }
    std::uint64_t required;
    if (!ZoneCheckedAdd(decoded_.decoded.capacity(), plan_.firstFitCapacity, required)
        || !ZoneCheckedAdd(required, 65, required)
        || required > workingSetLimit) { decoded_ = {}; plan_ = {}; return NativeZoneResult::WorkingSetLimit; }
    std::memcpy(name_.data(), ownerName, nameBytes + 1);
    unsigned failed;
    try { zoneStatus_ = AllocateZonePlan(plan_, page, {this, Begin, Allocate, End, Release}, blocks_, failed); }
    catch (...) { decoded_ = {}; plan_ = {}; name_.fill(0); throw; }
    if (zoneStatus_ != ZoneResult::Ok) { decoded_ = {}; plan_ = {}; name_.fill(0); return NativeZoneResult::AllocationFailed; }
    ownsAllocation_ = plan_.requestBytes != 0;
    position_ = 0; open_ = true; return NativeZoneResult::Ok;
}
NativeZoneResult NativeZoneFile::Close()
{
    if (!open_) return NativeZoneResult::Ok;
    if (childrenRetained_) return NativeZoneResult::BorrowedChildren;
    if (g_streamBlocks == blocks_) return NativeZoneResult::StreamsAttached;
    if (g_streamBlocks) for (const auto &owned : blocks_) {
        if (!owned.data) continue;
        const auto base = reinterpret_cast<std::uintptr_t>(owned.data);
        for (unsigned i = 0; i < 7; ++i) {
            const auto bound = reinterpret_cast<std::uintptr_t>(g_streamBlocks[i].data);
            if (bound >= base && bound - base < owned.size) return NativeZoneResult::StreamsAttached;
        }
    }
    DB_FreeXBlocks(blocks_, plan_.limits.flags);
    if (ownsAllocation_) Release(this);
    decoded_ = {}; plan_ = {}; name_.fill(0); position_ = 0;
    open_ = ownsAllocation_ = false; return NativeZoneResult::Ok;
}
const unsigned char *NativeZoneFile::Current() const
{
    return open_ ? decoded_.decoded.data() + position_ : nullptr;
}
bool NativeZoneFile::Read(unsigned char *destination, std::size_t capacity, int bytes)
{
    if (!open_ || bytes < 0 || static_cast<std::size_t>(bytes) > Remaining()
        || static_cast<std::size_t>(bytes) > capacity || (!destination && bytes)) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(destination);
    const auto source = reinterpret_cast<std::uintptr_t>(decoded_.decoded.data());
    if (bytes && (address <= source ? source - address < static_cast<std::size_t>(bytes)
        : address - source < decoded_.decoded.size())) return false;
    if (bytes) std::memcpy(destination, Current(), static_cast<std::size_t>(bytes));
    position_ += static_cast<std::size_t>(bytes); return true;
}
bool NativeZoneFile::ReadCString(unsigned char *destination, std::size_t capacity, int &bytes)
{
    bytes = 0;
    if (!open_ || !destination) return false;
    const auto *end = static_cast<const unsigned char *>(std::memchr(Current(), 0, Remaining()));
    if (!end) return false;
    const auto count = static_cast<std::size_t>(end - Current()) + 1;
    if (count > capacity || count > INT_MAX) return false;
    if (!Read(destination, capacity, static_cast<int>(count))) return false;
    bytes = static_cast<int>(count); return true;
}
}
