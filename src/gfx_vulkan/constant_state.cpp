#include "constant_state.h"
#include "pointer_hash.h"
#include "shader_assets.h"
#include <mutex>
#include <unordered_map>
#include <cstring>
namespace bo1::vulkan::constant_state {
namespace {std::mutex mutex;std::unordered_map<const GfxCmdBufState*,std::shared_ptr<Snapshot>,bo1::vulkan::PointerHash> states;}
bool Set(const GfxCmdBufState* state,ShaderStage stage,unsigned first,unsigned rows,const float* data,std::string& error) {
    if(!shader_assets::Active()||!state||!data||!rows||first>=256||rows>256-first||(stage!=ShaderStage::Vertex&&stage!=ShaderStage::Fragment)) {
        error="Invalid native float constant register range/owner";return false;
    }
    const unsigned bank=stage==ShaderStage::Vertex?0:1;std::lock_guard<std::mutex> lock(mutex);
    auto& current=states[state];
    if(current&&std::memcmp(current->floats[bank][first].data(),data,std::size_t(rows)*16)==0)return true;
    // Acquire and Clone share this owner under the same lock. Only a private
    // snapshot can change in place; recorded draws keep immutable old banks.
    if(!current)current=std::make_shared<Snapshot>();
    else if(current.use_count()!=1)current=std::make_shared<Snapshot>(*current);
    std::memcpy(current->floats[bank][first].data(),data,std::size_t(rows)*16);++current->revision;return true;
}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);return it==states.end()?nullptr:it->second;}
void Release(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);states.erase(state);}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination){std::lock_guard<std::mutex> lock(mutex);if(source==destination)return;auto it=states.find(source);if(it==states.end())states.erase(destination);else states[destination]=it->second;}
}
