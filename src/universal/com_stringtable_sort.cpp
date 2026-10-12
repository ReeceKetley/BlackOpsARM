#include "com_stringtable_sort.h"
#include "com_stringtable.h"
#include <platform/win32_compat/sort16.h>
#include <bitset>
#include <cstdint>
#include <limits>

namespace bo1::stringtable {
static_assert(sizeof(*StringTable{}.cellIndex) == 2);
static int Compare(std::int16_t a, std::int16_t b, const void* context) noexcept {
    const auto& t = *static_cast<const StringTable*>(context);
    const int left = t.values[a].hash, right = t.values[b].hash;
    if (left != right) return left < right ? -1 : 1;
    return a % t.columnCount - b % t.columnCount;
}
static bool Extent(const void* p, std::size_t bytes, std::size_t alignment) noexcept {
    const auto address = reinterpret_cast<std::uintptr_t>(p);
    return p && !(address % alignment) &&
        bytes <= std::size_t(PTRDIFF_MAX) && bytes <= UINTPTR_MAX - address;
}
static bool Overlaps(const void* a, std::size_t an, const void* b, std::size_t bn) noexcept {
    const auto x = reinterpret_cast<std::uintptr_t>(a), y = reinterpret_cast<std::uintptr_t>(b);
    return x < y + bn && y < x + an; // extents have already been checked
}
NormalizeResult NormalizeIndices(StringTable* t, std::size_t valuesBytes,
                                 std::size_t indexBytes) noexcept {
    if (!Extent(t, sizeof(*t), alignof(StringTable))) return NormalizeResult::InvalidTable;
    if (t->columnCount < 0 || t->rowCount < 0) return NormalizeResult::NegativeDimension;
    const auto count = std::uint64_t(t->columnCount) * std::uint64_t(t->rowCount);
    constexpr std::size_t domain = std::size_t(INT16_MAX) + 1;
    if (count > domain) return NormalizeResult::IndexDomain;
    if (!count) return NormalizeResult::Ok;
    const auto vb = std::size_t(count) * sizeof(*t->values), ib = std::size_t(count) * sizeof(*t->cellIndex);
    if (!t->values || !t->cellIndex || valuesBytes < vb || indexBytes < ib)
        return NormalizeResult::InvalidSpan;
    if (reinterpret_cast<std::uintptr_t>(t->values) % alignof(StringTableCell) ||
        reinterpret_cast<std::uintptr_t>(t->cellIndex) % alignof(std::int16_t))
        return NormalizeResult::InvalidAlignment;
    if (!Extent(t->values, vb, alignof(StringTableCell)) ||
        !Extent(t->cellIndex, ib, alignof(std::int16_t))) return NormalizeResult::InvalidSpan;
    if (Overlaps(t->values, vb, t->cellIndex, ib) ||
        Overlaps(t, sizeof(*t), t->cellIndex, ib) ||
        Overlaps(t, sizeof(*t), t->values, vb)) return NormalizeResult::OverlappingSpan;
    std::bitset<domain> seen;
    for (std::size_t i = 0; i < count; ++i) {
        const auto value = t->cellIndex[i];
        if (value < 0 || std::uint64_t(value) >= count) return NormalizeResult::IndexRange;
        if (seen[std::size_t(value)]) return NormalizeResult::DuplicateIndex;
        seen.set(std::size_t(value));
    }
    // All potentially failing input checks precede the first swap.
    return bo1::compat::Sort16(t->cellIndex, std::size_t(count), Compare, t)
        ? NormalizeResult::Ok : NormalizeResult::InvalidSpan;
}
NormalizeResult NormalizeIndices(StringTable* t) noexcept {
    if (!Extent(t, sizeof(*t), alignof(StringTable))) return NormalizeResult::InvalidTable;
    if (t->columnCount < 0 || t->rowCount < 0) return NormalizeResult::NegativeDimension;
    const auto count = std::uint64_t(t->columnCount) * std::uint64_t(t->rowCount);
    if (count > std::uint64_t(INT16_MAX) + 1) return NormalizeResult::IndexDomain;
    return NormalizeIndices(t, std::size_t(count) * sizeof(*t->values),
                            std::size_t(count) * sizeof(*t->cellIndex));
}
const char* NormalizeResultName(NormalizeResult result) noexcept {
    switch (result) {
    case NormalizeResult::Ok: return "ok";
    case NormalizeResult::InvalidTable: return "invalid_table";
    case NormalizeResult::NegativeDimension: return "negative_dimension";
    case NormalizeResult::IndexDomain: return "index_domain";
    case NormalizeResult::InvalidSpan: return "invalid_span";
    case NormalizeResult::InvalidAlignment: return "invalid_alignment";
    case NormalizeResult::OverlappingSpan: return "overlapping_span";
    case NormalizeResult::IndexRange: return "index_range";
    case NormalizeResult::DuplicateIndex: return "duplicate_index";
    }
    return "unknown";
}
}
