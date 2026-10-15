#include "texture_bindings.h"
#include "pointer_hash.h"
#include "image_usage.h"
#include <mutex>
#include <unordered_map>
#include <cmath>
namespace bo1::vulkan::texture_bindings {
namespace {
std::mutex mutex;
std::unordered_map<const GfxCmdBufState*,std::shared_ptr<const Snapshot>,bo1::vulkan::PointerHash> states;
bool Publish(const GfxCmdBufState* state,unsigned slot,std::shared_ptr<const image_assets::Record> image,unsigned decoded,float bias,std::string& error) {
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
    auto next=it==states.end()?std::make_shared<Snapshot>():std::make_shared<Snapshot>(*it->second);
    next->slots[slot]={std::move(image),std::move(sampler),decoded,bias};states[state]=std::move(next);return true;
}
}
bool Set(const GfxCmdBufState* state,unsigned slot,const GfxImage* owner,unsigned decoded,float bias,std::string& error) {
    if(!state||slot>=16) {error="Invalid native texture command owner/slot";return false;}
    auto image=image_assets::Acquire(owner);
    if(!image) {error=image_assets::MissingOwnerDiagnostic(owner);return false;}
    std::lock_guard<std::mutex> lock(mutex);return Publish(state,slot,std::move(image),decoded,bias,error);
}
bool Update(const GfxCmdBufState* state,unsigned slot,unsigned decoded,float bias,std::string& error) {
    if(!state||slot>=16) {error="Invalid native sampler update owner/slot";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);
    if(it==states.end()||!it->second->slots[slot].image) {
        // D3D sampler state is independent of image binding. Preserve the real
        // pending values; no image, VkSampler, or draw readiness is invented.
        if(!Sampler::Validate(decoded,bias,error))return false;
        auto next=it==states.end()?std::make_shared<Snapshot>():std::make_shared<Snapshot>(*it->second);
        next->slots[slot].decoded=decoded;next->slots[slot].bias=bias;
        states[state]=std::move(next);return true;
    }
    return Publish(state,slot,it->second->slots[slot].image,decoded,bias,error);
}
void Disable(const GfxCmdBufState* state,unsigned slot) {
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);if(it==states.end()||slot>=16)return;
    auto next=std::make_shared<Snapshot>(*it->second);next->slots[slot]={};it->second=std::move(next);
}
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination) {
    std::lock_guard<std::mutex> lock(mutex);if(source==destination)return;
    auto it=states.find(source);if(it==states.end())states.erase(destination);else states[destination]=it->second;
}
void Release(const GfxCmdBufState* state){std::lock_guard<std::mutex> lock(mutex);states.erase(state);}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState* state) {
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);return it==states.end()?nullptr:it->second;
}
bool AppendResources(const Snapshot& snapshot,const ShaderTranslation& shader,std::vector<ProgramResource>& resources,std::string& error) {
    std::vector<ProgramResource> added;
    for(const auto& reflected:shader.bindings) {
        if(reflected.kind==ResourceKind::Constants)continue;
        if(reflected.registerIndex>=16) {error="Native texture register exceeds command slots";return false;}
        const auto& bound=snapshot.slots[reflected.registerIndex];
        if(!bound.image||!bound.sampler) {error="Native shader texture/sampler is unbound slot="+std::to_string(reflected.registerIndex)+" stage="+std::to_string(unsigned(shader.stage))+" image="+std::to_string(bool(bound.image))+" sampler="+std::to_string(bool(bound.sampler));return false;}
        VkImageLayout layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if(image_usage::Known(bound.image->texture)||bound.image->uploads.empty()) {
            if(!image_usage::Sample(bound.image->texture,layout,error)){error+=" image="+bound.image->createdName+" slot="+std::to_string(reflected.registerIndex)+" serial="+std::to_string(bound.image->serial);return false;}
        } else if(bound.image->uploads.size()!=bound.image->description.mipLevels*bound.image->description.arrayLayers) {
            error="Native sampled image mip/face uploads incomplete";return false;
        }
        ProgramResource resource{};resource.kind=reflected.kind;resource.registerIndex=reflected.registerIndex;
        if(resource.kind==ResourceKind::Texture)resource.image={VK_NULL_HANDLE,bound.image->texture->View(),layout};
        else resource.image={bound.sampler->Handle(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
        added.push_back(resource);
    }
    resources.insert(resources.end(),added.begin(),added.end());return true;
}
}
