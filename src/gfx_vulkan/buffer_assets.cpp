#include "buffer_assets.h"
#include "pointer_hash.h"
#include "shader_assets.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>
namespace bo1::vulkan::buffer_assets {
namespace {
std::mutex mutex;VkPhysicalDevice physical{};VkDevice device{};std::uint64_t serial{};
std::unordered_map<const void*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> buffers;
std::vector<std::weak_ptr<Buffer>> live;
std::unordered_map<const void*,std::shared_ptr<Record>,bo1::vulkan::PointerHash> pending;
bool Drained(){live.erase(std::remove_if(live.begin(),live.end(),[](const auto& p){return p.expired();}),live.end());return buffers.empty()&&pending.empty()&&live.empty();}
void Track(const std::shared_ptr<Buffer>& buffer) {
    // Bound discard tracking by live allocations, not all past generations.
    live.erase(std::remove_if(live.begin(),live.end(),[](const auto& p){return p.expired();}),live.end());
    live.push_back(buffer);
}
// Retired dynamic backings per owner. Readers (records, snapshots, submitted
// packets) hold shared references until their fence retires, so a spare whose
// only reference is this list has no CPU or GPU user and can be rewritten.
std::unordered_map<const void*,std::vector<std::shared_ptr<Buffer>>,bo1::vulkan::PointerHash> spares;
// The engine's 1MB dynamic vertex/index buffers wrap several times per frame
// with up to six frames in flight; small backings keep enough spares to cover
// that. Large caches (e.g. the 8MB static-model cache) keep two.
constexpr std::size_t MaxSmallSpares=16,MaxLargeSpares=2;constexpr VkDeviceSize SmallSpareBytes=2u<<20;
std::shared_ptr<Buffer> Replacement(const void* owner,const std::shared_ptr<Buffer>& retired,Kind kind,std::string& error) {
    const auto capacity=retired->Size();auto& list=spares[owner];std::shared_ptr<Buffer> result;
    for(auto it=list.begin();it!=list.end();++it)if(it->use_count()==1&&(*it)->Size()==capacity){result=std::move(*it);list.erase(it);break;}
    if(!result) {
        result=std::make_shared<Buffer>(physical,device);
        const auto usage=kind==Kind::Vertex?VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if(!result->Create(capacity,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error))return nullptr;
        Track(result);
    }
    if(list.size()<(capacity<=SmallSpareBytes?MaxSmallSpares:MaxLargeSpares))list.push_back(retired);
    return result;
}
}
bool CloneOwners(std::initializer_list<OwnerCopy> copies,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::shared_ptr<const Record>> records;records.reserve(copies.size());
    for(const auto& copy:copies) {
        if(!copy.source||!copy.destination||pending.count(copy.source)||pending.count(copy.destination)) {
            error="Native buffer owner clone has an absent slot or pending writer";return false;
        }
        for(const auto& other:copies)if(&other!=&copy&&other.destination==copy.destination) {
            error="Native buffer owner clone has duplicate destinations";return false;
        }
        const auto it=buffers.find(copy.source);records.push_back(it==buffers.end()?nullptr:it->second);
    }
    unsigned i=0;for(const auto& copy:copies) {
        const auto& record=records[i++];
        if(record)buffers[copy.destination]=record;else buffers.erase(copy.destination);
    }
    return true;
}
bool Empty(){std::lock_guard<std::mutex> lock(mutex);return Drained();}
bool Attach(VkPhysicalDevice p,VkDevice d,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);
    if((!p)!=(!d)) {error="Incomplete native buffer context";return false;}
    if(p==physical&&d==device)return true;
    if(!Drained()) {error="Native buffer device has live owners/borrowers";return false;}
    physical=p;device=d;return true;
}
bool Load(const void* owner,const void* data,std::size_t payload,std::size_t allocation,Kind kind,std::string& error) {
    if(!owner||!data||!payload||allocation<payload||(kind==Kind::Index16&&(payload&1))) {error="Invalid native buffer source/range";return false;}
    std::lock_guard<std::mutex> lock(mutex);
    if(!shader_assets::Active()||!device||buffers.count(owner)||pending.count(owner)) {error="Native buffer context unavailable or owner already loaded";return false;}
    auto record=std::make_shared<Record>();record->kind=kind;record->payloadBytes=payload;
    record->gpu=std::make_shared<Buffer>(physical,device);
    const auto usage=kind==Kind::Vertex?VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if(!record->gpu->Create(allocation,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error)
        ||!record->gpu->Write(0,data,payload,error))return false;
    if(allocation>payload) {
        std::vector<unsigned char> padding(allocation-payload,0);
        if(!record->gpu->Write(payload,padding.data(),padding.size(),error))return false;
    }
    record->serial=++serial;Track(record->gpu);buffers.emplace(owner,std::move(record));return true;
}
void* Begin(const void* owner,std::size_t allocation,Kind kind,std::string& error) {
    if(!owner||!allocation||(kind==Kind::Index16&&(allocation&1))) {error="Invalid native initialization buffer capacity";return nullptr;}
    std::lock_guard<std::mutex> lock(mutex);
    if(!shader_assets::Active()||!device||buffers.count(owner)||pending.count(owner)) {error="Native initialization context unavailable or owner already loaded";return nullptr;}
    auto record=std::make_shared<Record>();record->kind=kind;record->gpu=std::make_shared<Buffer>(physical,device);
    const auto usage=kind==Kind::Vertex?VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if(!record->gpu->Create(allocation,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error))return nullptr;
    void* mapped=record->gpu->MappedForInitialization();
    if(!mapped) {error="Native initialization buffer has no mapping";return nullptr;}
    Track(record->gpu);pending.emplace(owner,std::move(record));return mapped;
}
bool Finish(const void* owner,Kind kind,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);auto it=pending.find(owner);
    if(!shader_assets::Active()||it==pending.end()||it->second->kind!=kind||(buffers.count(owner)&&!it->second->dynamic)) {error="Native buffer finish without matching initialization owner";return false;}
    it->second->payloadBytes=it->second->gpu->Size();it->second->serial=++serial;
    buffers[owner]=it->second;pending.erase(it);return true;
}
void* MapDiscard(const void* owner,std::string& error,bool preserve) {
    std::lock_guard<std::mutex> lock(mutex);auto it=buffers.find(owner);
    if(!shader_assets::Active()||it==buffers.end()||!it->second->dynamic||pending.count(owner)) {error="Native mapped discard requires an unlocked dynamic owner";return nullptr;}
    const auto& previous=it->second;auto record=std::make_shared<Record>();record->kind=previous->kind;record->dynamic=true;
    if(previous.use_count()>1||previous->gpu.use_count()>1) {
        record->gpu=std::make_shared<Buffer>(physical,device);
        const auto usage=record->kind==Kind::Vertex?VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if(!record->gpu->Create(previous->gpu->Size(),usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error))return nullptr;
        Track(record->gpu);
    } else record->gpu=previous->gpu;
    void* mapped=record->gpu->MappedForInitialization();
    if(preserve) {
        if(record->gpu!=previous->gpu&&previous->payloadBytes&&!record->gpu->Write(0,previous->gpu->MappedForInitialization(),previous->payloadBytes,error))return nullptr;
        if(previous->payloadBytes<record->gpu->Size())std::memset(static_cast<unsigned char*>(mapped)+previous->payloadBytes,0,record->gpu->Size()-previous->payloadBytes);
    }
    pending.emplace(owner,std::move(record));return mapped;
}
std::shared_ptr<const Record> Acquire(const void* owner) {std::lock_guard<std::mutex> lock(mutex);auto it=buffers.find(owner);return pending.count(owner)||it==buffers.end()?nullptr:it->second;}
bool Reserve(const void* owner,std::size_t allocation,Kind kind,std::string& error) {
    if(!owner||!allocation||(kind==Kind::Index16&&(allocation&1))) {error="Invalid native dynamic buffer capacity";return false;}
    std::lock_guard<std::mutex> lock(mutex);
    if(!shader_assets::Active()||!device||buffers.count(owner)||pending.count(owner)) {error="Native dynamic buffer context unavailable or owner already loaded";return false;}
    auto record=std::make_shared<Record>();record->kind=kind;record->dynamic=true;
    record->gpu=std::make_shared<Buffer>(physical,device);
    const auto usage=kind==Kind::Vertex?VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    // Coherent memory permits append beside submitted readers without a flush
    // touching their non-coherent atom. No staging copy or queue idle is needed.
    if(!record->gpu->Create(allocation,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error))return false;
    record->serial=++serial;Track(record->gpu);buffers.emplace(owner,std::move(record));return true;
}
bool Write(const void* owner,std::size_t offset,const void* data,std::size_t payload,std::string& error) {
    if(!owner||!data||!payload) {error="Invalid native dynamic buffer write";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=buffers.find(owner);
    if(!shader_assets::Active()||it==buffers.end()||!it->second->dynamic||pending.count(owner)) {error="Native dynamic buffer owner unavailable";return false;}
    const auto& previous=it->second;
    const auto capacity=previous->gpu->Size();
    if(offset>capacity||payload>capacity-offset||(offset&&offset!=previous->payloadBytes)
        ||(previous->kind==Kind::Index16&&((offset|payload)&1))) {error="Invalid native dynamic append/discard range";return false;}
    auto record=std::make_shared<Record>();record->kind=previous->kind;record->dynamic=true;
    // An earlier record can own the same backing even when the current record
    // itself is unborrowed. Check both ownership counts before discard reuse.
    if(!offset&&(previous.use_count()>1||previous->gpu.use_count()>1)) {
        record->gpu=Replacement(owner,previous->gpu,record->kind,error);if(!record->gpu)return false;
    } else record->gpu=previous->gpu;
    if(!record->gpu->Write(offset,data,payload,error))return false;
    record->payloadBytes=offset+payload;record->serial=++serial;it->second=std::move(record);return true;
}
bool UpdateRange(const void* owner,std::size_t offset,const void* data,std::size_t payload,std::string& error) {
    if(!owner||!data||!payload) {error="Invalid native buffer patch";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=buffers.find(owner);
    if(!shader_assets::Active()||it==buffers.end()||!it->second->dynamic||pending.count(owner)) {error="Native patch requires an unlocked dynamic owner";return false;}
    const auto& previous=it->second;const auto capacity=previous->gpu->Size();
    if(offset>capacity||payload>capacity-offset||(previous->kind==Kind::Index16&&((offset|payload)&1))) {error="Native patch exceeds capacity/alignment";return false;}
    auto record=std::make_shared<Record>();record->kind=previous->kind;record->dynamic=true;
    if(offset<previous->payloadBytes&&(previous.use_count()>1||previous->gpu.use_count()>1)) {
        record->gpu=Replacement(owner,previous->gpu,record->kind,error);if(!record->gpu)return false;
        // Dynamic backings are coherent and CPU-owned for writes. Preserve the
        // already published prefix when replacing a retained cache generation.
        if(previous->payloadBytes&&!record->gpu->Write(0,previous->gpu->MappedForInitialization(),previous->payloadBytes,error))return false;
    } else record->gpu=previous->gpu;
    if(offset>previous->payloadBytes)std::memset(static_cast<unsigned char*>(record->gpu->MappedForInitialization())+previous->payloadBytes,0,offset-previous->payloadBytes);
    if(!record->gpu->Write(offset,data,payload,error))return false;
    record->payloadBytes=std::max(previous->payloadBytes,offset+payload);record->serial=++serial;it->second=std::move(record);return true;
}
void Release(const void* owner){std::lock_guard<std::mutex> lock(mutex);buffers.erase(owner);pending.erase(owner);spares.erase(owner);}
}
