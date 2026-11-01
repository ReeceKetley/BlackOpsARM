#include "material_bindings.h"
#include "constant_upload.h"
#include "sharded_states.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_dvars.h"
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <limits>
#include <algorithm>
namespace bo1::vulkan::material_bindings {
namespace {ShardedStates<const GfxCmdBufState*,std::shared_ptr<const Selection>> registry;
using States=std::unordered_map<const GfxCmdBufState*,std::shared_ptr<const Selection>,bo1::vulkan::PointerHash>;
std::shared_ptr<Selection> Copy(States& states,const GfxCmdBufState* state){auto it=states.find(state);return it==states.end()?std::make_shared<Selection>():std::make_shared<Selection>(*it->second);}}
bool Vertex(const GfxCmdBufState* state,const MaterialVertexShader* owner,std::string& error){auto record=shader_assets::Acquire(owner);if(!state||!record){error="Native vertex shader owner missing";return false;}shader_assets::TraceIntegrity(record,"select_vs");auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto next=Copy(states,state);next->vertex=std::move(record);states[state]=std::move(next);return true;}
bool Pixel(const GfxCmdBufState* state,const MaterialPixelShader* owner,std::string& error){auto record=shader_assets::Acquire(owner);if(!state||!record){error="Native pixel shader owner missing";return false;}shader_assets::TraceIntegrity(record,"select_ps");auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto next=Copy(states,state);next->pixel=std::move(record);states[state]=std::move(next);return true;}
bool Declaration(const GfxCmdBufState* state,const MaterialVertexDeclaration* owner,unsigned variant,std::string& error){auto record=vertex_assets::Acquire(owner);if(!state||!record||variant>=record->variants.size()||!record->variants[variant].available){error="Native declaration variant unavailable";return false;}auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto next=Copy(states,state);next->declaration=std::move(record);next->variant=variant;states[state]=std::move(next);return true;}
bool Clear(const GfxCmdBufState* state,unsigned mask,std::string& error) {
    if(!state||!mask||(mask&~7u)){error="Invalid native material selection clear";return false;}
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto next=Copy(states,state);
    if(mask&1)next->vertex.reset();if(mask&2)next->pixel.reset();if(mask&4)next->declaration.reset();
    states[state]=std::move(next);return true;
}
std::shared_ptr<const Selection> Acquire(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);return it==states.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;states.erase(state);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){
    if(source==destination)return;
    std::shared_ptr<const Selection> shared;
    {auto& shard=registry.For(source);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(source);if(it!=shard.map.end())shared=it->second;}
    auto& shard=registry.For(destination);std::lock_guard<std::mutex> lock(shard.mutex);
    if(!shared)shard.map.erase(destination);else shard.map[destination]=std::move(shared);
}
bool Capture(const GfxCmdBufState* state,Draw& captured,std::string& error){
    captured={};auto* draw=&captured;
    if(!state){error="Native material command owner missing";return false;}
    draw->selection=Acquire(state);draw->constants=constant_state::Acquire(state);
    draw->textures=texture_bindings::Acquire(state);draw->geometry=geometry_bindings::Acquire(&state->prim);
    if(!draw->selection||!draw->selection->vertex||!draw->selection->pixel||!draw->selection->declaration||!draw->constants||!draw->geometry){error="Native draw state is incomplete";return false;}
    shader_assets::TraceIntegrity(draw->selection->vertex,"capture_vs");shader_assets::TraceIntegrity(draw->selection->pixel,"capture_ps");
    draw->words[0]=state->activeStateBits[0];draw->words[1]=state->activeStateBits[1];
    const auto mode=(draw->words[1]>>4)&3;
    if(mode) {
        const auto bias=mode==3?sm_polygonOffsetBias:r_polygonOffsetBias;
        const auto slope=mode==3?sm_polygonOffsetScale:r_polygonOffsetScale;
        if(!bias||!slope){error="Native polygon offset dvars unavailable";return false;}
        const double multiplier=mode==3?1.0:double(mode);
        draw->polygonBias=float(multiplier*double(bias->current.value)*0.000015258789);
        draw->polygonSlope=float(multiplier*double(slope->current.value));
        if(!std::isfinite(draw->polygonBias)||!std::isfinite(draw->polygonSlope)){error="Native polygon offset dvars are non-finite";return false;}
    }
    return true;
}
std::shared_ptr<const Draw> Capture(const GfxCmdBufState* state,std::string& error){
    auto draw=std::make_shared<Draw>();if(!Capture(state,*draw,error))return nullptr;return draw;
}
bool Upload(const Draw& draw,Buffer& buffer,VkDeviceSize& cursor,VkDeviceSize alignment,
    std::vector<ProgramResource>& vs,std::vector<ProgramResource>& ps,std::string& error){
    if(!draw.constants||!draw.selection||!alignment||cursor>std::numeric_limits<VkDeviceSize>::max()-(alignment-1)){error="Invalid material uniform arena";return false;}
    const auto first=((cursor+alignment-1)/alignment)*alignment;
    if(first>std::numeric_limits<VkDeviceSize>::max()-4096-(alignment-1)){error="Material uniform offset overflow";return false;}
    const auto second=((first+4096+alignment-1)/alignment)*alignment;
    if(second>buffer.Size()||4096>buffer.Size()-second){error="Material uniform arena exhausted";return false;}
    std::vector<ProgramResource> vertex,pixel;
    for(unsigned stage=0;stage<2;++stage){const auto& shader=stage?draw.selection->pixel:draw.selection->vertex;auto& list=stage?pixel:vertex;
        for(const auto& binding:shader->translation.bindings)if(binding.kind==ResourceKind::Constants){
            if(binding.registerIndex!=0){error="Native integer/boolean constant banks not implemented";return false;}
            list.push_back({ResourceKind::Constants,0,{buffer.Handle(),stage?second:first,4096},{}});
        }
        const bool sampled=std::any_of(shader->translation.bindings.begin(),shader->translation.bindings.end(),[](const auto& binding){return binding.kind!=ResourceKind::Constants;});
        if(sampled&&(!draw.textures||!texture_bindings::AppendResources(*draw.textures,shader->translation,list,error)))return false;
    }
    if(!buffer.Write(first,draw.constants->floats[0].data(),4096,error)||!buffer.Write(second,draw.constants->floats[1].data(),4096,error))return false;
    vs=std::move(vertex);ps=std::move(pixel);cursor=second+4096;return true;
}


std::size_t RetireAll(){return registry.RetireAll();}
}
