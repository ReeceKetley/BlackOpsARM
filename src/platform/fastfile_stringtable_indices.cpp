#include "fastfile_stringtable_indices.h"
#include <bitset>
#include <cstring>
#include <limits>
namespace bo1::fastfile {
static_assert(sizeof(std::int16_t)==2);
const char*StringTableIndicesResultName(StringTableIndicesResult r){switch(r){
case StringTableIndicesResult::Ok:return "ok";case StringTableIndicesResult::NegativeDimension:return "negative_dimension";
case StringTableIndicesResult::CountOverflow:return "count_overflow";case StringTableIndicesResult::CommitOverflow:return "commit_overflow";
case StringTableIndicesResult::IndexDomain:return "index_domain";case StringTableIndicesResult::SourceBounds:return "source_bounds";
case StringTableIndicesResult::InvalidSource:return "invalid_source";case StringTableIndicesResult::DiskBounds:return "disk_bounds";
case StringTableIndicesResult::NativeBounds:return "native_bounds";case StringTableIndicesResult::IndexRange:return "index_range";
case StringTableIndicesResult::DuplicateIndex:return "duplicate_index";}return "unknown";}
StringTableIndicesResult PlanStringTableIndices(std::int32_t columns,std::int32_t rows,std::uint32_t tag,const unsigned char*source,std::size_t bytes,
    std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableIndicesPlan&out){
    out={};if(columns<0||rows<0)return StringTableIndicesResult::NegativeDimension;
    const auto product=std::uint64_t(columns)*std::uint64_t(rows);
    if(product>INT32_MAX)return StringTableIndicesResult::CountOverflow;
    if(w>dc)return StringTableIndicesResult::DiskBounds;if(n>nc)return StringTableIndicesResult::NativeBounds;
    StringTableIndicesPlan p;p.count=static_cast<std::int32_t>(product);p.present=tag!=0;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!p.present){out=p;return StringTableIndicesResult::Ok;}
    p.diskBytes=p.nativeBytes=product*sizeof(std::int16_t);
    if(p.diskBytes>INT32_MAX)return StringTableIndicesResult::CommitOverflow;
    constexpr std::size_t domain=std::size_t(INT16_MAX)+1;
    if(product>domain)return StringTableIndicesResult::IndexDomain;
    if(p.diskBytes>bytes)return StringTableIndicesResult::SourceBounds;
    if(p.diskBytes&&!source)return StringTableIndicesResult::InvalidSource;
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t cap,std::uint64_t size,std::uint64_t&start,std::uint64_t&end){if(cursor>UINT64_MAX-mask)return false;start=(cursor+mask)&~mask;if(start>cap||size>cap-start)return false;end=start+size;return true;};
    if(!align(w,1,dc,p.diskBytes,p.diskStart,p.diskEnd))return StringTableIndicesResult::DiskBounds;
    if(!nc||!align(n,(BO1_NATIVE_64BIT?sizeof(void*):2)-1,nc,p.nativeBytes,p.nativeStart,p.nativeEnd))return StringTableIndicesResult::NativeBounds;
    if(p.diskEnd>UINT32_MAX||p.nativeEnd>UINT32_MAX)return StringTableIndicesResult::CommitOverflow;
    std::bitset<domain> seen;
    for(std::size_t i=0;i<product;++i){std::int16_t value;std::memcpy(&value,source+2*i,2);
        if(value<0||std::uint64_t(value)>=product)return StringTableIndicesResult::IndexRange;
        if(seen[std::size_t(value)])return StringTableIndicesResult::DuplicateIndex;seen.set(std::size_t(value));
    }
    out=p;return StringTableIndicesResult::Ok;
}
}
