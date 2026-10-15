#include "image_uploads.h"
#include "pointer_hash.h"
#include "upload_flights.h"
#include "platform/android/native_vulkan_device.h"
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <android/log.h>
namespace bo1::vulkan::image_uploads {
namespace {
std::mutex mutex;
using Record=image_assets::Record;
std::unordered_map<const Texture2D*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> pending;
struct State {std::weak_ptr<Texture2D> texture;std::weak_ptr<const Record> ordered;std::vector<bool> defined;};
std::unordered_map<const Texture2D*,State,bo1::vulkan::PointerHash> states;
std::vector<std::shared_ptr<Flight>> inFlight;bool failed{};
std::vector<std::shared_ptr<Buffer>> spareStaging; // reused once their flight retires
unsigned submittedBatches{},retiredBatches{},pruneCalls{};
std::size_t prunedSize{};
constexpr VkDeviceSize budget=64*1024*1024;
struct Copy {std::shared_ptr<const Record> record;const image_assets::Upload* upload;VkDeviceSize offset;};
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
    std::lock_guard<std::mutex> lock(mutex);pending[record->texture.get()]=std::move(record);
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
bool Flush(std::string& error) {
    error.clear();std::vector<std::shared_ptr<const Record>> records;
    auto renderer=bo1::android::native_device::Renderer();
    {
        std::lock_guard<std::mutex> lock(mutex);Prune();
        if(renderer&&!RetireFlights(renderer->Device(),inFlight,retiredBatches,error)){failed=true;return false;}
        if(failed) {error="Native upload failure requires device retirement";return false;}
        for(const auto& item:pending)records.push_back(item.second);
        pending.clear();
    }
    if(records.empty())return true;
    auto restore=[&](){std::lock_guard<std::mutex> lock(mutex);for(auto& record:records)pending.emplace(record->texture.get(),record);};
    if(!renderer||renderer->FrameRecording()) {error="Native image upload flush requires its device outside a recording frame";restore();return false;}
    // Validate the complete immutable snapshot before recording any GPU work.
    for(const auto& record:records) {
        const auto& d=record->description;
        if(!record->texture||!d.mipLevels||!d.arrayLayers||record->uploads.size()!=std::size_t(d.mipLevels)*d.arrayLayers) {
            error="Incomplete native image mip/layer uploads: "+record->createdName;restore();return false;
        }
        std::vector<bool> seen(std::size_t(d.mipLevels)*d.arrayLayers);
        for(const auto& upload:record->uploads) {
            if(upload.mip>=d.mipLevels||upload.mip>=32||upload.layer>=d.arrayLayers) {error="Invalid native upload subresource index";restore();return false;}
            const auto index=std::size_t(upload.layer)*d.mipLevels+upload.mip;
            const std::uint64_t bytes=image_assets::SubresourceBytes(*record,upload.mip);
            if(upload.mip>=d.mipLevels||upload.layer>=d.arrayLayers||seen[index]||!upload.bytes||upload.bytes->size()!=bytes||bytes>budget) {
                error="Invalid or oversized native upload subresource: "+record->createdName;restore();return false;
            }
            seen[index]=true;
        }
    }
    std::vector<Copy> batch;VkDeviceSize bytes=0;unsigned batches=0,subresources=0;VkDeviceSize total=0;
    auto submit=[&]()->bool {
        if(batch.empty())return true;
        // Reuse a retired staging buffer (its only owner is the pool once its
        // flight retired) instead of allocating device memory per upload.
        std::shared_ptr<Buffer> staging;
        {
            std::lock_guard<std::mutex> lock(mutex);
            for(auto it=spareStaging.begin();it!=spareStaging.end();++it)
                if(it->use_count()==1&&(*it)->Size()>=bytes){staging=*it;break;}
        }
        if(!staging) {
            VkDeviceSize size=1u<<20;while(size<bytes)size<<=1;
            staging=std::make_shared<Buffer>(renderer->Physical(),renderer->Device());
            if(!staging->Create(size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error))return false;
            std::lock_guard<std::mutex> lock(mutex);
            if(spareStaging.size()<8)spareStaging.push_back(staging);
        }
        for(const auto& copy:batch)if(!staging->Write(copy.offset,copy.upload->bytes->data(),copy.upload->bytes->size(),error))return false;
        auto flight=std::make_shared<Flight>();flight->staging=staging;
        for(const auto& copy:batch)flight->records.push_back(copy.record);
        {std::lock_guard<std::mutex> lock(mutex);inFlight.push_back(flight);}
        FrameContext frame{};
        if(renderer->BeginWork(frame,error)!=FrameResult::Recording)return false;
        for(const auto& copy:batch) {
            VkImageLayout previous=VK_IMAGE_LAYOUT_UNDEFINED;
            {
                std::lock_guard<std::mutex> lock(mutex);auto& state=states[copy.record->texture.get()];
                if(state.texture.lock()!=copy.record->texture) {state={};state.texture=copy.record->texture;state.defined.resize(std::size_t(copy.record->description.mipLevels)*copy.record->description.arrayLayers);}
                const auto index=std::size_t(copy.upload->layer)*copy.record->description.mipLevels+copy.upload->mip;
                if(state.defined[index])previous=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            if(!copy.record->texture->UploadSubresource(frame.command,staging->Handle(),copy.offset,copy.upload->mip,copy.upload->layer,previous,error))return false;
        }
        if(renderer->SubmitFrame(error,Published,flight.get())!=FrameResult::Presented)return false;
        // Same-queue consumers use the recorded barriers. These layouts become
        // ordered only after successful submission; staging/image owners remain
        // in Flight until its actual fence completes. No host completion claim.
        {
            std::lock_guard<std::mutex> lock(mutex);
            for(const auto& copy:batch)states[copy.record->texture.get()].defined[std::size_t(copy.upload->layer)*copy.record->description.mipLevels+copy.upload->mip]=true;
        }
        ++batches;subresources+=unsigned(batch.size());total+=bytes;batch.clear();bytes=0;return true;
    };
    for(const auto& record:records) {
        if(Ready(record))continue;
        for(const auto& upload:record->uploads) {
            const auto offset=(bytes+15)&~VkDeviceSize(15);
            if(offset+upload.bytes->size()>budget) {if(!submit()){restore();std::lock_guard<std::mutex> lock(mutex);failed=true;return false;}}
            const auto aligned=(bytes+15)&~VkDeviceSize(15);batch.push_back({record,&upload,aligned});bytes=aligned+upload.bytes->size();
        }
    }
    if(!submit()){restore();std::lock_guard<std::mutex> lock(mutex);failed=true;return false;}
    {
        std::lock_guard<std::mutex> lock(mutex);for(const auto& record:records)states[record->texture.get()].ordered=record;
    }
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_image_upload_batch=pass images=%zu subresources=%u batches=%u staging_bytes=%llu queue_ordered=1 host_wait=0 flights=%zu submitted=%u retired=%u",records.size(),subresources,batches,(unsigned long long)total,inFlight.size(),submittedBatches,retiredBatches);
    return true;
}
}
