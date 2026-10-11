#pragma once
#include <database/db_zone_types.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace bo1::fastfile {
enum class ZoneResult { Ok, ShortHeader, SizeMismatch, Alignment, Unsupported,
    BlockOverflow, AggregateLimit, InconsistentPlan, OccupiedOutput, BeginFailed, AllocationFailed };
const char *ZoneResultName(ZoneResult);
// Ceiling is the unmodified Windows reference PMem pool, not an expansion factor.
struct ZoneLimits {
    std::uint64_t capacityBytes;
    std::uint64_t blockBytes;
    unsigned alignment = 4096;
    unsigned allocType = 0;
    int flags = 0;
};
struct ZonePlan {
    XFile file{};
    std::array<unsigned, 7> logical{}, request{}, factor100{};
    std::uint64_t decodedBytes{}, logicalBytes{}, requestBytes{}, firstFitCapacity{}, pmemLowBytes{}, pmemHighBytes{};
    ZoneLimits limits{};
};
bool ZoneCheckedAdd(std::uint64_t a, std::uint64_t b, std::uint64_t &out);
// Input is the first 36 bytes of an already fully validated decoded stream.
// This planner does not certify the compressed checksum/provenance by itself.
ZoneResult PlanZoneHeader(const void *header, std::size_t headerBytes,
    std::uint64_t validatedDecodedBytes, const ZoneLimits &, std::size_t pageBytes, ZonePlan &);
// Backend owns a transaction. A successful Begin must be closed/released on
// every recoverable failure. Begin(false)/throw must leave ownership unchanged;
// End/Release must not throw. Fatal engine exits retain their fatal semantics.
struct ZoneBackend {
    void *context;
    bool (*begin)(void *, const ZonePlan &);
    unsigned char *(*allocate)(void *, unsigned logicalBytes, unsigned blockIndex);
    void (*end)(void *);
    void (*release)(void *);
};
ZoneResult AllocateZonePlan(const ZonePlan &, std::size_t pageBytes,
    const ZoneBackend &, XBlock (&out)[7], unsigned &failedBlock);
}
