#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>
namespace bo1::fastfile {
enum class AssetDirectoryResult { Ok, NegativeCount, HelperDomain, CommitOverflow, SourceBounds, InvalidSource, InvalidType, DiskBounds, NativeBounds };
struct AssetDirectoryPlan {
    bool present{};std::int32_t count{};
    std::uint64_t diskBytes{},nativeBytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{};
};
AssetDirectoryResult PlanAssetDirectory(std::int32_t count,std::uint32_t rootTag,
    const unsigned char*source,std::size_t remaining,std::uint64_t diskCapacity,
    std::uint64_t nativeCapacity,std::uint64_t diskCursor,std::uint64_t nativeCursor,
    AssetDirectoryPlan&out);
const char*AssetDirectoryResultName(AssetDirectoryResult);
}
