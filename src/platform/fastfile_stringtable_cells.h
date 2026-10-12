#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>
namespace bo1::fastfile {
enum class StringTableCellsResult { Ok, NegativeDimension, CountOverflow, HelperDomain, CommitOverflow, SourceBounds, InvalidSource, DiskBounds, NativeBounds };
struct StringTableCellsPlan {
    bool present{};std::int32_t count{};
    std::uint64_t diskBytes{},nativeBytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{};
};
// Child values tag is presence in the real loader. No cell tag is resolved here.
StringTableCellsResult PlanStringTableCells(std::int32_t columns,std::int32_t rows,
    std::uint32_t valuesTag,const unsigned char*source,std::size_t sourceBytes,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,
    std::uint64_t nativeCursor,StringTableCellsPlan&out);
const char*StringTableCellsResultName(StringTableCellsResult);
}
