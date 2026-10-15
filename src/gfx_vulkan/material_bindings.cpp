#include "material_bindings.h"
#include "constant_upload.h"
#include "pointer_hash.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_dvars.h"
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <limits>
#include <algorithm>
namespace bo1::vulkan::material_bindings {
namespace {std::mutex mutex;std::unordered_map<const GfxCmdBufState*,std::shared_ptr<const Selection>,bo1::vulkan::PointerHash> states;
std::shared_ptr<Selection> Copy(const GfxCmdBufState* state){auto it=states.find(state);return it==states.end()?std::make_shared<Selection>():std::make_shared<Selection>(*it->second);}}
bool Vertex(const GfxCmdBufState* state,const MaterialVertexShader* owner,std::string& error){auto record=shader_assets::Acquire(owner);if(!state||!record){error="Native vertex shader owner missing";return false;}std::lock_guard<std::mutex> lock(mutex);auto next=Copy(state);next->vertex=std::move(record);states[state]=std::move(next);return true;}
bool Pixel(const GfxCmdBufState* state,const MaterialPixelShader* owner,std::string& error){auto record=shader_assets::Acquire(owner);if(!state||!record){error="Native pixel shader owner missing";return false;}std::lock_guard<std::mutex> lock(mutex);auto next=Copy(state);next->pixel=std::move(record);states[state]=std::move(next);return true;}
bool Declaration(const GfxCmdBufState* state,const MaterialVertexDeclaration* owner,unsigned variant,std::string& error){auto record=vertex_assets::Acquire(owner);if(!state||!record||variant>=record->variants.size()||!record->variants[variant].available){error="Native declaration variant unavailable";return false;}std::lock_guard<std::mutex> lock(mutex);auto next=Copy(state);next->declaration=std::move(record);next->variant=variant;states[state]=std::move(next);return true;}
bool Clear(const GfxCmdBufState* state,unsigned mask,std::string& error) {
    if(!state||!mask||(mask&~7u)){error="Invalid native material selection clear";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto next=Copy(state);
    if(mask&1)next->vertex.reset();if(mask&2)next->pixel.reset();if(mask&4)next->declaration.reset();
    states[state]=std::move(next);return true;
}
std::shared_ptr<const Selection> Acquire(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);return it==states.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);states.erase(state);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){std::lock_guard<std::mutex> lock(mutex);if(source==destination)return;auto it=states.find(source);if(it==states.end())states.erase(destination);else states[destination]=it->second;}
std::shared_ptr<const Draw> Capture(const GfxCmdBufState* state,std::string& error){
    if(!state){error="Native material command owner missing";return nullptr;}
    auto draw=std::make_shared<Draw>();draw->selection=Acquire(state);draw->constants=constant_state::Acquire(state);
    draw->textures=texture_bindings::Acquire(state);draw->geometry=geometry_bindings::Acquire(&state->prim);
    if(!draw->selection||!draw->selection->vertex||!draw->selection->pixel||!draw->selection->declaration||!draw->constants||!draw->geometry){error="Native draw state is incomplete";return nullptr;}
    draw->words[0]=state->activeStateBits[0];draw->words[1]=state->activeStateBits[1];
    const auto mode=(draw->words[1]>>4)&3;
    if(mode) {
        const auto bias=mode==3?sm_polygonOffsetBias:r_polygonOffsetBias;
        const auto slope=mode==3?sm_polygonOffsetScale:r_polygonOffsetScale;
        if(!bias||!slope){error="Native polygon offset dvars unavailable";return nullptr;}
        const double multiplier=mode==3?1.0:double(mode);
        draw->polygonBias=float(multiplier*double(bias->current.value)*0.000015258789);
        draw->polygonSlope=float(multiplier*double(slope->current.value));
        if(!std::isfinite(draw->polygonBias)||!std::isfinite(draw->polygonSlope)){error="Native polygon offset dvars are non-finite";return nullptr;}
    }
    return draw;
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
bool Upload(const Draw& draw,DrawStorage& storage,VkDeviceSize alignment,
    std::vector<ProgramResource>& vs,std::vector<ProgramResource>& ps,VkDescriptorPool& pool,std::string& error,const unsigned* constantRows,BindCache* cache) {
    if(!draw.constants||!draw.selection||!draw.selection->vertex||!draw.selection->pixel||!alignment
        ||alignment>std::numeric_limits<VkDeviceSize>::max()-4095){error="Invalid material draw-page capture/alignment";return false;}
    std::vector<ProgramResource> lists[2];bool needed[2]{};
    for(unsigned stage=0;stage<2;++stage) {
        const auto& shader=stage?draw.selection->pixel:draw.selection->vertex;
        for(const auto& binding:shader->translation.bindings)if(binding.kind==ResourceKind::Constants) {
            if(binding.registerIndex!=0){error="Native integer/boolean constant banks not implemented";return false;}
            needed[stage]=true;
        }
        const bool sampled=std::any_of(shader->translation.bindings.begin(),shader->translation.bindings.end(),[](const auto& binding){return binding.kind!=ResourceKind::Constants;});
        if(sampled&&(!draw.textures||!texture_bindings::AppendResources(*draw.textures,shader->translation,lists[stage],error)))return false;
    }
    // Only the rows each translated shader can read (all 256 when it indexes
    // constants dynamically) are copied and exposed to the GPU.
    VkDeviceSize size[2]{};
    for(unsigned stage=0;stage<2;++stage)if(needed[stage]) {
        const unsigned rows=constantRows?constantRows[stage]:256;
        if(!rows||rows>256){error="Invalid native constant row count";return false;}
        size[stage]=VkDeviceSize(rows)*16;
    }
    const void* data[2]={draw.constants->floats[0].data(),draw.constants->floats[1].data()};
    VkDescriptorBufferInfo ranges[2]{};
    if(!UploadConstantRanges(storage,alignment,data,size,cache,ranges,pool,error))return false;
    for(unsigned stage=0;stage<2;++stage)if(needed[stage])
        lists[stage].push_back({ResourceKind::Constants,0,ranges[stage],{}});
    vs=std::move(lists[0]);ps=std::move(lists[1]);return true;
}

}
