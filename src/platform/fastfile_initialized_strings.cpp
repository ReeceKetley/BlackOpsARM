#include "fastfile_initialized_strings.h"
#include <cstring>
#include <limits>
namespace bo1::fastfile {
const char*InitializedStringResultName(InitializedStringResult r){switch(r){
case InitializedStringResult::Ok:return "ok";case InitializedStringResult::Absent:return "absent";
case InitializedStringResult::Inline:return "inline";case InitializedStringResult::UnsupportedTag:return "unsupported_tag";
case InitializedStringResult::WrongBlock:return "wrong_block";case InitializedStringResult::InvalidOwner:return "invalid_owner";
case InitializedStringResult::Uninitialized:return "uninitialized";case InitializedStringResult::AmbiguousStart:return "ambiguous_start";
case InitializedStringResult::StaleOwner:return "stale_owner";case InitializedStringResult::DiskBounds:return "disk_bounds";
case InitializedStringResult::NativeBounds:return "native_bounds";case InitializedStringResult::InvalidAddress:return "invalid_address";
case InitializedStringResult::InvalidString:return "invalid_string";
case InitializedStringResult::MappingMismatch:return "mapping_mismatch";}return "unknown";}
InitializedStringResult PlanInitializedStringOffset(std::uint32_t tag,const InitializedStringOwner&owner,
    std::span<const InitializedStringRange>initialized,InitializedStringOffsetPlan&out){
    out={};if(!tag)return InitializedStringResult::Absent;
    if(tag==0xffffffffu)return InitializedStringResult::Inline;
    if(tag==0xfffffffeu)return InitializedStringResult::UnsupportedTag;
    const auto block=(tag-1)>>29,within=(tag-1)&0x1fffffffu;
    if(block!=4)return InitializedStringResult::WrongBlock;
    if(!owner.base||!owner.generation)return InitializedStringResult::InvalidOwner;
    const auto base=reinterpret_cast<std::uintptr_t>(owner.base);
    if(owner.nativeCapacity>std::numeric_limits<std::uintptr_t>::max()-base)return InitializedStringResult::NativeBounds;
    const InitializedStringRange*match=nullptr;
    for(const auto&range:initialized)if(range.diskStart==within){if(match)return InitializedStringResult::AmbiguousStart;match=&range;}
    if(!match)return InitializedStringResult::Uninitialized;
    const auto&r=*match;
    if(r.owner!=owner.base||r.generation!=owner.generation)return InitializedStringResult::StaleOwner;
    if(r.diskStart>owner.diskCapacity||r.bytes>owner.diskCapacity-r.diskStart||r.bytes>UINT32_MAX-r.diskStart)return InitializedStringResult::DiskBounds;
    if(r.nativeStart>owner.nativeCapacity||r.bytes>owner.nativeCapacity-r.nativeStart||r.nativeStart>UINT32_MAX||r.bytes>UINT32_MAX-r.nativeStart||r.bytes>SIZE_MAX)return InitializedStringResult::NativeBounds;
    if(r.address!=owner.base+static_cast<std::size_t>(r.nativeStart))return InitializedStringResult::InvalidAddress;
    // Scan only a previously initialized string's declared length, never block
    // capacity, padding, descriptors or untouched zero-filled reservation pages.
    if(!r.bytes||std::memchr(r.address,0,static_cast<std::size_t>(r.bytes))!=r.address+static_cast<std::size_t>(r.bytes)-1)return InitializedStringResult::InvalidString;
    out={r.address,r.bytes,r.nativeStart,within};return InitializedStringResult::Ok;
}
}
