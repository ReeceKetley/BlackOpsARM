#include "constant_state.h"
#include "sharded_states.h"
#include "shader_assets.h"
#include <mutex>
#include <unordered_map>
#include <cstring>
namespace bo1::vulkan::constant_state {
namespace {ShardedStates<const GfxCmdBufState*,std::shared_ptr<Snapshot>> registry;}
bool Set(const GfxCmdBufState* state,ShaderStage stage,unsigned first,unsigned rows,const float* data,std::string& error) {
    if(!shader_assets::Active()||!state||!data||!rows||first>=256||rows>256-first||(stage!=ShaderStage::Vertex&&stage!=ShaderStage::Fragment)) {
        error="Invalid native float constant register range/owner";return false;
    }
    const unsigned bank=stage==ShaderStage::Vertex?0:1;auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;
    auto& current=states[state];
    if(current&&std::memcmp(current->floats[bank][first].data(),data,std::size_t(rows)*16)==0)return true;
    // Acquire and Clone share this owner under the same lock. Only a private
    // snapshot can change in place; recorded draws keep immutable old banks.
    if(!current)current=std::make_shared<Snapshot>();
    else if(current.use_count()!=1)current=std::make_shared<Snapshot>(*current);
    std::memcpy(current->floats[bank][first].data(),data,std::size_t(rows)*16);++current->revision;return true;
}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);return it==states.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;states.erase(state);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){
    if(source==destination)return;
    std::shared_ptr<Snapshot> shared;
    {auto& shard=registry.For(source);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(source);if(it!=shard.map.end())shared=it->second;}
    auto& shard=registry.For(destination);std::lock_guard<std::mutex> lock(shard.mutex);
    if(!shared)shard.map.erase(destination);else shard.map[destination]=std::move(shared);
}
std::size_t RetireAll(){return registry.RetireAll();}
}
