#include "render_targets.h"
#include "pointer_hash.h"
#include <mutex>
#include <unordered_map>
#include "gfx_d3d/r_image.h"
namespace bo1::vulkan::render_targets {
namespace {
std::mutex mutex;std::unordered_map<const GfxRenderTarget*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> targets;
struct DepthOwner {std::unique_ptr<GfxImage> owner;std::shared_ptr<const image_assets::Record> record;};
std::unordered_map<std::uint64_t,DepthOwner> depths;
std::vector<DepthOwner> auxiliary;
}
bool Assign(const GfxRenderTarget* target,const GfxImage* owner,bool depth,std::string& error) {
    auto image=image_assets::Acquire(owner);
    if(!target||!image||!image->texture->AttachmentView()) {error="Missing native render-target attachment owner";return false;}
    const auto& d=image->description;
    if(depth?!(d.usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT):!(d.usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {error="Native render-target role mismatch";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(target);
    auto next=it==targets.end()?std::make_shared<Record>():std::make_shared<Record>(*it->second);
    const auto& other=depth?next->color:next->depth;
    if(other&&(other->description.extent.width!=d.extent.width||other->description.extent.height!=d.extent.height)) {error="Native render-target dimensions mismatch";return false;}
    (depth?next->depth:next->color)=std::move(image);targets[target]=std::move(next);return true;
}
std::shared_ptr<const Record> Acquire(const GfxRenderTarget* owner) {std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(owner);return it==targets.end()?nullptr:it->second;}
void Release(const GfxRenderTarget* owner){
    std::lock_guard<std::mutex> lock(mutex);targets.erase(owner);
    for(auto it=auxiliary.begin();it!=auxiliary.end();) {
        bool referenced=false;
        for(const auto& target:targets)if(target.second->color==it->record||target.second->depth==it->record) {referenced=true;break;}
        if(referenced)++it;
        else {image_assets::Release(it->owner.get());it=auxiliary.erase(it);}
    }
}
bool Share(const GfxRenderTarget* source,const GfxRenderTarget* destination,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(source);
    if(!source||!destination||source==destination||it==targets.end()||targets.count(destination)) {error="Invalid native target share source/destination";return false;}
    targets.emplace(destination,it->second);return true;
}
bool AttachDepth(const GfxRenderTarget* target,unsigned width,unsigned height,std::string& error) {
    if(!target||!width||!height||width>65535||height>65535) {error="Invalid native shared depth extent";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(target);
    if(it==targets.end()||!it->second->color||it->second->depth
        ||it->second->color->description.extent.width!=width||it->second->color->description.extent.height!=height) {error="Native shared depth target role/extent mismatch";return false;}
    const std::uint64_t key=(std::uint64_t(width)<<32)|height;auto depth=depths.find(key);
    if(depth==depths.end()) {
        DepthOwner entry;entry.owner=std::make_unique<GfxImage>();entry.owner->mapType=3;entry.owner->width=width;entry.owner->height=height;entry.owner->depth=1;
        if(!image_assets::Create(entry.owner.get(),1,0x20000,D3DFMT_D24S8,error))return false;
        entry.record=image_assets::Acquire(entry.owner.get());depth=depths.emplace(key,std::move(entry)).first;
    }
    auto next=std::make_shared<Record>(*it->second);next->depth=depth->second.record;it->second=std::move(next);return true;
}
bool ShareDepth(const GfxRenderTarget* source,const GfxRenderTarget* destination,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(source);
    if(!source||!destination||source==destination||it==targets.end()||!it->second->depth||targets.count(destination)) {error="Invalid native depth-only target share";return false;}
    auto record=std::make_shared<Record>();record->depth=it->second->depth;targets.emplace(destination,std::move(record));return true;
}
bool AttachAuxiliary(const GfxRenderTarget* target,unsigned width,unsigned height,unsigned format,bool depth,std::string& error) {
    if(!target||!width||!height||width>65535||height>65535) {error="Invalid auxiliary attachment extent";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=targets.find(target);
    if(it==targets.end()||(depth?bool(it->second->depth):bool(it->second->color))) {error="Auxiliary target missing or role already assigned";return false;}
    const auto& other=depth?it->second->color:it->second->depth;
    if(!other||other->description.extent.width!=width||other->description.extent.height!=height) {error="Auxiliary attachment dimensions mismatch";return false;}
    DepthOwner entry;entry.owner=std::make_unique<GfxImage>();entry.owner->mapType=3;entry.owner->width=width;entry.owner->height=height;entry.owner->depth=1;
    if(!image_assets::Create(entry.owner.get(),1,0x20000,format,error))return false;
    entry.record=image_assets::Acquire(entry.owner.get());
    const bool isDepth=(entry.record->description.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)!=0;
    if(isDepth!=depth) {image_assets::Release(entry.owner.get());error="Auxiliary attachment format role mismatch";return false;}
    auto next=std::make_shared<Record>(*it->second);(depth?next->depth:next->color)=entry.record;
    it->second=std::move(next);auxiliary.push_back(std::move(entry));return true;
}
bool References(const GfxImage* owner) {
    auto image=image_assets::Acquire(owner);if(!image)return false;
    std::lock_guard<std::mutex> lock(mutex);
    // Image metadata can advance while targets retain an older immutable record.
    // Sharing/release follows the actual texture allocation, not record identity.
    for(const auto& entry:targets)if((entry.second->color&&entry.second->color->texture==image->texture)
        ||(entry.second->depth&&entry.second->depth->texture==image->texture))return true;
    return false;
}
void Clear(){
    std::lock_guard<std::mutex> lock(mutex);targets.clear();
    for(auto& entry:depths)image_assets::Release(entry.second.owner.get());depths.clear();
    for(auto& entry:auxiliary)image_assets::Release(entry.owner.get());auxiliary.clear();
}
}
