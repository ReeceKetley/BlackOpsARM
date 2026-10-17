#include "target_bindings.h"
#include "sharded_states.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_rendertarget.h"
#include <unordered_map>
#include <mutex>
namespace bo1::vulkan::target_bindings {
namespace {struct Selection {const GfxRenderTarget* owner;std::weak_ptr<const render_targets::Record> record;};
ShardedStates<const GfxCmdBufState*,Selection> registry;
ShardedStates<const GfxCmdBufPrimState*,const GfxCmdBufState*> primitiveRegistry;
void SetPrimitive(const GfxCmdBufState* state,bool present) {
 auto& shard=primitiveRegistry.For(&state->prim);std::lock_guard<std::mutex> lock(shard.mutex);
 if(present)shard.map[&state->prim]=state;else shard.map.erase(&state->prim);
}}
bool Select(GfxCmdBufState* state,const GfxRenderTarget* target,std::string& error) {
 auto record=render_targets::Acquire(target);
 if(!state||!target||!record||(!record->color&&!record->depth)||!target->width||!target->height){error="Native desired target has no real attachments";return false;}
 const auto primary=record->color?record->color:record->depth;
 if(!primary->texture||!primary->texture->Handle()||primary->description.extent.width!=target->width||primary->description.extent.height!=target->height){error="Native desired target extent/resource mismatch";return false;}
 {auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);auto old=it==states.end()?nullptr:it->second.record.lock();
 const auto oldPrimary=old?(old->color?old->color:old->depth):nullptr;
 if(oldPrimary!=primary){state->viewport={0,0,int(target->width),int(target->height)};state->depthRangeNear=0;state->depthRangeFar=1;state->depthRangeType=GFX_DEPTH_RANGE_FULL;}
 states[state]={target,record};}
 SetPrimitive(state,true);return true;
}
const GfxRenderTarget* Acquire(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);return it==states.end()?nullptr:it->second.owner;}
const GfxCmdBufState* Owner(const GfxCmdBufPrimState* state){auto& shard=primitiveRegistry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(state);return it==shard.map.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){{auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;states.erase(state);}if(state)SetPrimitive(state,false);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){
 if(source==destination)return;
 bool found=false;Selection selection{};
 {auto& shard=registry.For(source);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(source);if(it!=shard.map.end()){found=true;selection=it->second;}}
 {auto& shard=registry.For(destination);std::lock_guard<std::mutex> lock(shard.mutex);if(found)shard.map[destination]=selection;else shard.map.erase(destination);}
 SetPrimitive(destination,found);
}
bool Viewport(GfxCmdBufState* state,const GfxViewport& v,std::string& error) {
 auto owner=Acquire(state);auto target=render_targets::Acquire(owner);
 if(!state||!target){error="Native viewport has no desired target";return false;}
 const auto& size=(target->color?target->color:target->depth)->description.extent;
 if(v.x<0||v.y<0||v.width<=0||v.height<=0||unsigned(v.x)>size.width||unsigned(v.y)>size.height||unsigned(v.width)>size.width-unsigned(v.x)||unsigned(v.height)>size.height-unsigned(v.y)){error="Native desired viewport exceeds its attachment";return false;}
 state->viewport=v;return true;
}
}
