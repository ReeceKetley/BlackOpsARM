#pragma once
#include <cstddef>
struct StringTable;
namespace bo1::stringtable {
enum class NormalizeResult { Ok, InvalidTable, NegativeDimension, IndexDomain,
    InvalidSpan, InvalidAlignment, OverlappingSpan, IndexRange, DuplicateIndex };
// Borrowed live readable values/writable index spans, exclusively owned by caller.
// Full permutation is validated before mutation; only index words are changed.
NormalizeResult NormalizeIndices(StringTable*, std::size_t valuesBytes,
                                 std::size_t indexBytes) noexcept;
// Production registered header contract: children are live arrays of count items.
// This overload derives extents; it cannot establish registry/zone ownership.
NormalizeResult NormalizeIndices(StringTable*) noexcept;
const char* NormalizeResultName(NormalizeResult) noexcept;
}
