#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
namespace bo1::fastfile {
struct InitializedStringOwner {
    const unsigned char*base{};
    std::uint64_t diskCapacity{},nativeCapacity{},generation{};
};
// Caller constructs a record only after successful raw loading, source-byte/NUL
// verification and stream mapping checks. Owner must stay live; invalidate all
// records BEFORE stream reset or release. Generation must change on owner reuse.
struct InitializedStringRange {
    const unsigned char*owner{},*address{};
    std::uint64_t generation{},diskStart{},nativeStart{},bytes{};
};
enum class InitializedStringResult {
    Ok, Absent, Inline, UnsupportedTag, WrongBlock, InvalidOwner, Uninitialized,
    AmbiguousStart, StaleOwner, DiskBounds, NativeBounds, InvalidAddress, InvalidString,
    MappingMismatch // Caller compared actual mapping before conversion.
};
struct InitializedStringOffsetPlan {
    const unsigned char*address{};
    std::uint64_t bytes{},nativeStart{};
    std::uint32_t diskStart{};
};
// Certifies exact starts of earlier cell strings in block4. This is deliberately
// narrower than all possible FastFile suffix/other-block string references.
// This pure check cannot discover a freed owner: caller supplies live ownership.
InitializedStringResult PlanInitializedStringOffset(std::uint32_t tag,
    const InitializedStringOwner&owner,std::span<const InitializedStringRange>initialized,
    InitializedStringOffsetPlan&out);
const char*InitializedStringResultName(InitializedStringResult);
}
