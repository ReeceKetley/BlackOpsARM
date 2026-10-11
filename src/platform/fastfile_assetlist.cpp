#include "fastfile_assetlist.h"
#include <cstring>

namespace bo1::fastfile {
const char *AssetListResultName(AssetListResult r){
    switch(r){case AssetListResult::Ok:return "ok";case AssetListResult::ShortPrefix:return "short_prefix";
    case AssetListResult::ZoneHeader:return "zone_header";case AssetListResult::NegativeCount:return "negative_count";
    case AssetListResult::DiskFootprint:return "disk_footprint";case AssetListResult::NativeFootprint:return "native_footprint";}
    return "unknown";
}
AssetListResult PlanAssetList(const void *prefix,std::size_t n,std::uint64_t decoded,
    const ZoneLimits&limits,std::size_t page,AssetListPlan&out){
    out={};
    if(!prefix||n<sizeof(XFile)+sizeof(AssetListDisk)||decoded<sizeof(XFile)+sizeof(AssetListDisk))return AssetListResult::ShortPrefix;
    AssetListPlan p;
    if(PlanZoneHeader(prefix,sizeof(XFile),decoded,limits,page,p.zone)!=ZoneResult::Ok)return AssetListResult::ZoneHeader;
    std::memcpy(&p.disk,static_cast<const unsigned char*>(prefix)+sizeof(XFile),sizeof(p.disk));
    if(p.disk.stringCount<0||p.disk.assetCount<0)return AssetListResult::NegativeCount;
    const std::uint64_t strings=p.disk.stringsTag?static_cast<unsigned>(p.disk.stringCount):0;
    const std::uint64_t assets=p.disk.assetsTag?static_cast<unsigned>(p.disk.assetCount):0;
    // Signed int counts cannot overflow uint64 products/sum. Widths are actual
    // production layouts: packed disk slots4/assets8; native sizeof declarations.
    p.minimumDiskBytes=strings*4+assets*8;
    p.minimumNativeBytes=strings*sizeof(const char*)+assets*sizeof(XAsset);
    if(p.minimumDiskBytes>decoded-sizeof(XFile)-sizeof(AssetListDisk)||p.minimumDiskBytes>p.zone.file.blockSize[4])return AssetListResult::DiskFootprint;
    if(p.minimumNativeBytes>p.zone.logical[4])return AssetListResult::NativeFootprint;
    out=p;return AssetListResult::Ok;
}
}
