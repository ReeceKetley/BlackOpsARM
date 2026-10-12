#include "fastfile_stringtable_cells.h"
#include <universal/com_stringtable.h>
#include <limits>
namespace bo1::fastfile {
static_assert(sizeof(StringTableCell)==(BO1_NATIVE_64BIT?16:8));
const char*StringTableCellsResultName(StringTableCellsResult r){switch(r){
case StringTableCellsResult::Ok:return "ok";case StringTableCellsResult::NegativeDimension:return "negative_dimension";
case StringTableCellsResult::CountOverflow:return "count_overflow";case StringTableCellsResult::HelperDomain:return "helper_domain";
case StringTableCellsResult::CommitOverflow:return "commit_overflow";case StringTableCellsResult::SourceBounds:return "source_bounds";
case StringTableCellsResult::InvalidSource:return "invalid_source";case StringTableCellsResult::DiskBounds:return "disk_bounds";
case StringTableCellsResult::NativeBounds:return "native_bounds";}return "unknown";}
StringTableCellsResult PlanStringTableCells(std::int32_t columns,std::int32_t rows,std::uint32_t tag,const unsigned char*source,std::size_t bytes,
    std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableCellsPlan&out){
    out={};if(columns<0||rows<0)return StringTableCellsResult::NegativeDimension;
    const auto product=std::uint64_t(columns)*std::uint64_t(rows);
    if(product>INT32_MAX)return StringTableCellsResult::CountOverflow;
    if(w>dc)return StringTableCellsResult::DiskBounds;if(n>nc)return StringTableCellsResult::NativeBounds;
    StringTableCellsPlan p;p.count=static_cast<std::int32_t>(product);p.present=tag!=0;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!p.present){out=p;return StringTableCellsResult::Ok;}
#if BO1_NATIVE_64BIT
    if(product>(1<<20))return StringTableCellsResult::HelperDomain;
#endif
    p.diskBytes=product*8;p.nativeBytes=product*sizeof(StringTableCell);
    if(p.diskBytes>INT32_MAX||p.nativeBytes>INT32_MAX)return StringTableCellsResult::CommitOverflow;
    if(p.diskBytes>bytes)return StringTableCellsResult::SourceBounds;
    if(p.diskBytes&&!source)return StringTableCellsResult::InvalidSource;
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t cap,std::uint64_t size,std::uint64_t&start,std::uint64_t&end){if(cursor>std::numeric_limits<std::uint64_t>::max()-mask)return false;start=(cursor+mask)&~mask;if(start>cap||size>cap-start)return false;end=start+size;return true;};
    if(!align(w,3,dc,p.diskBytes,p.diskStart,p.diskEnd))return StringTableCellsResult::DiskBounds;
    if(!nc||!align(n,sizeof(void*)-1,nc,p.nativeBytes,p.nativeStart,p.nativeEnd))return StringTableCellsResult::NativeBounds;
    if(p.diskEnd>UINT32_MAX||p.nativeEnd>UINT32_MAX)return StringTableCellsResult::CommitOverflow;
    out=p;return StringTableCellsResult::Ok;
}
}
