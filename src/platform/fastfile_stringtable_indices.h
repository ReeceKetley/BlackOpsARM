#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>
namespace bo1::fastfile {
enum class StringTableIndicesResult { Ok, NegativeDimension, CountOverflow, CommitOverflow, IndexDomain, SourceBounds, InvalidSource, DiskBounds, NativeBounds, IndexRange, DuplicateIndex };
struct StringTableIndicesPlan {
    bool present{};std::int32_t count{};
    std::uint64_t diskBytes{},nativeBytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{};
};
// The signed16 permutation domain is mathematical, not an allocator/helper cap.
// Preserve retail order: wrapping hash comparators do not imply signed sorting.
StringTableIndicesResult PlanStringTableIndices(std::int32_t columns,std::int32_t rows,
    std::uint32_t tag,const unsigned char*source,std::size_t sourceBytes,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,
    std::uint64_t nativeCursor,StringTableIndicesPlan&out);
const char*StringTableIndicesResultName(StringTableIndicesResult);
}
