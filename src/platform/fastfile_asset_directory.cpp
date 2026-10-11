#include "fastfile_asset_directory.h"
#include <database/db_assetlist_types.h>
#include <cstring>
#include <limits>
namespace bo1::fastfile {
const char*AssetDirectoryResultName(AssetDirectoryResult r){switch(r){
case AssetDirectoryResult::Ok:return "ok";case AssetDirectoryResult::NegativeCount:return "negative_count";
case AssetDirectoryResult::HelperDomain:return "helper_domain";case AssetDirectoryResult::CommitOverflow:return "commit_overflow";
case AssetDirectoryResult::SourceBounds:return "source_bounds";case AssetDirectoryResult::InvalidSource:return "invalid_source";
case AssetDirectoryResult::InvalidType:return "invalid_type";case AssetDirectoryResult::DiskBounds:return "disk_bounds";
case AssetDirectoryResult::NativeBounds:return "native_bounds";}return "unknown";}
AssetDirectoryResult PlanAssetDirectory(std::int32_t count,std::uint32_t tag,const unsigned char*data,std::size_t source,
    std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,AssetDirectoryPlan&out){
    out={};if(count<0)return AssetDirectoryResult::NegativeCount;
    if(w>dc)return AssetDirectoryResult::DiskBounds;if(n>nc)return AssetDirectoryResult::NativeBounds;
    AssetDirectoryPlan p;p.present=tag!=0;p.count=count;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!p.present){out=p;return AssetDirectoryResult::Ok;}
#if BO1_NATIVE_64BIT
    if(count>(1<<20))return AssetDirectoryResult::HelperDomain;
#endif
    p.diskBytes=std::uint64_t(count)*8;p.nativeBytes=std::uint64_t(count)*sizeof(XAsset);
    if(p.diskBytes>INT32_MAX||p.nativeBytes>INT32_MAX)return AssetDirectoryResult::CommitOverflow;
    if(p.diskBytes>source)return AssetDirectoryResult::SourceBounds;
    if(p.diskBytes&&!data)return AssetDirectoryResult::InvalidSource;
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t capacity,std::uint64_t bytes,std::uint64_t&start,std::uint64_t&end){
        if(cursor>std::numeric_limits<std::uint64_t>::max()-mask)return false;
        start=(cursor+mask)&~mask;if(start>capacity||bytes>capacity-start)return false;end=start+bytes;return true;
    };
    if(!align(w,3,dc,p.diskBytes,p.diskStart,p.diskEnd))return AssetDirectoryResult::DiskBounds;
    if(!nc||!align(n,sizeof(void*)-1,nc,p.nativeBytes,p.nativeStart,p.nativeEnd))return AssetDirectoryResult::NativeBounds;
    if(p.diskEnd>UINT32_MAX||p.nativeEnd>UINT32_MAX)return AssetDirectoryResult::CommitOverflow;
    for(int i=0;i<count;++i){std::int32_t type;std::memcpy(&type,data+8*std::size_t(i),4);if(type<0||type>=ASSET_TYPE_COUNT)return AssetDirectoryResult::InvalidType;}
    out=p;return AssetDirectoryResult::Ok;
}
}
