#include "target_bindings.h"
#include "pointer_hash.h"
#include "gfx_d3d/rb_state.h"
#include "gfx_d3d/r_rendertarget.h"
#include <unordered_map>
#include <mutex>
namespace bo1::vulkan::target_bindings {
namespace {struct Selection {const GfxRenderTarget* owner;std::weak_ptr<const render_targets::Record> record;};
std::mutex mutex;std::unordered_map<const GfxCmdBufState*,Selection,bo1::vulkan::PointerHash> states;
std::unordered_map<const GfxCmdBufPrimState*,const GfxCmdBufState*,bo1::vulkan::PointerHash> primitives;}
bool Select(GfxCmdBufState* state,const GfxRenderTarget* target,std::string& error) {
 auto record=render_targets::Acquire(target);
 if(!state||!target||!record||(!record->color&&!record->depth)||!target->width||!target->height){error="Native desired target has no real attachments";return false;}
 const auto primary=record->color?record->color:record->depth;
 if(!primary->texture||!primary->texture->Handle()||primary->description.extent.width!=target->width||primary->description.extent.height!=target->height){error="Native desired target extent/resource mismatch";return false;}
 std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);auto old=it==states.end()?nullptr:it->second.record.lock();
 const auto oldPrimary=old?(old->color?old->color:old->depth):nullptr;
 if(oldPrimary!=primary){state->viewport={0,0,int(target->width),int(target->height)};state->depthRangeNear=0;state->depthRangeFar=1;state->depthRangeType=GFX_DEPTH_RANGE_FULL;}
 states[state]={target,record};primitives[&state->prim]=state;return true;
}
const GfxRenderTarget* Acquire(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);return it==states.end()?nullptr:it->second.owner;}
const GfxCmdBufState* Owner(const GfxCmdBufPrimState* state){std::lock_guard<std::mutex> lock(mutex);auto it=primitives.find(state);return it==primitives.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);states.erase(state);if(state)primitives.erase(&state->prim);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){std::lock_guard<std::mutex> lock(mutex);if(source==destination)return;auto it=states.find(source);
 if(it==states.end()){states.erase(destination);primitives.erase(&destination->prim);}else{states[destination]=it->second;primitives[&destination->prim]=destination;}}
bool Viewport(GfxCmdBufState* state,const GfxViewport& v,std::string& error) {
 auto owner=Acquire(state);auto target=render_targets::Acquire(owner);
 if(!state||!target){error="Native viewport has no desired target";return false;}
 const auto& size=(target->color?target->color:target->depth)->description.extent;
 if(v.x<0||v.y<0||v.width<=0||v.height<=0||unsigned(v.x)>size.width||unsigned(v.y)>size.height||unsigned(v.width)>size.width-unsigned(v.x)||unsigned(v.height)>size.height-unsigned(v.y)){error="Native desired viewport exceeds its attachment";return false;}
 state->viewport=v;return true;
}
}
