#pragma once
#include "geometry_bindings.h"
namespace bo1::vulkan::geometry_packet {
// Deliberately narrower than general geometry equivalence: only offsets may
// differ. GPU owners remain frame-retained; no vertex/index data is rewritten.
inline bool OffsetOnly(const geometry_bindings::Snapshot& a,const geometry_bindings::Snapshot& b) {
    if(!a.indices||!b.indices||!a.indices->gpu||a.indices->gpu!=b.indices->gpu||a.indices->offset!=b.indices->offset)return false;
    bool changed=false;
    for(unsigned i=0;i<a.streams.size();++i) {
        const auto& x=a.streams[i];const auto& y=b.streams[i];
        if(bool(x.buffer)!=bool(y.buffer))return false;
        if(x.buffer) {
            if(!x.buffer->gpu||x.buffer->gpu!=y.buffer->gpu||x.stride!=y.stride)return false;
            changed|=x.buffer->offset+x.offset!=y.buffer->offset+y.offset;
        }
    }
    return a.streams[0].buffer&&changed;
}
inline bool Proven(bool enabled,const BindCache::LastDraw& last,const void* state,VkRenderPass pass,
    const PreparedProgramStamp& stamp,bool selection,bool constants,bool textures,bool offsetOnly) {
    return enabled&&last.valid&&last.state==state&&last.pass==pass&&last.programStamp==stamp
        &&selection&&constants&&textures&&offsetOnly;
}
// Called only after Proven. Retain before issuing commands; the original
// material packet still owns its pipeline, textures and descriptors. The new
// packet independently keeps every new geometry owner until the same fence.
template<class Packet> bool Record(VkCommandBuffer command,
    const std::shared_ptr<const geometry_bindings::Snapshot>& geometry,
    std::vector<Packet>& packets,BindCache& binds,int first,int vertices,int triangles,std::string& error) {
    Packet packet;packet.geometry=geometry;packets.push_back(std::move(packet));
    return geometry_bindings::DrawIndexedRetained(command,geometry,packets.back().geometry,
        &binds,first,vertices,triangles,error,false);
}
}
