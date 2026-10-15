#include "program_cache.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
namespace bo1::vulkan {
ProgramCache::~ProgramCache(){Clear();if(driver_)vkDestroyPipelineCache(device_,driver_,nullptr);}
void ProgramCache::EraseRenderPass(VkRenderPass pass) {
    for(auto it=programs_.begin();it!=programs_.end();)if(it->first.RenderPass()==pass){++evictions_;it=programs_.erase(it);}else ++it;
}
bool ProgramCache::Initialize(std::string& error,std::string path,const VkPhysicalDeviceProperties* properties) {
    if(!device_||!capacity_||driver_) {error="Invalid native program cache initialization";return false;}
    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    std::vector<unsigned char> stored;
    if(!path.empty()&&properties) {
        path_=std::move(path);
        if(FILE* file=std::fopen(path_.c_str(),"rb")) {
            if(!std::fseek(file,0,SEEK_END)) {
                const long size=std::ftell(file);
                if(size>=32&&size<=(64l<<20)&&!std::fseek(file,0,SEEK_SET)) {
                    stored.resize(std::size_t(size));if(std::fread(stored.data(),1,stored.size(),file)!=stored.size())stored.clear();
                }
            }
            std::fclose(file);
        }
        // VkPipelineCacheHeaderVersionOne: length, version, vendor, device, UUID.
        std::uint32_t header[4]{};if(stored.size()>=32)std::memcpy(header,stored.data(),sizeof(header));
        if(stored.size()<32||header[0]<32||header[0]>stored.size()||header[1]!=VK_PIPELINE_CACHE_HEADER_VERSION_ONE
            ||header[2]!=properties->vendorID||header[3]!=properties->deviceID
            ||std::memcmp(stored.data()+16,properties->pipelineCacheUUID,VK_UUID_SIZE))stored.clear();
        info.initialDataSize=stored.size();info.pInitialData=stored.empty()?nullptr:stored.data();
    }
    auto result=vkCreatePipelineCache(device_,&info,nullptr,&driver_);
    if(result!=VK_SUCCESS&&!stored.empty()) {info.initialDataSize=0;info.pInitialData=nullptr;result=vkCreatePipelineCache(device_,&info,nullptr,&driver_);}
    if(result!=VK_SUCCESS) {error="Native driver pipeline cache: "+std::to_string(int(result));return false;}return true;
}
bool ProgramCache::Save(std::string& error) {
    if(!driver_||path_.empty())return true;
    std::size_t size=0;if(vkGetPipelineCacheData(device_,driver_,&size,nullptr)!=VK_SUCCESS) {error="Native pipeline cache size query failed";return false;}
    if(!size)return true;
    std::vector<unsigned char> data(size);
    if(vkGetPipelineCacheData(device_,driver_,&size,data.data())!=VK_SUCCESS) {error="Native pipeline cache read failed";return false;}
    const std::string temporary=path_+".tmp";FILE* file=std::fopen(temporary.c_str(),"wb");
    if(!file) {error="Native pipeline cache temp file unavailable";return false;}
    const bool written=std::fwrite(data.data(),1,size,file)==size;
    if(std::fclose(file)!=0||!written||std::rename(temporary.c_str(),path_.c_str())!=0) {std::remove(temporary.c_str());error="Native pipeline cache write failed";return false;}
    return true;
}
std::shared_ptr<GraphicsProgram> ProgramCache::Acquire(const ProgramDescription& description,std::string& error) {
    if(!driver_) {error="Native program cache is not initialized";return {};}
    const auto keyStart=timing_?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    PipelineKey key;const bool keyed=PipelineKey::Create(description,key,error);
    if(timing_)keyNs_+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-keyStart).count();
    if(!keyed)return {};
    auto found=programs_.find(key);if(found!=programs_.end()) {++hits_;return found->second;}
    if(programs_.size()>=capacity_) {
        // Only cache-owned programs can be evicted. In-flight frame borrowers
        // prevent destruction; a fully pinned cache fails explicitly.
        auto victim=programs_.begin();for(;victim!=programs_.end();++victim)if(victim->second.use_count()==1)break;
        if(victim==programs_.end()) {error="Native program cache capacity pinned by frame borrowers";return {};}
        programs_.erase(victim);++evictions_;
    }
    auto program=std::make_shared<GraphicsProgram>(device_);
    const auto createStart=std::chrono::steady_clock::now();
    const bool created=program->Create(description,driver_,error);
    createNs_+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-createStart).count();
    if(!created)return {};
    programs_.emplace(std::move(key),program);++misses_;return program;
}
}
