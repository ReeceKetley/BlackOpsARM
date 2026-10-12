#include "fastfile_rawfile_header.h"
#include <database/database.h>
#include <database/db_rawfile_types.h>
#include <cstring>
#include <limits>
namespace bo1::fastfile {
const char*RawFileHeaderResultName(RawFileHeaderResult r){switch(r){
case RawFileHeaderResult::Ok:return "ok";
case RawFileHeaderResult::UnsupportedAsset:return "unsupported_asset";
case RawFileHeaderResult::UnsupportedTag:return "unsupported_tag";
case RawFileHeaderResult::ShortHeader:return "short_header";
case RawFileHeaderResult::NegativeLength:return "negative_length";
case RawFileHeaderResult::LengthOverflow:return "length_overflow";
case RawFileHeaderResult::SourceBounds:return "source_bounds";
case RawFileHeaderResult::DiskBounds:return "disk_bounds";
case RawFileHeaderResult::NativeBounds:return "native_bounds";}return "unknown";}
RawFileHeaderResult PlanRawFileHeader(std::int32_t type,std::uint32_t tag,const unsigned char*source,std::size_t bytes,
    std::uint64_t remaining,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,RawFileHeaderPlan&out){
    out={};if(type!=ASSET_TYPE_RAWFILE)return RawFileHeaderResult::UnsupportedAsset;
    if(w>dc)return RawFileHeaderResult::DiskBounds;if(n>nc)return RawFileHeaderResult::NativeBounds;
    RawFileHeaderPlan p;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!tag){out=p;return RawFileHeaderResult::Ok;}
    if(tag!=0xffffffffu)return RawFileHeaderResult::UnsupportedTag;
    if(!source||bytes<12)return RawFileHeaderResult::ShortHeader;
    if(remaining<12)return RawFileHeaderResult::SourceBounds;
    std::memcpy(&p.nameTag,source,4);std::memcpy(&p.length,source+4,4);std::memcpy(&p.bufferTag,source+8,4);
    if(p.length<0)return RawFileHeaderResult::NegativeLength;
    // Load_RawFile's nonzero-buffer branch calls Load_ConstCharArray(len+1).
    // Check that signed request without loading, duplicating or reserving children.
    if(p.bufferTag){
        if(p.length==INT32_MAX)return RawFileHeaderResult::LengthOverflow;
        p.futureBufferBytes=std::uint64_t(p.length)+1;
        if(p.futureBufferBytes>remaining-12)return RawFileHeaderResult::SourceBounds;
    }
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t cap,std::uint64_t size,std::uint64_t&start,std::uint64_t&end){
        if(cursor>std::numeric_limits<std::uint64_t>::max()-mask)return false;
        start=(cursor+mask)&~mask;if(start>cap||size>cap-start)return false;
        end=start+size;return end<=UINT32_MAX;
    };
    if(!align(w,3,dc,12,p.diskStart,p.diskEnd))return RawFileHeaderResult::DiskBounds;
    if(!nc||!align(n,3|(sizeof(void*)-1),nc,sizeof(RawFile),p.nativeStart,p.nativeEnd))return RawFileHeaderResult::NativeBounds;
    p.present=true;out=p;return RawFileHeaderResult::Ok;
}
}
