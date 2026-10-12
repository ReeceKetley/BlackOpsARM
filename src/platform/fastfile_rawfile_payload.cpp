#include "fastfile_rawfile_payload.h"
#include <limits>
namespace bo1::fastfile {
const char*RawFilePayloadResultName(RawFilePayloadResult r){switch(r){
case RawFilePayloadResult::Ok:return "ok";case RawFilePayloadResult::Name:return "name";
case RawFilePayloadResult::BufferLength:return "buffer_length";case RawFilePayloadResult::SourceBounds:return "source_bounds";
case RawFilePayloadResult::DiskBounds:return "disk_bounds";case RawFilePayloadResult::NativeBounds:return "native_bounds";
case RawFilePayloadResult::CommitOverflow:return "commit_overflow";}return "unknown";}
RawFilePayloadResult PlanRawFilePayload(std::uint32_t tag,std::int32_t length,std::uint32_t buffer,
    const unsigned char*source,std::size_t bytes,std::uint64_t dc,std::uint64_t nc,
    std::uint64_t w,std::uint64_t n,RawFilePayloadPlan&out){
    out={};RawFilePayloadPlan p;
    if(w>dc)return RawFilePayloadResult::DiskBounds;
    if(n>nc)return RawFilePayloadResult::NativeBounds;
    if(PlanRawString(tag,source,bytes,dc,nc,w,n,p.name)!=RawStringResult::Ok)return RawFilePayloadResult::Name;
    p.sourceBytes=p.name.bytes;p.diskEnd=p.name.diskEnd;p.nativeEnd=p.name.nativeEnd;
    if(buffer){
        if(length<0||length==INT32_MAX)return RawFilePayloadResult::BufferLength;
        p.bufferBytes=std::uint64_t(length)+1;
        if(!source||p.sourceBytes>bytes||p.bufferBytes>bytes-p.sourceBytes)return RawFilePayloadResult::SourceBounds;
        p.bufferDiskStart=p.diskEnd;
        if(p.bufferBytes>dc-p.diskEnd)return RawFilePayloadResult::DiskBounds;
        constexpr std::uint64_t mask=BO1_NATIVE_64BIT?sizeof(void*)-1:0;
        if(p.nativeEnd>std::numeric_limits<std::uint64_t>::max()-mask)return RawFilePayloadResult::NativeBounds;
        p.bufferNativeStart=(p.nativeEnd+mask)&~mask;
        if(p.bufferNativeStart>nc||p.bufferBytes>nc-p.bufferNativeStart)return RawFilePayloadResult::NativeBounds;
        p.diskEnd+=p.bufferBytes;p.nativeEnd=p.bufferNativeStart+p.bufferBytes;
        p.sourceBytes+=p.bufferBytes;p.bufferPresent=true;
    }
    if(p.diskEnd>UINT32_MAX||p.nativeEnd>UINT32_MAX)return RawFilePayloadResult::CommitOverflow;
    out=p;return RawFilePayloadResult::Ok;
}
}
