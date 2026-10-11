#pragma once
#include "fastfile_zone.h"
#include <database/db_assetlist_types.h>

namespace bo1::fastfile {
struct AssetListDisk {
    std::int32_t stringCount;
    std::uint32_t stringsTag;
    std::int32_t assetCount;
    std::uint32_t assetsTag;
};
static_assert(sizeof(AssetListDisk)==16 && offsetof(AssetListDisk,assetsTag)==12);
enum class AssetListResult { Ok, ShortPrefix, ZoneHeader, NegativeCount, DiskFootprint, NativeFootprint };
struct AssetListPlan {
    ZonePlan zone{};
    AssetListDisk disk{};
    std::uint64_t minimumDiskBytes{},minimumNativeBytes{};
};
// Additional checked gate policy, not a claim that the legacy expander checks
// counts. Prefix comes from an independently validated COMPLETE decode.
// Nonzero root tags mean array presence, not addresses to dereference here.
AssetListResult PlanAssetList(const void *prefix,std::size_t prefixBytes,
    std::uint64_t validatedDecodedBytes,const ZoneLimits&,std::size_t pageBytes,AssetListPlan&);
const char *AssetListResultName(AssetListResult);
}
