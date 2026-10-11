#include "fastfile_raw_string.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace bo1::fastfile {
const char*RawStringResultName(RawStringResult r){switch(r){
case RawStringResult::Ok:return "ok";case RawStringResult::UnsupportedTag:return "unsupported_tag";
case RawStringResult::InvalidSource:return "invalid_source";case RawStringResult::Unterminated:return "unterminated";
case RawStringResult::CommitOverflow:return "commit_overflow";case RawStringResult::DiskBounds:return "disk_bounds";
case RawStringResult::NativeBounds:return "native_bounds";}return "unknown";}
static RawStringResult length(const unsigned char*source,std::size_t remaining,std::uint64_t&bytes){
    bytes=0;if(!source&&remaining)return RawStringResult::InvalidSource;
    if(!remaining)return RawStringResult::Unterminated;
    const auto scan=std::min(remaining,std::size_t(INT32_MAX));
    const auto*end=static_cast<const unsigned char*>(std::memchr(source,0,scan));
    if(!end)return remaining>scan?RawStringResult::CommitOverflow:RawStringResult::Unterminated;
    bytes=std::uint64_t(end-source)+1;return RawStringResult::Ok;
}
RawStringResult PlanRawString(std::uint32_t tag,const unsigned char*source,std::size_t remaining,
    std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,RawStringPlan&out){
    out={};if(w>dc)return RawStringResult::DiskBounds;if(n>nc)return RawStringResult::NativeBounds;
    RawStringPlan p;p.diskStart=p.diskEnd=w;p.nativeStart=p.nativeEnd=n;
    if(!tag){out=p;return RawStringResult::Ok;}
    if(tag!=0xffffffffu)return RawStringResult::UnsupportedTag;
    p.present=true;
    const auto result=length(source,remaining,p.bytes);if(result!=RawStringResult::Ok)return result;
    if(p.bytes>dc-w)return RawStringResult::DiskBounds;
    constexpr std::uint64_t mask=BO1_NATIVE_64BIT?sizeof(void*)-1:0;
    if(n>std::numeric_limits<std::uint64_t>::max()-mask)return RawStringResult::NativeBounds;
    p.nativeStart=(n+mask)&~mask;
    if(p.nativeStart>nc||p.bytes>nc-p.nativeStart)return RawStringResult::NativeBounds;
    p.diskEnd=w+p.bytes;p.nativeEnd=p.nativeStart+p.bytes;p.nativePadding=p.nativeStart-n;
    // Original stream cursors/shadow positions are unsigned32; Commit takes int.
    if(p.diskEnd>UINT32_MAX||p.nativeEnd>UINT32_MAX)return RawStringResult::CommitOverflow;
    out=p;return RawStringResult::Ok;
}
static bool disjoint(const unsigned char*source,std::size_t total,const unsigned char*dst,std::size_t bytes){
    if(!bytes||!total)return true;const auto s=reinterpret_cast<uintptr_t>(source),d=reinterpret_cast<uintptr_t>(dst);
    return d>=s?d-s>=total:s-d>=bytes;
}
bool DecodedSource::Read(unsigned char*dst,std::size_t capacity,int bytes){
    if(bytes<0||(!dst&&bytes)||(!data_&&bytes)||std::size_t(bytes)>Remaining()||std::size_t(bytes)>capacity||!disjoint(data_,size_,dst,std::size_t(bytes)))return false;
    if(bytes)std::memcpy(dst,Current(),std::size_t(bytes));position_+=std::size_t(bytes);return true;
}
RawStringResult DecodedSource::ReadCString(unsigned char*dst,std::size_t capacity,int&bytes){
    bytes=0;std::uint64_t count;auto result=length(Current(),Remaining(),count);if(result!=RawStringResult::Ok)return result;
    if(!dst||count>capacity)return RawStringResult::NativeBounds;
    if(!disjoint(data_,size_,dst,std::size_t(count)))return RawStringResult::InvalidSource;
    std::memcpy(dst,Current(),std::size_t(count));position_+=std::size_t(count);bytes=static_cast<int>(count);return RawStringResult::Ok;
}
}
