#include "texture_bindings.h"
#include "sharded_states.h"
#include "image_usage.h"
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cmath>
namespace bo1::vulkan::texture_bindings {
namespace {
std::atomic<bool> samplingValidationReuse{false};
thread_local SamplingValidationProfile samplingProfile;
using States=std::unordered_map<const GfxCmdBufState*,std::shared_ptr<const Snapshot>,bo1::vulkan::PointerHash>;
ShardedStates<const GfxCmdBufState*,std::shared_ptr<const Snapshot>> registry;
// Copy-on-write: a snapshot retained by a draw (or the bind fast path, which
// compares snapshot identity) is never modified. One held only by this map
// (checked under its shard lock, which every reader of it takes) is updated in place.
Snapshot& Writable(std::shared_ptr<const Snapshot>& current) {
    if(!current)current=std::make_shared<Snapshot>();
    else if(current.use_count()!=1)current=std::make_shared<Snapshot>(*current);
    return const_cast<Snapshot&>(*current);
}
bool Publish(States& states,const GfxCmdBufState* state,unsigned slot,std::shared_ptr<const image_assets::Record> image,unsigned decoded,float bias,std::string& error) {
    if(!image->description.linearSampling&&
        (((decoded>>8)&15)>1||((decoded>>12)&15)>1||((decoded>>16)&15)==2)) {
        error="Native image format does not support requested linear sampling";return false;
    }
    auto it=states.find(state);
    if(it!=states.end()) {
        const auto& old=it->second->slots[slot];
        if(old.image==image&&old.decoded==decoded&&old.bias==bias)return true;
    }
    auto sampler=Sampler::Acquire(image->texture->Device(),decoded,image->description.mipLevels,bias,error,image->depthComparison);
    if(!sampler)return false;
    auto& current=it==states.end()?states[state]:it->second;
    Writable(current).slots[slot]={std::move(image),std::move(sampler),decoded,bias};return true;
}
}
bool Set(const GfxCmdBufState* state,unsigned slot,const GfxImage* owner,unsigned decoded,float bias,std::string& error) {
    if(!state||slot>=16) {error="Invalid native texture command owner/slot";return false;}
    auto image=image_assets::Acquire(owner);
    if(!image) {error=image_assets::MissingOwnerDiagnostic(owner);return false;}
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;return Publish(states,state,slot,std::move(image),decoded,bias,error);
}
bool Update(const GfxCmdBufState* state,unsigned slot,unsigned decoded,float bias,std::string& error) {
    if(!state||slot>=16) {error="Invalid native sampler update owner/slot";return false;}
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);
    if(it==states.end()||!it->second->slots[slot].image) {
        // D3D sampler state is independent of image binding. Preserve the real
        // pending values; no image, VkSampler, or draw readiness is invented.
        if(!Sampler::Validate(decoded,bias,error))return false;
        auto& current=it==states.end()?states[state]:it->second;
        auto& binding=Writable(current).slots[slot];binding.decoded=decoded;binding.bias=bias;return true;
    }
    return Publish(states,state,slot,it->second->slots[slot].image,decoded,bias,error);
}
void Disable(const GfxCmdBufState* state,unsigned slot) {
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);if(it==states.end()||slot>=16)return;
    Writable(it->second).slots[slot]={};
}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination) {
    if(source==destination)return;
    std::shared_ptr<const Snapshot> shared;
    {auto& shard=registry.For(source);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(source);if(it!=shard.map.end())shared=it->second;}
    auto& shard=registry.For(destination);std::lock_guard<std::mutex> lock(shard.mutex);
    if(!shared)shard.map.erase(destination);else shard.map[destination]=std::move(shared);
}
void Release(const GfxCmdBufState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;states.erase(state);}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState* state) {
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);return it==states.end()?nullptr:it->second;
}
void SetSamplingValidationReuse(bool enabled){samplingValidationReuse.store(enabled,std::memory_order_relaxed);}
bool SamplingValidationReuseEnabled(){return samplingValidationReuse.load(std::memory_order_relaxed);}
SamplingValidationProfile ReadSamplingValidationProfile(){auto result=samplingProfile;result.lookups=image_usage::SamplingLookupCount();return result;}
bool AppendResources(const Snapshot& snapshot,const ShaderTranslation& shader,std::vector<ProgramResource>& resources,std::string& error) {
    std::vector<ProgramResource> added;return AppendResourcesWithScratch(snapshot,shader,resources,added,error);
}
bool AppendResourcesWithScratch(const Snapshot& snapshot,const ShaderTranslation& shader,std::vector<ProgramResource>& resources,
    std::vector<ProgramResource>& added,std::string& error) {
    if(&resources==&added){error="Native resource scratch aliases destination";return false;}
    added.clear();
    const bool reuse=SamplingValidationReuseEnabled();
    std::array<bool,16> validated{};
    std::array<VkImageLayout,16> layouts{};
    for(const auto& reflected:shader.bindings) {
        if(reflected.kind==ResourceKind::Constants)continue;
        if(reflected.registerIndex>=16) {error="Native texture register exceeds command slots";return false;}
        const auto& bound=snapshot.slots[reflected.registerIndex];
        if(!bound.image||!bound.sampler) {error="Native shader texture/sampler is unbound slot="+std::to_string(reflected.registerIndex)+" stage="+std::to_string(unsigned(shader.stage))+" image="+std::to_string(bool(bound.image))+" sampler="+std::to_string(bool(bound.sampler));return false;}
        VkImageLayout layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if(reuse&&validated[reflected.registerIndex]) {
            ++samplingProfile.slotReuses;layout=layouts[reflected.registerIndex];
        } else {
            ++samplingProfile.slotValidations;
            if(reuse) {
                bool known=false;
                if(!image_usage::ResolveSampling(bound.image->texture,bound.image->uploads.empty(),layout,known,error)) {
                    error+=" image="+bound.image->createdName+" slot="+std::to_string(reflected.registerIndex)+" serial="+std::to_string(bound.image->serial);return false;
                }
                if(!known&&!bound.image->uploads.empty()&&bound.image->uploads.size()!=bound.image->description.mipLevels*bound.image->description.arrayLayers) {
                    error="Native sampled image mip/face uploads incomplete";return false;
                }
            } else {
            if(image_usage::Known(bound.image->texture)||bound.image->uploads.empty()) {
                if(!image_usage::Sample(bound.image->texture,layout,error)){error+=" image="+bound.image->createdName+" slot="+std::to_string(reflected.registerIndex)+" serial="+std::to_string(bound.image->serial);return false;}
            } else if(bound.image->uploads.size()!=bound.image->description.mipLevels*bound.image->description.arrayLayers) {
                error="Native sampled image mip/face uploads incomplete";return false;
            }
            }
            validated[reflected.registerIndex]=true;layouts[reflected.registerIndex]=layout;
        }
        ProgramResource resource{};resource.kind=reflected.kind;resource.registerIndex=reflected.registerIndex;
        if(resource.kind==ResourceKind::Texture)resource.image={VK_NULL_HANDLE,bound.image->texture->View(),layout};
        else resource.image={bound.sampler->Handle(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
        added.push_back(resource);
    }
    resources.insert(resources.end(),added.begin(),added.end());return true;
}
std::size_t RetireAll(){return registry.RetireAll();}
}
