#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>
namespace bo1::fastfile {
enum class StringTableHeaderResult { Ok, UnsupportedAsset, UnsupportedTag, ShortHeader, NegativeDimension, CellCountOverflow, HelperDomain, SourceBounds, DiskBounds, NativeBounds };
struct StringTableHeaderPlan {
    bool present{};std::int32_t columns{},rows{};
    std::uint32_t nameTag{},valuesTag{},indexTag{};
    std::uint64_t cells{},diskStart{},nativeStart{},diskEnd{},nativeEnd{};
};
// First inline(-1) StringTable only. -2 needs the real insertion path and encoded
// asset aliases need a later validated fixup boundary. Inner tags are preserved.
StringTableHeaderResult PlanStringTableHeader(std::int32_t assetType,std::uint32_t assetTag,
    const unsigned char*source,std::size_t sourceBytes,std::uint64_t decodedRemaining,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,
    std::uint64_t nativeCursor,StringTableHeaderPlan&out);
const char*StringTableHeaderResultName(StringTableHeaderResult);
}
