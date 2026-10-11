#pragma once
#include <platform/bo1_native_width.h>
#include <cstdint>

namespace bo1::fastfile {
enum class ScriptTableResult { Ok, NegativeCount, HelperDomain, CommitOverflow, SourceBounds, DiskBounds, NativeBounds };
struct ScriptTablePlan {
    bool present{};
    std::int32_t count{};
    std::uint64_t diskBytes{},nativeBytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{},diskPadding{},nativePadding{};
};
// Disk/native cursor footprints are separate from decoded FILE consumption.
// Root null means absent, even with a positive count. Present zero still aligns.
ScriptTableResult PlanScriptTable(std::int32_t count,std::uint32_t rootTag,
    std::uint64_t sourceRemaining,std::uint64_t diskCapacity,std::uint64_t nativeCapacity,
    std::uint64_t diskCursor,std::uint64_t nativeCursor,ScriptTablePlan&out);
const char *ScriptTableResultName(ScriptTableResult);
}
