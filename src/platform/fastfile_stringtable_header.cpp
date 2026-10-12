#include "fastfile_stringtable_header.h"
#include <database/database.h>
#include <universal/com_stringtable.h>
#include <cstring>
#include <limits>
namespace bo1::fastfile {
static_assert(sizeof(StringTable)==(BO1_NATIVE_64BIT?32:20));
const char*StringTableHeaderResultName(StringTableHeaderResult r){switch(r){
case StringTableHeaderResult::Ok:return "ok";case StringTableHeaderResult::UnsupportedAsset:return "unsupported_asset";
case StringTableHeaderResult::UnsupportedTag:return "unsupported_tag";case StringTableHeaderResult::ShortHeader:return "short_header";
case StringTableHeaderResult::NegativeDimension:return "negative_dimension";case StringTableHeaderResult::CellCountOverflow:return "cell_count_overflow";
case StringTableHeaderResult::HelperDomain:return "helper_domain";case StringTableHeaderResult::SourceBounds:return "source_bounds";
case StringTableHeaderResult::DiskBounds:return "disk_bounds";case StringTableHeaderResult::NativeBounds:return "native_bounds";}return "unknown";}
StringTableHeaderResult PlanStringTableHeader(std::int32_t type,std::uint32_t tag,const unsigned char*source,std::size_t bytes,
    std::uint64_t remaining,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableHeaderPlan&out){
    out={};if(type!=ASSET_TYPE_STRINGTABLE)return StringTableHeaderResult::UnsupportedAsset;
    if(w>dc)return StringTableHeaderResult::DiskBounds;if(n>nc)return StringTableHeaderResult::NativeBounds;
    StringTableHeaderPlan p;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!tag){out=p;return StringTableHeaderResult::Ok;}
    if(tag!=0xffffffffu)return StringTableHeaderResult::UnsupportedTag;
    if(!source||bytes<20)return StringTableHeaderResult::ShortHeader;
    if(remaining<20)return StringTableHeaderResult::SourceBounds;
    std::memcpy(&p.nameTag,source,4);std::memcpy(&p.columns,source+4,4);std::memcpy(&p.rows,source+8,4);
    std::memcpy(&p.valuesTag,source+12,4);std::memcpy(&p.indexTag,source+16,4);
    if(p.columns<0||p.rows<0)return StringTableHeaderResult::NegativeDimension;
    p.cells=std::uint64_t(p.columns)*std::uint64_t(p.rows);
    if(p.cells>INT32_MAX||(p.valuesTag&&p.cells>std::uint64_t(INT32_MAX)/sizeof(StringTableCell))||(p.indexTag&&p.cells>std::uint64_t(INT32_MAX)/2))return StringTableHeaderResult::CellCountOverflow;
#if BO1_NATIVE_64BIT
    if(p.valuesTag&&p.cells>(1<<20))return StringTableHeaderResult::HelperDomain;
#endif
    // Lower bound only: names/per-cell strings are intentionally not parsed.
    const auto minimum=(p.valuesTag?p.cells*8:0)+(p.indexTag?p.cells*2:0);
    if(minimum>remaining-20)return StringTableHeaderResult::SourceBounds;
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t cap,std::uint64_t size,std::uint64_t&start,std::uint64_t&end){if(cursor>std::numeric_limits<std::uint64_t>::max()-mask)return false;start=(cursor+mask)&~mask;if(start>cap||size>cap-start)return false;end=start+size;return true;};
    if(!align(w,3,dc,20,p.diskStart,p.diskEnd))return StringTableHeaderResult::DiskBounds;
    if(!nc||!align(n,sizeof(void*)-1,nc,sizeof(StringTable),p.nativeStart,p.nativeEnd))return StringTableHeaderResult::NativeBounds;
    if(p.diskEnd>UINT32_MAX)return StringTableHeaderResult::DiskBounds;if(p.nativeEnd>UINT32_MAX)return StringTableHeaderResult::NativeBounds;
    p.present=true;out=p;return StringTableHeaderResult::Ok;
}
}
