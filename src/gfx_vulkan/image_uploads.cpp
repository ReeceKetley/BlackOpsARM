#include "image_uploads.h"
#include "pointer_hash.h"
#include "model_lighting_regions.h"
#include "upload_flights.h"
#include "platform/android/native_vulkan_device.h"
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <android/log.h>
namespace bo1::vulkan::image_uploads {
namespace {
std::mutex mutex;
thread_local FlushProfile lastProfile;
thread_local std::uint64_t flushSequence{};
using Clock=std::chrono::steady_clock;
struct PhaseTimer {
    bool timed;std::uint64_t& elapsed;Clock::time_point start;
    PhaseTimer(bool enabled,std::uint64_t& value):timed(enabled),elapsed(value){if(timed)start=Clock::now();}
    ~PhaseTimer(){if(timed)elapsed+=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();}
};
struct FlushSample {
    FlushProfile profile;Clock::time_point start;
    explicit FlushSample(bool timed){profile.sampled=timed;profile.sequence=++flushSequence;if(timed)start=Clock::now();}
    ~FlushSample(){
        if(profile.sampled)profile.totalNs=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();
        lastProfile=profile;
        if(!profile.sampled)return;
        const auto ms=[](std::uint64_t ns){return double(ns)/1e6;};
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "native_image_upload_cpu sample=1 flush=%llu success=%u images=%u batches=%u staged_bytes=%llu region_count=%u box_bytes=%llu box_full_fallbacks=%u allocations=%u reuses=%u total_ms=%.3f retire_ms=%.3f validation_ms=%.3f staging_ms=%.3f pack_ms=%.3f begin_ms=%.3f record_ms=%.3f submit_ms=%.3f",
            (unsigned long long)profile.sequence,unsigned(profile.success),profile.images,profile.batches,
            (unsigned long long)profile.stagedBytes,profile.regionCount,(unsigned long long)profile.boxBytes,profile.boxFullFallbacks,profile.stagingAllocations,profile.stagingReuses,
            ms(profile.totalNs),ms(profile.retireNs),ms(profile.validationNs),ms(profile.stagingNs),
            ms(profile.packNs),ms(profile.beginNs),ms(profile.recordNs),ms(profile.submitNs));
    }
};
using Record=image_assets::Record;
using Pending=model_lighting_regions::Pending;
std::unordered_map<const Texture2D*,Pending,bo1::vulkan::PointerHash> pending;
void Enqueue(Pending next) {
    auto found=pending.find(next.record->texture.get());
    if(found==pending.end())pending.emplace(next.record->texture.get(),std::move(next));
    else found->second=model_lighting_regions::Merge(std::move(found->second),std::move(next));
}
struct State {std::weak_ptr<Texture2D> texture;std::weak_ptr<const Record> ordered;std::vector<bool> defined;std::uint64_t orderedRevision{};};
std::unordered_map<const Texture2D*,State,bo1::vulkan::PointerHash> states;
std::vector<std::shared_ptr<Flight>> inFlight;bool failed{};
std::vector<std::shared_ptr<Buffer>> spareStaging; // reused once their flight retires
unsigned submittedBatches{},retiredBatches{},pruneCalls{};
std::size_t prunedSize{};
constexpr VkDeviceSize budget=64*1024*1024;
struct Copy {std::shared_ptr<const Record> record;const image_assets::Upload* upload;VkDeviceSize offset;std::vector<unsigned> tiles;std::vector<VkBufferImageCopy> regions;model_lighting_regions::Box box;bool boxed{};};
void Published(void* data,unsigned,VkFence fence) {
    std::lock_guard<std::mutex> lock(mutex);static_cast<Flight*>(data)->fence=fence;++submittedBatches;
}
void Prune() {
    // Entries contain weak owners only. Identity checks below reject recycled
    // addresses; housekeeping need not scan every image on each resource poll.
    // Periodic sweeps and a growth threshold bound stale metadata accumulation.
    if(++pruneCalls<256 && states.size()<prunedSize+1024)return;
    for(auto it=states.begin();it!=states.end();)if(it->second.texture.expired())it=states.erase(it);else ++it;
    pruneCalls=0;prunedSize=states.size();
}
}
void Queue(std::shared_ptr<const Record> record) {
    std::lock_guard<std::mutex> lock(mutex);Enqueue(Pending(std::move(record)));
}
bool Ready(const std::shared_ptr<const Record>& record) {
    if(!record)return false;std::lock_guard<std::mutex> lock(mutex);
    if(failed)return false;
    auto it=states.find(record->texture.get());return it!=states.end()&&it->second.texture.lock()==record->texture&&it->second.ordered.lock()==record;
}
bool Clear(std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);
    if(!inFlight.empty()) {
        auto renderer=bo1::android::native_device::Renderer();
        if(!renderer||vkDeviceWaitIdle(renderer->Device())!=VK_SUCCESS) {error="Native upload staging is not retired";return false;}
    }
    inFlight.clear();spareStaging.clear();pending.clear();states.clear();pruneCalls=0;prunedSize=0;failed=false;return true;
}
FlushProfile ReadFlushProfile(){return lastProfile;}
bool Flush(std::string& error,bool timed) {
    FlushSample sample(timed);auto& profile=sample.profile;
    error.clear();std::vector<Pending> records;
    auto renderer=bo1::android::native_device::Renderer();
    {
        // Includes prune, fence retirement, pending snapshot copy and mutex wait.
        PhaseTimer phase(timed,profile.retireNs);
        std::lock_guard<std::mutex> lock(mutex);Prune();
        if(renderer&&!RetireFlights(renderer->Device(),inFlight,retiredBatches,error)){failed=true;return false;}
        if(failed) {error="Native upload failure requires device retirement";return false;}
        for(const auto& item:pending)records.push_back(item.second);
        pending.clear();
    }
    profile.images=unsigned(records.size());
    if(records.empty()){profile.success=true;return true;}
    auto restore=[&](){std::lock_guard<std::mutex> lock(mutex);for(auto& record:records)Enqueue(record);};
    if(!renderer||renderer->FrameRecording()) {error="Native image upload flush requires its device outside a recording frame";restore();return false;}
    // Validate the complete immutable snapshot before recording any GPU work.
    {PhaseTimer phase(timed,profile.validationNs);
    for(const auto& queued:records) {
        const auto& record=queued.record;
        const auto& d=record->description;
        if(!record->texture||!d.mipLevels||!d.arrayLayers||record->uploads.size()!=std::size_t(d.mipLevels)*d.arrayLayers) {
            error="Incomplete native image mip/layer uploads: "+record->createdName;restore();return false;
        }
        std::vector<bool> seen(std::size_t(d.mipLevels)*d.arrayLayers);
        for(const auto& upload:record->uploads) {
            if(upload.mip>=d.mipLevels||upload.mip>=32||upload.layer>=d.arrayLayers) {error="Invalid native upload subresource index";restore();return false;}
            const auto index=std::size_t(upload.layer)*d.mipLevels+upload.mip;
            const std::uint64_t bytes=image_assets::SubresourceBytes(*record,upload.mip);
            // A released subresource is already on the GPU (uploadsReleased).
            if(!upload.bytes&&record->uploadsReleased&&upload.mip<d.mipLevels&&upload.layer<d.arrayLayers&&!seen[index]){seen[index]=true;continue;}
            if(upload.mip>=d.mipLevels||upload.layer>=d.arrayLayers||seen[index]||!upload.bytes||upload.bytes->size()!=bytes||bytes>budget) {
                error="Invalid or oversized native upload subresource: "+record->createdName;restore();return false;
            }
            seen[index]=true;
        }
    }
    }
    std::vector<Copy> batch;VkDeviceSize bytes=0;unsigned batches=0,subresources=0,regionTiles=0;VkDeviceSize total=0;
    auto submit=[&]()->bool {
        if(batch.empty())return true;
        // Reuse a retired staging buffer (its only owner is the pool once its
        // flight retired) instead of allocating device memory per upload.
        std::shared_ptr<Buffer> staging;
        {PhaseTimer phase(timed,profile.stagingNs);
        {
            std::lock_guard<std::mutex> lock(mutex);
            for(auto it=spareStaging.begin();it!=spareStaging.end();++it)
                if(it->use_count()==1&&(*it)->Size()>=bytes){staging=*it;break;}
        }
        if(!staging) {
            ++profile.stagingAllocations;
            VkDeviceSize size=1u<<20;while(size<bytes)size<<=1;
            staging=std::make_shared<Buffer>(renderer->Physical(),renderer->Device());
            if(!staging->Create(size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error))return false;
            std::lock_guard<std::mutex> lock(mutex);
            if(spareStaging.size()<8)spareStaging.push_back(staging);
        } else ++profile.stagingReuses;
        }
        {PhaseTimer phase(timed,profile.packNs);
        for(auto& copy:batch) {
            if(copy.tiles.empty()) {if(!staging->Write(copy.offset,copy.upload->bytes->data(),copy.upload->bytes->size(),error))return false;}
            else {
                std::vector<unsigned char> packed;
                if(copy.boxed)model_lighting_regions::PackBox(*copy.record,copy.box,copy.offset,packed,copy.regions);
                else model_lighting_regions::Pack(*copy.record,copy.tiles,copy.offset,packed,copy.regions);
                if(!staging->Write(copy.offset,packed.data(),packed.size(),error))return false;
            }
        }
        }
        auto flight=std::make_shared<Flight>();flight->staging=staging;
        for(const auto& copy:batch)flight->records.push_back(copy.record);
        {std::lock_guard<std::mutex> lock(mutex);inFlight.push_back(flight);}
        FrameContext frame{};
        {PhaseTimer phase(timed,profile.beginNs);
            if(renderer->BeginWork(frame,error)!=FrameResult::Recording)return false;
        }
        {PhaseTimer phase(timed,profile.recordNs);
        for(const auto& copy:batch) {
            VkImageLayout previous=VK_IMAGE_LAYOUT_UNDEFINED;
            {
                std::lock_guard<std::mutex> lock(mutex);auto& state=states[copy.record->texture.get()];
                if(state.texture.lock()!=copy.record->texture) {state={};state.texture=copy.record->texture;state.defined.resize(std::size_t(copy.record->description.mipLevels)*copy.record->description.arrayLayers);}
                const auto index=std::size_t(copy.upload->layer)*copy.record->description.mipLevels+copy.upload->mip;
                if(state.defined[index])previous=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            if(copy.tiles.empty()) {
                if(!copy.record->texture->UploadSubresource(frame.command,staging->Handle(),copy.offset,copy.upload->mip,copy.upload->layer,previous,error))return false;
            } else {
                if(previous!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){error="Partial atlas upload lacks initialized contents";return false;}
                if(!copy.record->texture->UploadRegions(frame.command,staging->Handle(),staging->Size(),copy.regions,error))return false;
            }
        }
        }
        {PhaseTimer phase(timed,profile.submitNs);
            if(renderer->SubmitFrame(error,Published,flight.get())!=FrameResult::Presented)return false;
        }
        ++profile.batches;profile.stagedBytes+=bytes;
        // Same-queue consumers use the recorded barriers. These layouts become
        // ordered only after successful submission; staging/image owners remain
        // in Flight until its actual fence completes. No host completion claim.
        {
            std::lock_guard<std::mutex> lock(mutex);
            for(const auto& copy:batch)states[copy.record->texture.get()].defined[std::size_t(copy.upload->layer)*copy.record->description.mipLevels+copy.upload->mip]=true;
        }
        ++batches;subresources+=unsigned(batch.size());total+=bytes;batch.clear();bytes=0;return true;
    };
    for(const auto& queued:records) {
        const auto& record=queued.record;
        if(Ready(record))continue;
        bool partial=false;
        {
            std::lock_guard<std::mutex> lock(mutex);const auto found=states.find(record->texture.get());
            if(found!=states.end())partial=model_lighting_regions::CanPartial(queued,found->second.texture.lock(),
                found->second.orderedRevision,found->second.defined.size()==1&&found->second.defined[0]);
        }
        model_lighting_regions::Box box;bool boxed=false;
        if(partial&&image_assets::ModelLightingRegionsEnabled()&&image_assets::ModelLightingBoxesEnabled()) {
            boxed=model_lighting_regions::PlanBox(*record,queued.tiles,box);
            if(!boxed){partial=false;++profile.boxFullFallbacks;}
        }
        for(const auto& upload:record->uploads) {
            if(!upload.bytes)continue; // released: already uploaded
            if(partial)regionTiles+=unsigned(queued.tiles.size());
            const VkDeviceSize copyBytes=partial?(boxed?box.bytes:queued.tiles.size()*256):upload.bytes->size();
            profile.regionCount+=partial&&!boxed?unsigned(queued.tiles.size()):1;
            if(boxed)profile.boxBytes+=copyBytes;
            const auto offset=(bytes+15)&~VkDeviceSize(15);
            if(offset+copyBytes>budget) {if(!submit()){restore();std::lock_guard<std::mutex> lock(mutex);failed=true;return false;}}
            const auto aligned=(bytes+15)&~VkDeviceSize(15);batch.push_back({record,&upload,aligned,partial?queued.tiles:std::vector<unsigned>{},{},box,boxed});bytes=aligned+copyBytes;
        }
    }
    if(!submit()){restore();std::lock_guard<std::mutex> lock(mutex);failed=true;return false;}
    {
        std::lock_guard<std::mutex> lock(mutex);for(const auto& queued:records){auto& state=states[queued.record->texture.get()];state.ordered=queued.record;state.orderedRevision=queued.record->revision;}
    }
    // The staging buffers hold every copied byte now: drop the CPU copies.
    // Lock order is image_assets then image_uploads, so release outside ours.
    {
        std::vector<std::shared_ptr<const Record>> uploaded;uploaded.reserve(records.size());
        for(const auto& queued:records)uploaded.push_back(queued.record);
        const auto replaced=image_assets::ReleaseUploadedBytes(uploaded);
        std::lock_guard<std::mutex> lock(mutex);
        for(const auto& pair:replaced) {
            auto found=states.find(pair.second->texture.get());
            if(found!=states.end()&&found->second.ordered.lock()==pair.first)found->second.ordered=pair.second;
        }
    }
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_image_upload_batch=pass images=%zu subresources=%u batches=%u staging_bytes=%llu region_tiles=%u region_count=%u box_bytes=%llu box_full_fallbacks=%u queue_ordered=1 host_wait=0 flights=%zu submitted=%u retired=%u",records.size(),subresources,batches,(unsigned long long)total,regionTiles,profile.regionCount,(unsigned long long)profile.boxBytes,profile.boxFullFallbacks,inFlight.size(),submittedBatches,retiredBatches);
    profile.success=true;return true;
}
}
