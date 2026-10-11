#include "fastfile_script_table.h"
#include <limits>

namespace bo1::fastfile {
const char *ScriptTableResultName(ScriptTableResult r){switch(r){case ScriptTableResult::Ok:return "ok";
case ScriptTableResult::NegativeCount:return "negative_count";case ScriptTableResult::HelperDomain:return "helper_domain";
case ScriptTableResult::CommitOverflow:return "commit_overflow";case ScriptTableResult::SourceBounds:return "source_bounds";
case ScriptTableResult::DiskBounds:return "disk_bounds";case ScriptTableResult::NativeBounds:return "native_bounds";}return "unknown";}
ScriptTableResult PlanScriptTable(std::int32_t count,std::uint32_t tag,std::uint64_t source,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,std::uint64_t nativeCursor,ScriptTablePlan&out){
    out={};if(count<0)return ScriptTableResult::NegativeCount;
    if(diskCursor>diskCapacity)return ScriptTableResult::DiskBounds;
    if(nativeCursor>nativeCapacity)return ScriptTableResult::NativeBounds;
    ScriptTablePlan p;p.present=tag!=0;p.count=count;
    p.diskStart=p.diskEnd=diskCursor;p.nativeStart=p.nativeEnd=nativeCursor;
    if(!p.present){out=p;return ScriptTableResult::Ok;}
    // Present-zero still calls DB_AllocStreamPos/DB_IncStreamPos and needs a
    // real nonnull block, even though no table bytes are consumed.
    if(!nativeCapacity)return ScriptTableResult::NativeBounds;
#if BO1_NATIVE_64BIT
    // Actual DB_Expand_ClampCount domain, not its no-op4096 diagnostic branch.
    if(count>(1<<20))return ScriptTableResult::HelperDomain;
#endif
    p.diskBytes=std::uint64_t(count)*4;p.nativeBytes=std::uint64_t(count)*sizeof(void*);
    if(p.diskBytes>std::uint64_t(INT32_MAX)||p.nativeBytes>std::uint64_t(INT32_MAX))return ScriptTableResult::CommitOverflow;
    if(p.diskBytes>source)return ScriptTableResult::SourceBounds;
    auto align=[](std::uint64_t cursor,std::uint64_t mask,std::uint64_t capacity,std::uint64_t bytes,std::uint64_t&start,std::uint64_t&end)->bool{
        if(cursor>std::numeric_limits<std::uint64_t>::max()-mask)return false;
        start=(cursor+mask)&~mask;
        if(start>capacity||bytes>capacity-start)return false;
        end=start+bytes;return true;
    };
    if(!align(diskCursor,3,diskCapacity,p.diskBytes,p.diskStart,p.diskEnd))return ScriptTableResult::DiskBounds;
    if(!align(nativeCursor,sizeof(void*)-1,nativeCapacity,p.nativeBytes,p.nativeStart,p.nativeEnd))return ScriptTableResult::NativeBounds;
    p.diskPadding=p.diskStart-diskCursor;p.nativePadding=p.nativeStart-nativeCursor;out=p;return ScriptTableResult::Ok;
}
}
