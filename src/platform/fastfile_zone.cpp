#include "fastfile_zone.h"
#include <platform/bo1_native_width.h>
#include <universal/mem_firstfit.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace bo1::fastfile {
const char *ZoneResultName(ZoneResult r)
{
    switch (r) {
#define ZCASE(x) case ZoneResult::x: return #x
    ZCASE(Ok); ZCASE(ShortHeader); ZCASE(SizeMismatch); ZCASE(Alignment);
    ZCASE(Unsupported); ZCASE(BlockOverflow); ZCASE(AggregateLimit);
    ZCASE(InconsistentPlan); ZCASE(OccupiedOutput); ZCASE(BeginFailed); ZCASE(AllocationFailed);
#undef ZCASE
    }
    return "Unknown";
}
bool ZoneCheckedAdd(std::uint64_t a, std::uint64_t b, std::uint64_t &out)
{
    if (b > (std::numeric_limits<std::uint64_t>::max)() - a) return false;
    out = a + b; return true;
}
static bool align(std::uint64_t n, std::uint64_t a, std::uint64_t &out)
{
    if (!ZoneCheckedAdd(n, a - 1, out)) return false;
    out &= ~(a - 1); return true;
}
ZoneResult PlanZoneHeader(const void *header, std::size_t bytes, std::uint64_t decoded,
    const ZoneLimits &limits, std::size_t page, ZonePlan &out)
{
    out = {};
    if (!header || bytes < sizeof(XFile)) return ZoneResult::ShortHeader;
    if (limits.alignment != 4096 || page < 4096 || (page & (page - 1))) return ZoneResult::Alignment;
    if (limits.allocType > 1 || limits.flags || !limits.capacityBytes || !limits.blockBytes) return ZoneResult::Unsupported;
    ZonePlan p{}; std::memcpy(&p.file, header, sizeof(XFile));
    p.decodedBytes = decoded; p.limits = limits;
    if (decoded != sizeof(XFile) + std::uint64_t(p.file.size)) return ZoneResult::SizeMismatch;
    std::uint64_t cursor = sizeof(HunkUserDebug) + sizeof(FirstFitHunkUser);
    std::uint64_t low = 0, highUsed = 0;
    for (unsigned i = 0; i < 7; ++i) {
        const std::uint64_t disk = p.file.blockSize[i];
        if (!disk) continue;
        std::uint64_t size = disk;
#if BO1_NATIVE_64BIT
        p.factor100[i] = disk >= 0x2000000u ? 135u : disk >= 0x100000u ? 190u : 200u;
        size = disk * p.factor100[i] / 100 + 0x10000;
#else
        p.factor100[i] = 100;
#endif
        const std::uint64_t request = size + ((i == 3 || i == 6) ? 0 : 15);
        if (size > UINT32_MAX || request > limits.blockBytes || request > INT32_MAX - 8192u)
            return ZoneResult::BlockOverflow;
        p.logical[i] = static_cast<unsigned>(size); p.request[i] = static_cast<unsigned>(request);
        if (!ZoneCheckedAdd(p.logicalBytes, size, p.logicalBytes)
            || !ZoneCheckedAdd(p.requestBytes, request, p.requestBytes)
            || !ZoneCheckedAdd(cursor, sizeof(_firstfit_heapnode) + sizeof(void *), cursor)
            || !align(cursor, limits.alignment, cursor)
            || !ZoneCheckedAdd(cursor, request, cursor)) return ZoneResult::AggregateLimit;
#if BO1_NATIVE_64BIT
        if (!align(cursor, alignof(_firstfit_heapnode), cursor)) return ZoneResult::AggregateLimit;
#endif
        if (!align(low, limits.alignment, low) || !ZoneCheckedAdd(low, request, low)
            || !ZoneCheckedAdd(highUsed, request, highUsed)
            || !align(highUsed, limits.alignment, highUsed)) return ZoneResult::AggregateLimit;
    }
    // Real first-fit requires a final free tail strictly larger than 1024.
    if (!ZoneCheckedAdd(cursor, 1025, cursor) || !align(cursor, page, cursor)) return ZoneResult::AggregateLimit;
    p.firstFitCapacity = (std::max<std::uint64_t>)(65536, cursor);
    p.pmemLowBytes = low; p.pmemHighBytes = highUsed;
    if (p.firstFitCapacity > limits.capacityBytes || p.firstFitCapacity > INT32_MAX - 8192u
        || p.requestBytes > limits.capacityBytes || low > limits.capacityBytes || highUsed > limits.capacityBytes)
        return ZoneResult::AggregateLimit;
    out = p; return ZoneResult::Ok;
}
static bool same(const ZonePlan &a, const ZonePlan &b)
{
    return !std::memcmp(&a.file, &b.file, sizeof(XFile)) && a.logical == b.logical
        && a.request == b.request && a.factor100 == b.factor100
        && a.logicalBytes == b.logicalBytes && a.requestBytes == b.requestBytes
        && a.firstFitCapacity == b.firstFitCapacity && a.pmemLowBytes == b.pmemLowBytes
        && a.pmemHighBytes == b.pmemHighBytes;
}
ZoneResult AllocateZonePlan(const ZonePlan &plan, std::size_t page, const ZoneBackend &backend,
    XBlock (&out)[7], unsigned &failed)
{
    failed = 7;
    for (const auto &b : out) if (b.data || b.size) return ZoneResult::OccupiedOutput;
    ZonePlan checked{};
    const auto status = PlanZoneHeader(&plan.file, sizeof(XFile), plan.decodedBytes, plan.limits, page, checked);
    if (status != ZoneResult::Ok || !same(plan, checked)) return ZoneResult::InconsistentPlan;
    if (!backend.begin || !backend.allocate || !backend.end || !backend.release) return ZoneResult::Unsupported;
    if (!checked.requestBytes) return ZoneResult::Ok;
    if (!backend.begin(backend.context, checked)) return ZoneResult::BeginFailed;
    struct Rollback {
        const ZoneBackend &b; bool open = true, retained = false;
        ~Rollback() { if (open) b.end(b.context); if (!retained) b.release(b.context); }
    } rollback{backend};
    XBlock blocks[7]{};
    for (unsigned i = 0; i < 7; ++i) {
        if (!checked.logical[i]) continue;
        auto *p = backend.allocate(backend.context, checked.logical[i], i);
        if (!p) { failed = i; return ZoneResult::AllocationFailed; }
        if (reinterpret_cast<std::uintptr_t>(p) % checked.limits.alignment) {
            failed = i; return ZoneResult::Alignment;
        }
        blocks[i] = {p, checked.logical[i]};
    }
    backend.end(backend.context); rollback.open = false;
    std::copy(std::begin(blocks), std::end(blocks), std::begin(out));
    rollback.retained = true; return ZoneResult::Ok;
}
}
